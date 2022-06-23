// clang-import.cc
// Code for clang-import.h.

#include "clang-import-internal.h"     // this module

// elsa
#include "clang-print.h"               // toString(CXCursorKind), etc.
#include "libclang-additions.h"        // clang_getUnaryExpressionOperator, etc.

// ast
#include "asthelp.h"                   // ind

// smbase
#include "map-utils.h"                 // insertMapUnique
#include "exc.h"                       // xfatal
#include "str.h"                       // string
#include "trace.h"                     // tracingSys


// This is a candidate to go into smbase.
template <class DEST, class SRC>
void checkedAssignment(DEST &dest, SRC src)
{
  dest = static_cast<DEST>(src);
  if (static_cast<SRC>(dest) != src ||
      (dest < 0) != (src < 0)) {
    xfatal(stringb("Value " << src << " is outside representable range."));
  }
}


static string cxErrorCodeString(CXErrorCode code)
{
  static char const * const table[] = {
    "Success",
    "Failure",
    "Crashed",
    "InvalidArguments",
    "ASTReadError"
  };

  if ((unsigned)code < TABLESIZE(table)) {
    return string(table[code]);
  }
  else {
    return stringb("Unknown error code: " << (int)code);
  }
}


// Entry point to this module.
void clangParseTranslationUnit(
  ElsaParse &elsaParse,
  std::vector<std::string> const &gccOptions)
{
  CXIndex cxIndex = clang_createIndex(
    0 /*excludeDeclarationsFromPCH*/,
    1 /*displayDiagnostics*/);
  DisposeCXIndex disposeCxIndex(cxIndex);

  // Copy the arguments into an ordinary array of pointers.
  char const **commandLine = new char const * [gccOptions.size()];
  {
    size_t i = 0;
    for (std::string const &s : gccOptions) {
      xassert(i < gccOptions.size());
      commandLine[i++] = s.c_str();
    }
  }

  CXTranslationUnit cxTU;
  CXErrorCode errorCode = clang_parseTranslationUnit2(
    cxIndex,
    nullptr,                 // source_filename
    commandLine,
    (int)gccOptions.size(),
    nullptr,                 // unsaved_files
    0,                       // num_unsaved_files
    CXTranslationUnit_None,  // options
    &cxTU);

  delete[] commandLine;

  if (errorCode != CXError_Success) {
    xfatal("clang parse failed: " << cxErrorCodeString(errorCode));
  }

  DisposeCXTranslationUnit disposeTU(cxTU);

  ClangImport importClang(cxIndex, cxTU, elsaParse);
  importClang.importTranslationUnit();
}


// --------------------------- ClangImport -----------------------------
ClangImport::ClangImport(CXIndex cxIndex,
                         CXTranslationUnit cxTranslationUnit,
                         ElsaParse &elsaParse)
  : ElsaASTBuild(elsaParse.m_stringTable,
                 elsaParse.m_typeFactory,
                 elsaParse.m_lang,
                 *this),
    m_cxIndex(cxIndex),
    m_cxTranslationUnit(cxTranslationUnit),
    m_elsaParse(elsaParse),
    m_globalScope(new Scope(SK_GLOBAL, 0 /*changeCount*/, SL_INIT)),
    m_cxFileToName(),
    m_locToVariable()
{}


void ClangImport::importTranslationUnit()
{
  CXCursor cursor = clang_getTranslationUnitCursor(m_cxTranslationUnit);
  if (tracingSys("dumpClang")) {
    printSubtree(cursor, 0);
  }

  std::vector<CXCursor> decls = getChildren(cursor);

  m_elsaParse.m_translationUnit = new TranslationUnit(nullptr);

  for (auto cursor : decls) {
    m_elsaParse.m_translationUnit->topForms.append(importTopForm(cursor));
  }
}


static CXChildVisitResult childCollector(CXCursor cursor,
  CXCursor parent, CXClientData client_data)
{
  std::vector<CXCursor> *children =
    reinterpret_cast<std::vector<CXCursor>*>(client_data);
  children->push_back(cursor);
  return CXChildVisit_Continue;
}


std::vector<CXCursor> ClangImport::getChildren(CXCursor cursor)
{
  std::vector<CXCursor> children;
  clang_visitChildren(cursor, childCollector, &children);
  return children;
}


StringRef ClangImport::importString(CXString cxString)
{
  WrapCXString wcxString(cxString);
  return m_elsaParse.m_stringTable.add(wcxString.c_str());
}


StringRef ClangImport::importFileName(CXFile cxFile)
{
  auto it = m_cxFileToName.find(cxFile);
  if (it == m_cxFileToName.end()) {
    StringRef fname = importString(clang_getFileName(cxFile));
    insertMapUnique(m_cxFileToName, cxFile, fname);
    return fname;
  }
  else {
    return (*it).second;
  }
}


SourceLoc ClangImport::importSourceLocation(CXSourceLocation cxLoc)
{
  // TODO: It would probably be more efficient to use the offset.
  CXFile cxFile;
  unsigned line;
  unsigned column;
  clang_getFileLocation(cxLoc, &cxFile, &line, &column, nullptr /*offset*/);

  if (!cxFile) {
    // This happens for the null location.
    return SL_UNKNOWN;
  }

  StringRef fname = importFileName(cxFile);
  return sourceLocManager->encodeLineCol(fname, line, column);
}


SourceLoc ClangImport::cursorLocation(CXCursor cxCursor)
{
  return importSourceLocation(clang_getCursorLocation(cxCursor));
}


StringRef ClangImport::cursorSpelling(CXCursor cxCursor)
{
  return importString(clang_getCursorSpelling(cxCursor));
}


TopForm *ClangImport::importTopForm(CXCursor cxTopForm)
{
  SourceLoc loc = cursorLocation(cxTopForm);

  CXCursorKind cxKind = clang_getCursorKind(cxTopForm);
  switch (cxKind) {
    default:
      xfailure(stringb("Unknown cxKind: " << cxKind));

    case CXCursor_StructDecl: // 2
    case CXCursor_UnionDecl: // 3
    case CXCursor_ClassDecl: { // 4
      if (clang_isCursorDefinition(cxTopForm)) {
        Declaration *decl = importCompoundTypeDefinition(cxTopForm);
        return new TF_decl(loc, decl);
      }
      else {
        xunimp("struct forward declaration");
      }
    }

    case CXCursor_EnumDecl: { // 5
      if (clang_isCursorDefinition(cxTopForm)) {
        Declaration *decl = importEnumDefinition(cxTopForm);
        return new TF_decl(loc, decl);
      }
      else {
        xunimp("enum forward declaration");
      }
    }

    case CXCursor_FunctionDecl: // 8
    case CXCursor_CXXMethod: { // 21
      if (clang_isCursorDefinition(cxTopForm)) {
        Function *func = importFunctionDefinition(cxTopForm);
        return new TF_func(loc, func);
      }
      else {
        goto importDecl;
      }
    }

    importDecl:
    case CXCursor_VarDecl: // 9
    case CXCursor_TypedefDecl: { // 20
      Declaration *decl = importVarOrTypedefDecl(cxTopForm, DC_TF_DECL);
      return new TF_decl(loc, decl);
    }
  }

  // Not reached.
}


Variable *ClangImport::importEnumTypeAsForward(CXCursor cxEnumDecl)
{
  StringRef enumName = cursorSpelling(cxEnumDecl);

  // Have we already created a record for it?
  Variable *&var = variableRefForDeclaration(cxEnumDecl);
  if (!var) {
    // Conceptually, we would like to make a forward-declared enum, but
    // the Elsa type system cannot explicitly do that.  So we just make
    // a regular EnumType without any values.
    SourceLoc loc = cursorLocation(cxEnumDecl);
    TypeFactory &tfac = m_elsaParse.m_typeFactory;

    EnumType *enumType = tfac.makeEnumType(enumName);
    Type *type = tfac.makeCVAtomicType(enumType, CV_NONE);
    var = enumType->typedefVar =
      makeVariable_setScope(loc, enumName, type, DF_NONE, cxEnumDecl);
  }

  else {
    // Verify the existing type matches.
    xassert(var->name == enumName);
    xassert(var->type->isEnumType());
  }

  return var;
}


Declaration *ClangImport::importEnumDefinition(CXCursor cxEnumDefn)
{
  Variable *enumVar = importEnumTypeAsForward(cxEnumDefn);
  EnumType *enumType = enumVar->type->asNamedAtomicType()->asEnumType();

  // Begin the syntactic list.
  FakeList<Enumerator> *enumerators = FakeList<Enumerator>::emptyList();

  // Get all of the enumerators and add them to 'enumType' and
  // 'enumerators'.
  for (CXCursor const &cxEnumerator : getChildren(cxEnumDefn)) {
    xassert(clang_getCursorKind(cxEnumerator) == CXCursor_EnumConstantDecl);

    // Get the enumerator name and value.
    StringRef enumeratorName = cursorSpelling(cxEnumerator);
    long long llValue = clang_getEnumConstantDeclValue(cxEnumerator);
    int intValue = (int)llValue;
    checkedAssignment(intValue, llValue);

    // Add it to the semantic type.
    SourceLoc enumeratorLoc = cursorLocation(cxEnumerator);
    Variable *enumeratorVar = makeVariable_setScope(
      enumeratorLoc,
      enumeratorName,
      enumVar->type,
      DF_ENUMERATOR,
      cxEnumerator);

    enumType->addValue(enumeratorName, intValue, enumeratorVar);

    // Get the expression that specifies the value, if any.
    Expression * NULLABLE expr = nullptr;
    std::vector<CXCursor> children = getChildren(cxEnumerator);
    if (!children.empty()) {
      xassert(children.size() == 1);
      expr = importExpression(children.front());
    }

    // Build the enumerator syntax.
    Enumerator *enumerator =
      new Enumerator(enumeratorLoc, enumeratorName, expr);
    enumerator->var = enumeratorVar;
    enumerator->enumValue = intValue;
    enumerators = fl_prepend(enumerators, enumerator);
  }

  // The list was built in reverse order due to prepending.
  enumerators = fl_reverse(enumerators);

  TS_enumSpec *enumTS = new TS_enumSpec(
    cursorLocation(cxEnumDefn),
    enumVar->name,
    enumerators);
  enumTS->etype = legacyTypeNC(enumType);

  // Wrap up the type specifier in a declaration with no declarators.
  return new Declaration(DF_NONE, enumTS, nullptr /*decllist*/);
}


Variable *ClangImport::importCompoundTypeAsForward(CXCursor cxCompoundDecl)
{
  StringRef compoundName = cursorSpelling(cxCompoundDecl);

  // What kind of compound is this?
  CompoundType::Keyword keyword;
  switch (clang_getCursorKind(cxCompoundDecl)) {
    default: xfailure("bad kind");
    case CXCursor_StructDecl: keyword = CompoundType::K_STRUCT; break;
    case CXCursor_UnionDecl:  keyword = CompoundType::K_UNION;  break;
    case CXCursor_ClassDecl:  keyword = CompoundType::K_CLASS;  break;
  }

  // Have we already created a record for it?
  Variable *&var = variableRefForDeclaration(cxCompoundDecl);
  if (!var) {
    // Make a forward-declared one.
    SourceLoc loc = cursorLocation(cxCompoundDecl);
    TypeFactory &tfac = m_elsaParse.m_typeFactory;

    // This makes a CompoundType with 'm_isForwardDeclared==true'.
    CompoundType *compoundType = tfac.makeCompoundType(keyword, compoundName);
    Type *type = tfac.makeCVAtomicType(compoundType, CV_NONE);
    var = compoundType->typedefVar =
      makeVariable_setScope(loc, compoundName, type, DF_NONE, cxCompoundDecl);
  }

  else {
    // Verify the existing type matches.
    xassert(var->name == compoundName);
    CompoundType const *ct = var->type->asCompoundTypeC();
    xassert(ct->keyword == keyword);
  }

  return var;
}


Declaration *ClangImport::importCompoundTypeDefinition(CXCursor cxCompoundDefn)
{
  Variable *var = importCompoundTypeAsForward(cxCompoundDefn);
  CompoundType *ct = var->type->asCompoundType();

  // Begin the syntactic lists.
  FakeList<BaseClassSpec> *bases = FakeList<BaseClassSpec>::emptyList();
  MemberList *memberList = new MemberList(nullptr /*list*/);

  // Process the children of the definition.
  for (CXCursor const &child : getChildren(cxCompoundDefn)) {
    CXCursorKind childKind = clang_getCursorKind(child);
    SourceLoc childLoc = cursorLocation(child);

    CVFlags methodCV = CV_NONE;

    Member *newMember = nullptr;
    switch (childKind) {
      default:
        xunimp(stringb("compound defn child kind: " << childKind));

      importDecl:
      case CXCursor_FieldDecl: // 6: non-static data
      case CXCursor_VarDecl: { // 9: static data
        // Associate the Variable we will create with the declaration of
        // the field.
        Variable *&fieldVar = variableRefForDeclaration(child);
        xassert(!fieldVar);

        Type *fieldType = importType_maybeMethod(
          clang_getCursorType(child), ct, methodCV);

        DeclFlags dflags = importStorageClass(
          clang_Cursor_getStorageClass(child));

        fieldVar = makeVariable_setScope(
          cursorLocation(child),
          cursorSpelling(child),
          fieldType,
          dflags,
          child);

        ct->addField(fieldVar);

        newMember = new MR_decl(childLoc,
          makeDeclaration(fieldVar, DC_MR_DECL));
        break;
      }

      case CXCursor_CXXMethod: { // 21
        if (clang_isCursorDefinition(child)) {
          Function *func = importFunctionDefinition(child);
          newMember = new MR_func(childLoc, func);
        }
        else {
          methodCV = importMethodCVFlags(child);
          goto importDecl;
        }
        break;
      }

      case CXCursor_CXXAccessSpecifier: { // 39
        newMember = new MR_access(childLoc,
          importAccessKeyword(clang_getCXXAccessSpecifier(child)));
        break;
      }
    }

    xassert(newMember);
    memberList->list.append(newMember);
  }

  // Now that the members have been populated, 'ct' is no longer forward.
  ct->m_isForwardDeclared = false;

  // Finish making the definition syntax.
  TS_classSpec *specifier = new TS_classSpec(
    cursorLocation(cxCompoundDefn),
    CompoundType::toTypeIntr(ct->keyword),
    makePQName(var),
    bases,
    memberList);
  specifier->ctype = ct;

  // Wrap up the type specifier in a declaration with no declarators.
  return new Declaration(DF_NONE, specifier, nullptr /*decllist*/);
}


CVFlags ClangImport::importMethodCVFlags(CXCursor cxMethodDecl)
{
  // libclang does not expose an 'isVolatile' bit for methods.
  // Fortunately, volatile methods are virtually non-existent in
  // practice.
  return clang_CXXMethod_isConst(cxMethodDecl)? CV_CONST : CV_NONE;
}


Function *ClangImport::importFunctionDefinition(CXCursor cxFuncDefn)
{
  TypeFactory &tfac = m_elsaParse.m_typeFactory;

  // Build a representative for the function.  This will have an
  // associated FunctionType, but without parameter names.
  Variable *funcVar = variableForDeclaration(cxFuncDefn);
  FunctionType const *declFuncType = funcVar->type->asFunctionTypeC();

  // Function body.
  CXCursor cxFunctionBody;

  // Build a FunctionType specific to this definition, containing the
  // parameter names.
  FunctionType *defnFuncType = tfac.makeFunctionType(declFuncType->retType);
  {
    bool foundBody = false;

    // The parameter declarations are children of 'cxFuncDefn'.
    std::vector<CXCursor> children = getChildren(cxFuncDefn);
    for (CXCursor const &child : children) {
      CXCursorKind childKind = clang_getCursorKind(child);
      switch (childKind) {
        case CXCursor_ParmDecl: { // 10
          Variable *paramVar = variableForDeclaration(child, DF_PARAMETER);
          defnFuncType->addParam(paramVar);
          break;
        }

        case CXCursor_TypeRef: { // 43
          // A TypeRef child indicates the receiver object type.
          //
          // TODO: I believe it could also indicate that the return type
          // (or maybe any parameter type?) is named with a typedef.
          Type const *receiverType = importType(clang_getCursorType(child));

          CVFlags cv = importMethodCVFlags(child);

          addReceiver(defnFuncType, cursorLocation(child),
                      receiverType->asCompoundTypeC(), cv);
          break;
        }

        case CXCursor_CompoundStmt: { // 202
          // The final child is the function body.
          xassert(!foundBody);
          foundBody = true;
          cxFunctionBody = child;
          break;
        }

        default:
          xfailure(stringb("Unexpected childKind: " << childKind));
      }
    }

    defnFuncType->flags = declFuncType->flags;

    tfac.doneParams(defnFuncType);
    xassert(foundBody);
  }

  std::pair<TypeSpecifier*, Declarator*> tsAndDeclarator =
    makeTSandDeclaratorForType(funcVar, defnFuncType, DC_FUNCTION);

  S_compound *body = importCompoundStatement(cxFunctionBody);

  return new Function(
    DF_NONE,       // TODO: Refine.
    tsAndDeclarator.first,
    tsAndDeclarator.second,
    nullptr,       // inits
    body,
    nullptr);      // handlers
}


Variable *&ClangImport::variableRefForDeclaration(CXCursor cxDecl)
{
  // Go to the canonical cursor to handle the case of an entity that is
  // declared multiple times.
  cxDecl = clang_getCanonicalCursor(cxDecl);

  // CXCursor is a structure with several elements, so does not appear
  // usable as a map key.  Use the location instead.
  SourceLoc loc = cursorLocation(cxDecl);

  return m_locToVariable[loc];
}


Variable *ClangImport::variableForDeclaration(CXCursor cxDecl,
  DeclFlags declFlags)
{
  StringRef name = cursorSpelling(cxDecl);

  Variable *&var = variableRefForDeclaration(cxDecl);
  if (!var) {
    Type *type;

    CXCursorKind declKind = clang_getCursorKind(cxDecl);
    if (declKind == CXCursor_TypedefDecl) {
      declFlags |= DF_TYPEDEF;

      // For a typedef, the 'type' is the underlying type.
      type = importType(clang_getTypedefDeclUnderlyingType(cxDecl));
    }

    else {
      // For anything else, the type is what the declaration says.
      type = importType(clang_getCursorType(cxDecl));
    }

    SourceLoc loc = cursorLocation(cxDecl);

    xassert(!var);
    var = makeVariable_setScope(loc, name, type, declFlags, cxDecl);
  }

  else {
    // Detect location collisions.
    xassert(var->name == name);
  }

  return var;
}


Variable *ClangImport::existingVariableForDeclaration(CXCursor cxDecl)
{
  Variable *&var = variableRefForDeclaration(cxDecl);
  xassert(var);
  return var;
}


Declaration *ClangImport::importVarOrTypedefDecl(CXCursor cxVarDecl,
  DeclaratorContext context)
{
  Variable *var = variableForDeclaration(cxVarDecl);
  Declaration *decl = makeDeclaration(var, context);

  decl->dflags |= importStorageClass(
    clang_Cursor_getStorageClass(cxVarDecl));

  Declarator *declarator = fl_first(decl->decllist);

  CXCursorKind declKind = clang_getCursorKind(cxVarDecl);
  if (declKind == CXCursor_VarDecl) {
    // Import the initializer, if any.
    std::vector<CXCursor> children = getChildren(cxVarDecl);

    if (children.size() == 2) {
      // The first child is a TypeRef node indicating that the type of
      // the variable was specified with a typedef.
      xassert(clang_getCursorKind(children[0]) == CXCursor_TypeRef);

      // The next is the initializer.
      declarator->init = importInitializer(children[1]);
    }

    else if (children.size() == 1) {
      // The child could be either a TypeRef or an initializer.
      if (clang_getCursorKind(children[0]) == CXCursor_TypeRef) {
        // TypeRef, ignore.
      }
      else {
        declarator->init = importInitializer(children[0]);
      }
    }

    else {
      xassert(children.empty());
    }
  }

  return decl;
}


// Handle the low end of the 'CXTypeKind' range.
static SimpleTypeId cxTypeKindToSimpleTypeId(CXTypeKind kind)
{
  struct Entry {
    // I store the key in the table just so I can assert that it is
    // correct.
    CXTypeKind m_kind;

    // Some of the entries are NUM_SIMPLE_TYPES to indicate that Elsa
    // is missing the corresponding type.
    SimpleTypeId m_id;
  };

  static Entry const table[] = {
    #define ENTRY(kind, id) { kind, id }

    // Columns: \S+ \S+
    ENTRY(CXType_Invalid,    NUM_SIMPLE_TYPES),
    ENTRY(CXType_Unexposed,  NUM_SIMPLE_TYPES),
    ENTRY(CXType_Void,       ST_VOID),
    ENTRY(CXType_Bool,       ST_BOOL),
    ENTRY(CXType_Char_U,     ST_CHAR),   // 'char' for targets where it is unsigned
    ENTRY(CXType_UChar,      ST_UNSIGNED_CHAR),
    ENTRY(CXType_Char16,     NUM_SIMPLE_TYPES),
    ENTRY(CXType_Char32,     NUM_SIMPLE_TYPES),
    ENTRY(CXType_UShort,     ST_UNSIGNED_SHORT_INT),
    ENTRY(CXType_UInt,       ST_UNSIGNED_INT),
    ENTRY(CXType_ULong,      ST_UNSIGNED_LONG_INT),
    ENTRY(CXType_ULongLong,  ST_UNSIGNED_LONG_LONG),
    ENTRY(CXType_UInt128,    NUM_SIMPLE_TYPES),
    ENTRY(CXType_Char_S,     ST_CHAR),   // 'char' for targets where it is signed
    ENTRY(CXType_SChar,      ST_SIGNED_CHAR),
    ENTRY(CXType_WChar,      ST_WCHAR_T),
    ENTRY(CXType_Short,      ST_SHORT_INT),
    ENTRY(CXType_Int,        ST_INT),
    ENTRY(CXType_Long,       ST_LONG_INT),
    ENTRY(CXType_LongLong,   ST_LONG_LONG),
    ENTRY(CXType_Int128,     NUM_SIMPLE_TYPES),
    ENTRY(CXType_Float,      ST_FLOAT),
    ENTRY(CXType_Double,     ST_DOUBLE),
    ENTRY(CXType_LongDouble, ST_LONG_DOUBLE),

    #undef ENTRY
  };

  STATIC_ASSERT(TABLESIZE(table) == CXType_LongDouble+1);
  xassert(kind < TABLESIZE(table));
  Entry const &entry = table[kind];
  if (entry.m_id == NUM_SIMPLE_TYPES) {
    xunimp(stringb("Unhandled simple type kind: " << kind));
  }
  return entry.m_id;
}


Type *ClangImport::importType(CXType cxType)
{
  return importType_maybeMethod(cxType, nullptr /*methodCV*/, CV_NONE);
}


Type *ClangImport::importType_maybeMethod(CXType cxType,
  CompoundType const * NULLABLE methodClass, CVFlags methodCV)
{
  TypeFactory &tfac = m_elsaParse.m_typeFactory;

  CVFlags cv = CV_NONE;
  if (clang_isConstQualifiedType(cxType)) {
    cv |= CV_CONST;
  }
  if (clang_isVolatileQualifiedType(cxType)) {
    cv |= CV_VOLATILE;
  }
  if (clang_isRestrictQualifiedType(cxType)) {
    cv |= CV_RESTRICT;
  }

  // Codes 2 through 23.
  if (CXType_Void <= cxType.kind && cxType.kind <= CXType_LongDouble) {
    return tfac.getSimpleType(cxTypeKindToSimpleTypeId(cxType.kind), cv);
  }

  switch (cxType.kind) {
    default:
      xfailure(stringb("Unknown cxType kind: " << cxType.kind));

    case CXType_Invalid: // 0
      xfailure("importType: cxType is invalid");

    // Codes 2 through 23 are handled above.

    case CXType_Pointer: // 101
      return tfac.makePointerType(cv,
        importType(clang_getPointeeType(cxType)));

    case CXType_LValueReference: // 103
      return tfac.makeReferenceType(
        importType(clang_getPointeeType(cxType)));

    // TODO: CXType_RValueReference = 104

    case CXType_Record: // 105
    case CXType_Enum: // 106
    case CXType_Elaborated: { // 119
      CXCursor cxTypeDecl = clang_getTypeDeclaration(cxType);
      Variable *typedefVar = existingVariableForDeclaration(cxTypeDecl);
      return typedefVar->type;
    }

    case CXType_Typedef: { // 107
      CXCursor cxTypeDecl = clang_getTypeDeclaration(cxType);
      Variable *typedefVar = existingVariableForDeclaration(cxTypeDecl);
      return tfac.makeTypedefType(typedefVar, cv);
    }

    case CXType_FunctionProto: { // 110
      xassert(cv == CV_NONE);
      Type *retType = importType(clang_getResultType(cxType));

      FunctionType *ft = tfac.makeFunctionType(retType);

      if (methodClass) {
        // Note: Even if 'cxType' refers to a method type, libclang does
        // *not* report it as cv-qualified, even though
        // 'clang_getTypeSpelling' will include the 'const'.  Instead,
        // the declaration itself can be 'clang_CXXMethod_isConst'.
        // Consequently, 'methodCV' must be passed in by the caller.
        addReceiver(ft, SL_UNKNOWN, methodClass, methodCV);
      }

      int numParams = clang_getNumArgTypes(cxType);
      for (int i=0; i < numParams; i++) {
        Type *paramType = importType(clang_getArgType(cxType, i));

        // The Clang AST does not appear to record the names of function
        // parameters for non-definitions anywhere other than as tokens
        // that would have to be parsed.
        Variable *paramVar = makeVariable(SL_UNKNOWN, nullptr /*name*/,
          paramType, DF_PARAMETER);

        ft->addParam(paramVar);
      }

      if (clang_isFunctionTypeVariadic(cxType)) {
        ft->setFlag(FF_VARARGS);
      }

      tfac.doneParams(ft);
      return ft;
    }
  }

  // Not reached.
}


void ClangImport::addReceiver(FunctionType *methodType,
  SourceLoc loc, CompoundType const *containingClass, CVFlags cv)
{
  TypeFactory &tfac = m_elsaParse.m_typeFactory;

  Type *qualifiedCT =
    tfac.makeCVAtomicType(legacyTypeNC(containingClass), cv);
  Type *recvRefType = tfac.makeReferenceType(qualifiedCT);
  methodType->addReceiver(makeVariable(
    loc,
    m_elsaParse.m_stringTable.add("__receiver"), // Like Env::receiverName.
    recvRefType,
    DF_PARAMETER));
}


S_compound *ClangImport::importCompoundStatement(CXCursor cxCompStmt)
{
  SourceLoc loc = cursorLocation(cxCompStmt);
  S_compound *comp = new S_compound(loc, nullptr /*stmts*/);

  std::vector<CXCursor> children = getChildren(cxCompStmt);
  for (CXCursor const &child : children) {
    comp->stmts.append(importStatement(child));
  }

  return comp;
}


Statement *ClangImport::importStatement(CXCursor cxStmt)
{
  if (clang_Cursor_isNull(cxStmt)) {
    // This is for a case like a missing initializer in a 'for'
    // statement.
    return new S_skip(SL_UNKNOWN, MI_IMPLICIT);
  }

  SourceLoc loc = cursorLocation(cxStmt);
  CXCursorKind stmtKind = clang_getCursorKind(cxStmt);
  std::vector<CXCursor> children = getChildren(cxStmt);

  // Codes 100 through 199.
  if (CXCursor_FirstExpr <= stmtKind && stmtKind < CXCursor_FirstStmt) {
    FullExpression *fullExpr = importFullExpression(cxStmt);
    return new S_expr(loc, fullExpr);
  }

  switch (stmtKind) {
    default:
      xunimp(stringb("Unhandled stmtKind: " << stmtKind));

    // Codes 100 through 199 handled above.

    case CXCursor_LabelStmt: { // 201
      StringRef name = importString(clang_getCursorSpelling(cxStmt));
      return new S_label(loc, name, importStatement(getOnlyChild(cxStmt)));
    }

    case CXCursor_CompoundStmt: // 202
      return importCompoundStatement(cxStmt);

    case CXCursor_CaseStmt: { // 203
      CXCursor caseValue, childStmt;
      getTwoChildren(caseValue, childStmt, cxStmt);

      S_case *ret = new S_case(loc, importExpression(caseValue),
        importStatement(childStmt));
      checkedAssignment(ret->labelVal, evalAsLongLong(caseValue));
      return ret;
    }

    case CXCursor_DefaultStmt: // 204
      return new S_default(loc, importStatement(getOnlyChild(cxStmt)));

    case CXCursor_IfStmt: { // 205
      std::vector<CXCursor> children = getChildren(cxStmt);

      Statement *elseStmt;
      if (children.size() == 2) {
        elseStmt = new S_skip(loc, MI_IMPLICIT);
      }
      else {
        xassert(children.size() == 3);
        elseStmt = importStatement(children[2]);
      }

      return new S_if(loc,
        importCondition(children[0]),
        importStatement(children[1]),
        elseStmt);
    }

    case CXCursor_SwitchStmt: { // 206
      CXCursor selectorExpr, childStmt;
      getTwoChildren(selectorExpr, childStmt, cxStmt);

      return new S_switch(loc,
        importCondition(selectorExpr),
        importStatement(childStmt));
    }

    case CXCursor_WhileStmt: { // 207
      std::vector<CXCursor> children = getChildren(cxStmt);

      // If two children, they are [cond,body].  If three, the first is
      // a variable declaration, the second is an expression using the
      // declared variable that acts as the loop condition, and the
      // third is the body.
      xassert(children.size() == 2 || children.size() == 3);

      // Elsa does not currently have any way to represent the implicit
      // conversion that can appear in the condition when there are
      // three children, so I'll just ignore it.
      CXCursor conditionExpr = children.front();
      CXCursor childStmt = children.back();

      return new S_while(loc,
        importCondition(conditionExpr),
        importStatement(childStmt));
    }

    case CXCursor_DoStmt: { // 208
      CXCursor childStmt, conditionExpr;
      getTwoChildren(childStmt, conditionExpr, cxStmt);

      return new S_doWhile(loc,
        importStatement(childStmt),
        importFullExpression(conditionExpr));
    }

    case CXCursor_ForStmt: { // 209
      return new S_for(loc,
        importStatement(     clang_forStmtElement(cxStmt, CXForStmtElement_init)),
        importCondition(     clang_forStmtElement(cxStmt, CXForStmtElement_cond)),
        importFullExpression(clang_forStmtElement(cxStmt, CXForStmtElement_inc )),
        importStatement(     clang_forStmtElement(cxStmt, CXForStmtElement_body))
      );
    }

    case CXCursor_GotoStmt: { // 210
      // The 'goto' itself does not carry the label, but we can get the
      // referenced statement, and that has it.
      CXCursor label = clang_getCursorReferenced(cxStmt);
      xassert(clang_getCursorKind(label) == CXCursor_LabelStmt);
      StringRef name = importString(clang_getCursorSpelling(label));
      return new S_goto(loc, name);
    }

    // TODO: CXCursor_IndirectGotoStmt: // 211

    case CXCursor_ContinueStmt: // 212
      return new S_continue(loc);

    case CXCursor_BreakStmt: // 213
      return new S_break(loc);

    case CXCursor_ReturnStmt: { // 214
      FullExpression *retval = nullptr;
      if (!children.empty()) {
        xassert(children.size() == 1);
        retval = importFullExpression(children.front());
      }
      return new S_return(loc, retval);
    }

    case CXCursor_CXXTryStmt: { // 224
      // The first child is the body, and the rest are handlers.
      std::vector<CXCursor> children = getChildren(cxStmt);
      xassert(children.size() >= 2);

      Statement *body = importStatement(children.front());

      FakeList<Handler> *handlers = FakeList<Handler>::emptyList();
      for (unsigned i=1; i < children.size(); i++) {
        handlers = fl_prepend(handlers, importHandler(children[i]));
      }
      handlers = fl_reverse(handlers);

      return new S_try(loc, body, handlers);
    }

    case CXCursor_NullStmt: // 230
      return new S_skip(loc, MI_EXPLICIT);

    case CXCursor_DeclStmt: { // 231
      xassert(children.size() == 1);
      for (CXCursor const &child : children) {
        xassert(clang_getCursorKind(child) == CXCursor_VarDecl);
        Declaration *decl = importVarOrTypedefDecl(child, DC_S_DECL);
        return new S_decl(loc, decl);
      }
    }
  }

  // Not reached.
  return NULL;
}


FullExpression *ClangImport::importFullExpression(CXCursor cxExpr)
{
  return new FullExpression(importExpression(cxExpr));
}


CXCursor ClangImport::getOnlyChild(CXCursor cxNode)
{
  std::vector<CXCursor> children = getChildren(cxNode);
  xassert(children.size() == 1);
  return children.front();
}


void ClangImport::getTwoChildren(CXCursor &c1, CXCursor &c2,
  CXCursor cxNode)
{
  std::vector<CXCursor> children = getChildren(cxNode);
  xassert(children.size() == 2);
  c1 = children[0];
  c2 = children[1];
}


static UnaryOp cxUnaryToElsaUnary(CXUnaryOperator op)
{
  switch (op) {
    default: xfailure("bad code");
    case CXUnaryOperator_Plus:  return UNY_PLUS;
    case CXUnaryOperator_Minus: return UNY_MINUS;
    case CXUnaryOperator_Not:   return UNY_BITNOT;
    case CXUnaryOperator_LNot:  return UNY_NOT;
  }
}


static EffectOp cxUnaryToElsaEffect(CXUnaryOperator op)
{
  switch (op) {
    default: xfailure("bad code");
    case CXUnaryOperator_PostInc: return EFF_POSTINC;
    case CXUnaryOperator_PostDec: return EFF_POSTDEC;
    case CXUnaryOperator_PreInc:  return EFF_PREINC;
    case CXUnaryOperator_PreDec:  return EFF_PREDEC;
  }
}


static BinaryOp cxBinaryToElsaBinary(CXBinaryOperator op)
{
  switch (op) {
    default: xfailure("bad code");

    // Columns: case return
    case CXBinaryOperator_PtrMemD:   return BIN_DOT_STAR;
    case CXBinaryOperator_PtrMemI:   return BIN_ARROW_STAR;
    case CXBinaryOperator_Mul:       return BIN_MULT;
    case CXBinaryOperator_Div:       return BIN_DIV;
    case CXBinaryOperator_Rem:       return BIN_MOD;
    case CXBinaryOperator_Add:       return BIN_PLUS;
    case CXBinaryOperator_Sub:       return BIN_MINUS;
    case CXBinaryOperator_Shl:       return BIN_LSHIFT;
    case CXBinaryOperator_Shr:       return BIN_RSHIFT;
    case CXBinaryOperator_Cmp:       xunimp("spaceship");
    case CXBinaryOperator_LT:        return BIN_LESS;
    case CXBinaryOperator_GT:        return BIN_GREATER;
    case CXBinaryOperator_LE:        return BIN_LESSEQ;
    case CXBinaryOperator_GE:        return BIN_GREATEREQ;
    case CXBinaryOperator_EQ:        return BIN_EQUAL;
    case CXBinaryOperator_NE:        return BIN_NOTEQUAL;
    case CXBinaryOperator_And:       return BIN_BITAND;
    case CXBinaryOperator_Xor:       return BIN_BITXOR;
    case CXBinaryOperator_Or:        return BIN_BITOR;
    case CXBinaryOperator_LAnd:      return BIN_AND;
    case CXBinaryOperator_LOr:       return BIN_OR;
    case CXBinaryOperator_Assign:    return BIN_ASSIGN;
    case CXBinaryOperator_MulAssign: return BIN_MULT;
    case CXBinaryOperator_DivAssign: return BIN_DIV;
    case CXBinaryOperator_RemAssign: return BIN_MOD;
    case CXBinaryOperator_AddAssign: return BIN_PLUS;
    case CXBinaryOperator_SubAssign: return BIN_MINUS;
    case CXBinaryOperator_ShlAssign: return BIN_LSHIFT;
    case CXBinaryOperator_ShrAssign: return BIN_RSHIFT;
    case CXBinaryOperator_AndAssign: return BIN_BITAND;
    case CXBinaryOperator_XorAssign: return BIN_BITXOR;
    case CXBinaryOperator_OrAssign:  return BIN_BITOR;
    case CXBinaryOperator_Comma:     return BIN_COMMA;
  }
}


long long ClangImport::evalAsLongLong(CXCursor cxExpr)
{
  CXEvalResult cxEvalResult = clang_Cursor_Evaluate(cxExpr);
  xassert(clang_EvalResult_getKind(cxEvalResult) == CXEval_Int);
  long long value = clang_EvalResult_getAsLongLong(cxEvalResult);
  clang_EvalResult_dispose(cxEvalResult);

  return value;
}


Expression *ClangImport::importExpression(CXCursor cxExpr)
{
  if (clang_Cursor_isNull(cxExpr)) {
    // This is used for a missing 'for' condition or increment.
    return new E_boolLit(true);
  }

  CXCursorKind exprKind = clang_getCursorKind(cxExpr);
  Type const *type = importType(clang_getCursorType(cxExpr));

  Expression *ret = nullptr;

  switch (exprKind) {
    default:
      xunimp(stringb("Unhandled exprKind: " << exprKind));

    case CXCursor_UnexposedExpr: { // 100
      // This is used for implicit conversions (perhaps among other
      // things).
      CXCursor child = getOnlyChild(cxExpr);
      Type const *childType = importType(clang_getCursorType(child));
      if (type->equals(childType)) {
        // This happens for lvalue-to-rvalue conversions.  There does
        // not seem to be a way in libclang to see that directly, so I
        // will just drop the conversion.
        return importExpression(child);
      }
      else {
        StandardConversion conv =
          describeAsStandardConversion(type, childType);
        ret = new E_implicitStandardConversion(conv,
          importExpression(child));
      }
      break;
    }

    case CXCursor_DeclRefExpr: { // 101
      CXCursor decl = clang_getCursorReferenced(cxExpr);
      Variable *var = existingVariableForDeclaration(decl);
      ret = makeE_variable(var);
      break;
    }

    case CXCursor_MemberRefExpr: { // 102
      Expression *object = importExpression(getOnlyChild(cxExpr));

      CXCursor decl = clang_getCursorReferenced(cxExpr);
      Variable *field = existingVariableForDeclaration(decl);

      E_fieldAcc *acc = new E_fieldAcc(object, makePQName(field));
      acc->field = field;
      ret = acc;
      break;
    }

    case CXCursor_CallExpr: { // 103
      std::vector<CXCursor> children = getChildren(cxExpr);
      xassert(!children.empty());

      // The first child is the callee expression.
      Expression *callee = importExpression(children.front());

      // Remaining children are the arguments.
      FakeList<ArgExpression> *args = FakeList<ArgExpression>::emptyList();
      for (size_t i=1; i < children.size(); ++i) {
        args = fl_prepend(args, new ArgExpression(
          importExpression(children[i])));
      }
      args = fl_reverse(args);

      ret = new E_funCall(callee, args);
      break;
    }

    case CXCursor_IntegerLiteral: { // 106
      int value;
      checkedAssignment(value, evalAsLongLong(cxExpr));
      E_intLit *intLit = new E_intLit(
        m_elsaParse.m_stringTable.add(stringbc(value)));
      intLit->i = value;
      ret = intLit;
      break;
    }

    case CXCursor_ParenExpr: { // 111
      // Elsa drops parentheses, so I will do that here as well.
      ret = importExpression(getOnlyChild(cxExpr));
      break;
    }

    case CXCursor_UnaryOperator: { // 112
      CXCursor childCursor = getOnlyChild(cxExpr);
      Expression *childExpr = importExpression(childCursor);

      CXUnaryOperator cxCode = clang_unaryOperator_operator(cxExpr);
      switch (cxCode) {
        default:
          xunimp(stringb("Unary operator: " << cxCode));

        case CXUnaryOperator_PostInc:
        case CXUnaryOperator_PostDec:
        case CXUnaryOperator_PreInc:
        case CXUnaryOperator_PreDec: {
          EffectOp effOp = cxUnaryToElsaEffect(cxCode);
          ret = new E_effect(effOp, childExpr);
          break;
        }

        case CXUnaryOperator_AddrOf:
          ret = new E_addrOf(childExpr);
          break;

        case CXUnaryOperator_Deref:
          ret = new E_deref(childExpr);
          break;

        case CXUnaryOperator_Plus:
        case CXUnaryOperator_Minus:
        case CXUnaryOperator_Not:
        case CXUnaryOperator_LNot: {
          UnaryOp unOp = cxUnaryToElsaUnary(cxCode);
          ret = new E_unary(unOp, childExpr);
          break;
        }
      }
      break;
    }

    case CXCursor_BinaryOperator: { // 114
      CXCursor cxLHS, cxRHS;
      getTwoChildren(cxLHS, cxRHS, cxExpr);

      CXBinaryOperator cxCode = clang_binaryOperator_operator(cxExpr);
      BinaryOp binOp = cxBinaryToElsaBinary(cxCode);
      if (CXBinaryOperator_Assign <= cxCode &&
                                     cxCode <= CXBinaryOperator_OrAssign) {
        ret = new E_assign(
          importExpression(cxLHS),
          binOp,
          importExpression(cxRHS)
        );
      }
      else {
        ret = new E_binary(
          importExpression(cxLHS),
          binOp,
          importExpression(cxRHS)
        );
      }
      break;
    }

    case CXCursor_CStyleCastExpr: { // 117
      // The destination type is simply the type of this expression.
      ASTTypeId *castType =
        makeASTTypeId(type, nullptr /*name*/, DC_E_CAST);

      // Get the expression under the cast.
      CXCursor underExpr;
      {
        std::vector<CXCursor> children = getChildren(cxExpr);
        if (children.size() == 1) {
          // The only child is the underlying expression.
          underExpr = children[0];
        }
        else {
          xassert(children.size() == 2);

          // The first child is a TypeRef.  This happens when the type is
          // a typedef.  But I don't need to pay attention to it, since
          // the 'type' already names the typedef.
          CXCursor typeRef = children[0];
          xassert(clang_getCursorKind(typeRef) == CXCursor_TypeRef);

          underExpr = children[1];
        }
      }

      // Based on -ast-dump output, it seems that a typical C-style cast
      // has CStyleCastExpr on top of one or more ImplicitCastExpr nodes
      // (which themselves are conveyed as "unexposed" in libclang).  I
      // don't think the implicit nodes help me, so I'll skip them.
      while (clang_getCursorKind(underExpr) == CXCursor_UnexposedExpr) {
        underExpr = getOnlyChild(underExpr);
      }

      ret = new E_cast(castType, importExpression(underExpr));
      break;
    }

    case CXCursor_CXXThrowExpr: { // 133
      Expression *expr = nullptr;

      std::vector<CXCursor> children = getChildren(cxExpr);
      if (children.size() == 1) {
        expr = importExpression(children.front());
      }
      else {
        xassert(children.empty());
      }

      ret = new E_throw(expr);
      break;
    }
  }

  ret->type = legacyTypeNC(type);
  return ret;
}


StandardConversion ClangImport::describeAsStandardConversion(
  Type const *destType, Type const *srcType)
{
  CVAtomicType const *destCVAT = destType->ifCVAtomicTypeC();
  CVAtomicType const *srcCVAT  = srcType ->ifCVAtomicTypeC();
  if (destCVAT && srcCVAT) {
    SimpleType const *destSimpleType = destCVAT->atomic->ifSimpleTypeC();
    SimpleType const *srcSimpleType  = srcCVAT ->atomic->ifSimpleTypeC();
    if (destSimpleType && srcSimpleType) {
      if (isIntegerType(destSimpleType->type) &&
          isIntegerType(srcSimpleType->type)) {
        // TODO: This could also be SC_INT_PROM.
        return SC_INT_CONV;
      }
    }
  }

  if (destType->isPointerType()) {
    if (srcType->isFunctionType()) {
      return SC_FUNC_TO_PTR;
    }
    if (srcType->isArrayType()) {
      return SC_ARRAY_TO_PTR;
    }
  }

  xunimp(stringb(
    "describeAsStandardConversion: destType='" << destType->toString() <<
    "' srcType='" << srcType->toString() << "'"));
  return SC_IDENTITY;
}


Condition *ClangImport::importCondition(CXCursor cxCond)
{
  CXCursorKind kind = clang_getCursorKind(cxCond);

  if (kind == CXCursor_VarDecl) {
    return new CN_decl(importASTTypeId(cxCond, DC_CN_DECL));
  }
  else {
    return new CN_expr(importFullExpression(cxCond));
  }
}


ASTTypeId *ClangImport::importASTTypeId(CXCursor cxDecl,
  DeclaratorContext context)
{
  xassert(clang_getCursorKind(cxDecl) == CXCursor_VarDecl);

  Variable *var = variableForDeclaration(cxDecl);
  ASTTypeId *typeId =
    makeASTTypeId(var->type, makePQName(var), context);

  if (context == DC_HANDLER) {
    // There is no initializer for a handler parameter, but Clang will
    // have a child here if the type is a typedef.  Skip it.
    //
    // TODO: That's certainly going to cause problems elsewhere.
  }
  else {
    std::vector<CXCursor> children = getChildren(cxDecl);
    if (children.size() == 1) {
      typeId->decl->init = importInitializer(children.front());
    }
    else {
      xassert(children.empty());
    }
  }

  return typeId;
}


Initializer *ClangImport::importInitializer(CXCursor cxInit)
{
  if (clang_getCursorKind(cxInit) == CXCursor_UnexposedExpr &&
      getChildren(cxInit).empty()) {
    // This is an 'ImplicitValueInitExpr', which in Elsa is represented
    // with a NULL initialzer.
    return nullptr;
  }

  if (clang_getCursorKind(cxInit) == CXCursor_CallExpr &&
      getChildren(cxInit).empty()) {
    // This is a 'CXXConstructExpr', which in Elsa is also represented
    // with a NULL initialzer.
    return nullptr;
  }

  // TODO: Handle IN_compound cases.
  return new IN_expr(cursorLocation(cxInit), importExpression(cxInit));
}


Handler *ClangImport::importHandler(CXCursor cxHandler)
{
  xassert(clang_getCursorKind(cxHandler) == CXCursor_CXXCatchStmt);

  std::vector<CXCursor> children = getChildren(cxHandler);
  xassert(children.size() == 1 || children.size() == 2);

  ASTTypeId *typeId;
  if (children.size() == 1) {
    typeId = Handler::makeEllipsisTypeId(cursorLocation(cxHandler));

    // Fill in the declarator 'type' and 'var' fields similar to how the
    // Elsa type checker does, in order to pacify the integrity checker.
    typeId->decl->type = m_elsaParse.m_typeFactory.getSimpleType(ST_ELLIPSIS);
    typeId->decl->var = makeVariable(SL_UNKNOWN, nullptr /*name*/,
      typeId->decl->type, DF_NONE);
  }
  else {
    typeId = importASTTypeId(children.front(), DC_HANDLER);
  }

  Statement *body = importStatement(children.back());

  return new Handler(typeId, body);
}


Variable *ClangImport::makeVariable_setScope(SourceLoc loc,
  StringRef name, Type *type, DeclFlags flags, CXCursor cxDecl)
{
  Variable *var = makeVariable(loc, name, type, flags);

  CXCursor cxSemParent = clang_getCursorSemanticParent(cxDecl);
  if (clang_getCursorKind(cxSemParent) == CXCursor_TranslationUnit) {
    var->m_containingScope = m_globalScope;
  }

  return var;
}


Variable *ClangImport::makeVariable(SourceLoc loc, StringRef name,
  Type *type, DeclFlags flags)
{
  return m_elsaParse.m_typeFactory.makeVariable(loc, name, type, flags);
}


DeclFlags ClangImport::importStorageClass(CX_StorageClass storageClass)
{
  switch (storageClass) {
    case CX_SC_Extern:   return DF_EXTERN;
    case CX_SC_Static:   return DF_STATIC;
    case CX_SC_Auto:     return DF_AUTO;
    case CX_SC_Register: return DF_REGISTER;
    default:             return DF_NONE;
  }
}


static char const *toString(CX_StorageClass storageClass)
{
  struct Entry {
    CX_StorageClass m_storageClass;
    char const *m_name;
  };

  static Entry const table[] = {
    #define ENTRY(name) { CX_SC_##name, #name }
    ENTRY(Invalid),
    ENTRY(None),
    ENTRY(Extern),
    ENTRY(Static),
    ENTRY(PrivateExtern),
    ENTRY(OpenCLWorkGroupLocal),
    ENTRY(Auto),
    ENTRY(Register),
    #undef ENTRY
  };

  for (Entry const &e : table) {
    if (e.m_storageClass == storageClass) {
      return e.m_name;
    }
  }

  return "(invalid storage class)";
}


AccessKeyword ClangImport::importAccessKeyword(
  CX_CXXAccessSpecifier accessSpecifier)
{
  switch (accessSpecifier) {
    case CX_CXXPublic:    return AK_PUBLIC;
    case CX_CXXProtected: return AK_PROTECTED;
    case CX_CXXPrivate:   return AK_PRIVATE;
    default:              return AK_UNSPECIFIED;
  }
}


void ClangImport::maybePrintType(char const *label, CXType cxType)
{
  if (cxType.kind != CXType_Invalid) {
    cout << " " << label << "=\""
         << importString(clang_getTypeSpelling(cxType)) << "\"";
  }
}


void ClangImport::printSubtree(CXCursor cursor, int indent)
{
  StringRef spelling = cursorSpelling(cursor);
  StringRef display = importString(clang_getCursorDisplayName(cursor));

  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  ind(cout, indent)
    << "kind=" << toString(cursorKind)
    << "(" << cursorKindClassificationsString(cursorKind) << ")"
    << " loc=" << toLCString(cursorLocation(cursor))
    << " spelling=\"" << spelling << "\"";

  if (spelling != display) {
    cout << " display=\"" << display << "\"";
  }

  maybePrintType("type", clang_getCursorType(cursor));

  if (clang_isExpression(cursorKind)) {
    // 'getReceiverType' crashes if passed a non-expression.
    maybePrintType("receiverType", clang_Cursor_getReceiverType(cursor));
  }

  CXCursor decl = clang_getCursorReferenced(cursor);
  if (!clang_Cursor_isNull(decl)) {
    cout << " references=" << toLCString(cursorLocation(decl));
  }

  CX_StorageClass storageClass = clang_Cursor_getStorageClass(cursor);
  if (storageClass != CX_SC_Invalid && storageClass != CX_SC_None) {
    cout << " storage=" << toString(storageClass);
  }

  cout << "\n";

  for (CXCursor const &child : getChildren(cursor)) {
    printSubtree(child, indent+2);
  }
}


// EOF