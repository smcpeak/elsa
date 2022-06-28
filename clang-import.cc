// clang-import.cc
// Code for clang-import.h.

#include "clang-import-internal.h"     // this module

// elsa
#include "clang-print.h"               // toString(CXCursorKind), etc.
#include "clang-additions.h"           // clang_getUnaryExpressionOperator, etc.

// smbase
#include "codepoint.h"                 // isASCIIDigit, isCIdentifierCharacter
#include "exc.h"                       // xfatal
#include "map-utils.h"                 // insertMapUnique
#include "parsestring.h"               // ParseString
#include "str.h"                       // string
#include "trace.h"                     // tracingSys

// libc++
#include <sstream>                     // std::ostringstream

// libc
#include <stdlib.h>                    // atoi, getenv


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

  CXTranslationUnit_Flags tuFlags = CXTranslationUnit_None;
  if (0) {
    // This causes the TU to include things like MacroDefinition nodes,
    // but it does not make the results of clang_tokenize any more
    // accurate.
    tuFlags = CXTranslationUnit_DetailedPreprocessingRecord;
  }

  CXTranslationUnit cxTU;
  CXErrorCode errorCode = clang_parseTranslationUnit2(
    cxIndex,
    nullptr,                 // source_filename
    commandLine,
    (int)gccOptions.size(),
    nullptr,                 // unsaved_files
    0,                       // num_unsaved_files
    tuFlags,                 // options
    &cxTU);

  delete[] commandLine;

  if (errorCode != CXError_Success) {
    xfatal("clang parse failed: " << cxErrorCodeString(errorCode));
  }

  DisposeCXTranslationUnit disposeTU(cxTU);

  // Check for errors.
  unsigned numDiagnostics = clang_getNumDiagnostics(cxTU);
  for (unsigned i=0; i < numDiagnostics; i++) {
    CXDiagnostic diag = clang_getDiagnostic(cxTU, i);
    CXDiagnosticSeverity sev = clang_getDiagnosticSeverity(diag);
    clang_disposeDiagnostic(diag);

    if (sev >= CXDiagnostic_Error) {
      // The diagnostics should already be printed.
      xfatal("clang parse produced error diagnostics");
    }
  }

  ClangImport importClang(cxIndex, cxTU, elsaParse);
  importClang.importTranslationUnit();
}


// ----------------------- PartitionedChildren -------------------------
static CXChildVisitResult partitionedChildCollector(CXCursor cursor,
  CXCursor parent, CXClientData client_data)
{
  PartitionedChildren *children =
    static_cast<PartitionedChildren*>(client_data);

  CXCursorKind kind = clang_getCursorKind(cursor);
  if (clang_isDeclaration(kind)) {
    children->m_declarations.push_back(cursor);
  }
  else if (clang_isExpression(kind)) {
    children->m_expressions.push_back(cursor);
  }
  else if (clang_isStatement(kind)) {
    children->m_statements.push_back(cursor);
  }
  else if (clang_isAttribute(kind)) {
    children->m_attributes.push_back(cursor);
  }
  else if (kind == CXCursor_TypeRef) {
    // These are entirely useless, so I don't even bother to save them.
  }
  else {
    xunimp(stringb("partitionedChildCollector: kind: " << toString(kind)));
  }

  return CXChildVisit_Continue;
}


PartitionedChildren::PartitionedChildren(CXCursor cursor)
  : m_declarations(),
    m_expressions(),
    m_statements(),
    m_attributes()
{
  clang_visitChildren(cursor, partitionedChildCollector, this);
}


bool PartitionedChildren::empty() const
{
  return m_declarations.empty() &&
         m_expressions.empty() &&
         m_statements.empty() &&
         m_attributes.empty();
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
    m_tfac(elsaParse.m_typeFactory),
    m_globalScope(new Scope(SK_GLOBAL, 0 /*changeCount*/, SL_INIT)),
    m_cxFileToName(),
    m_locToVariable()
{}


void ClangImport::importTranslationUnit()
{
  CXCursor cursor = clang_getTranslationUnitCursor(m_cxTranslationUnit);
  if (tracingSys("dumpClang")) {
    printSubtree(cursor);
  }

  std::vector<CXCursor> decls = getChildren(cursor);

  m_elsaParse.m_translationUnit = new TranslationUnit(nullptr);

  for (auto cursor : decls) {
    m_elsaParse.m_translationUnit->topForms.append(importTopForm(cursor));
  }
}


StringRef ClangImport::importString(CXString cxString)
{
  WrapCXString wcxString(cxString);
  return addStringRef(wcxString.c_str());
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
      xfailure(stringb("importTopForm: Unknown cxKind: " << toString(cxKind)));

    importDecl:
    case CXCursor_StructDecl: // 2
    case CXCursor_UnionDecl: // 3
    case CXCursor_ClassDecl: // 4
    case CXCursor_EnumDecl: // 5
    case CXCursor_VarDecl: // 9
    case CXCursor_TypedefDecl: // 20
      return new TF_decl(loc, importDeclaration(cxTopForm, DC_TF_DECL));

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
  }

  // Not reached.
}


Declaration *ClangImport::importDeclaration(CXCursor cxDecl,
  DeclaratorContext context)
{
  CXCursorKind cxKind = clang_getCursorKind(cxDecl);
  switch (cxKind) {
    default:
      xfailure(stringb("importDeclaration: Unknown cxKind: " << toString(cxKind)));

    case CXCursor_StructDecl: // 2
    case CXCursor_UnionDecl: // 3
    case CXCursor_ClassDecl: // 4
      if (clang_isCursorDefinition(cxDecl)) {
        return importCompoundTypeDefinition(cxDecl);
      }
      else {
        return importCompoundTypeForwardDeclaration(cxDecl);
      }

    case CXCursor_EnumDecl: // 5
      if (clang_isCursorDefinition(cxDecl)) {
        return importEnumDefinition(cxDecl);
      }
      else {
        printSubtree(cxDecl);
        xunimp("enum forward declaration");
      }

    case CXCursor_FunctionDecl: // 8
    case CXCursor_CXXMethod: // 21
      // This is only meant for use with non-definition declarations.
      xassert(!clang_isCursorDefinition(cxDecl));
      // Fallthrough.

    case CXCursor_VarDecl: // 9
    case CXCursor_TypedefDecl: // 20
      return importVarOrTypedefDecl(cxDecl, context);
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

    EnumType *enumType = m_tfac.makeEnumType(enumName);
    Type *type = m_tfac.makeCVAtomicType(enumType, CV_NONE);
    var = enumType->typedefVar =
      makeVariable_setScope(loc, enumName, type, DF_TYPEDEF, cxEnumDecl);
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
    SourceLoc enumeratorLoc = cursorLocation(cxEnumerator);

    // Get the enumerator name and value.
    StringRef enumeratorName = cursorSpelling(cxEnumerator);
    long long llValue = clang_getEnumConstantDeclValue(cxEnumerator);
    int intValue = (int)llValue;
    checkedAssignment(intValue, llValue);

    // Create a Variable to represent the enumerator, associated with
    // its declaration.
    Variable *&enumeratorVar = variableRefForDeclaration(cxEnumerator);
    xassert(!enumeratorVar);
    enumeratorVar = makeVariable_setScope(
      enumeratorLoc,
      enumeratorName,
      enumVar->type,
      DF_ENUMERATOR,
      cxEnumerator);

    // Add it to the semantic type.
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

    // This makes a CompoundType with 'm_isForwardDeclared==true'.
    CompoundType *compoundType = m_tfac.makeCompoundType(keyword, compoundName);
    Type *type = m_tfac.makeCVAtomicType(compoundType, CV_NONE);
    var = compoundType->typedefVar =
      makeVariable_setScope(loc, compoundName, type, DF_TYPEDEF, cxCompoundDecl);
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
        printSubtree(child);
        xunimp(stringb("compound defn child kind: " << toString(childKind)));

      case CXCursor_StructDecl: // 2
      case CXCursor_UnionDecl: // 3
      case CXCursor_ClassDecl: // 4
      case CXCursor_EnumDecl: // 5
        newMember = new MR_decl(childLoc,
          importDeclaration(child, DC_MR_DECL));
        break;

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


Declaration *ClangImport::importCompoundTypeForwardDeclaration(
  CXCursor cxCompoundDecl)
{
  Variable *var = importCompoundTypeAsForward(cxCompoundDecl);
  CompoundType *ct = var->type->asCompoundType();

  TS_elaborated *specifier = new TS_elaborated(
    cursorLocation(cxCompoundDecl),
    CompoundType::toTypeIntr(ct->keyword),
    makePQName(var));
  specifier->atype = ct;

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
  // Build a representative for the function.  This will have an
  // associated FunctionType, but without parameter names.
  Variable *funcVar = variableForDeclaration(cxFuncDefn);
  FunctionType const *declFuncType = funcVar->type->asFunctionTypeC();

  // Function body.
  CXCursor cxFunctionBody;

  // Build a FunctionType specific to this definition, containing the
  // parameter names.
  FunctionType *defnFuncType = m_tfac.makeFunctionType(declFuncType->retType);

  // Possibly add the receiver parameter.
  if (clang_getCursorKind(cxFuncDefn) == CXCursor_CXXMethod &&
      clang_Cursor_getStorageClass(cxFuncDefn) != CX_SC_Static) {
    // The containing class is the semantic parent.
    CXCursor semParent = clang_getCursorSemanticParent(cxFuncDefn);
    Variable *classTypedefVar = existingVariableForDeclaration(semParent);
    CVFlags cv = importMethodCVFlags(cxFuncDefn);
    addReceiver(defnFuncType, cursorLocation(cxFuncDefn),
                classTypedefVar->type->asCompoundTypeC(), cv);
  }

  // Scan for the parameter declarations and the body.
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

        case CXCursor_TypeRef: // 43
          // A TypeRef child can indicate the receiver object type, but
          // it can also indicate the type of a return value or a
          // parameter that is not a basic type like 'int', and there's
          // no indication which is which, so these are entirely useless
          // here.  Just skip them.
          break;

        case CXCursor_CompoundStmt: { // 202
          // The final child is the function body.
          xassert(!foundBody);
          foundBody = true;
          cxFunctionBody = child;
          break;
        }

        default:
          xfailure(stringb("importFunctionDefinition: Unexpected childKind: " <<
                           toString(childKind)));
      }
    }

    defnFuncType->flags = declFuncType->flags;

    m_tfac.doneParams(defnFuncType);
    xassert(foundBody);
  }

  std::pair<TypeSpecifier*, Declarator*> tsAndDeclarator =
    makeTSandDeclaratorForType(funcVar, defnFuncType, DC_FUNCTION);

  DeclFlags declFlags = possiblyAddNameQualifiers_and_getStorageClass(
    tsAndDeclarator.second, cxFuncDefn);

  S_compound *body = importCompoundStatement(cxFunctionBody);

  return new Function(
    declFlags,
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
  CXCursorKind declKind = clang_getCursorKind(cxDecl);

  Variable *&var = variableRefForDeclaration(cxDecl);
  if (!var) {
    Type *type;

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

    if (declKind == CXCursor_FunctionDecl) {
      FunctionType const *ft = var->type->asFunctionTypeC();
      if (ft->hasFlag(FF_NO_PARAM_INFO)) {
        // This is another declaration of a function for which we, so
        // far, do not have parameter info.  Import the type at this
        // location to see if it has parameter info.
        FunctionType const *ft2 =
          importType(clang_getCursorType(cxDecl))->asFunctionTypeC();
        if (!ft2->hasFlag(FF_NO_PARAM_INFO)) {
          // Update the Variable accordingly.
          var->type = legacyTypeNC(ft2);
        }
      }
    }
  }

  return var;
}


Variable *ClangImport::existingVariableForDeclaration(CXCursor cxDecl)
{
  Variable *&var = variableRefForDeclaration(cxDecl);
  xassert(var);
  return var;
}


// Return true if 'type' is a variable-length array.
static bool isVariableLengthArray(Type const *type)
{
  if (ArrayType const *at = type->ifArrayTypeC()) {
    return at->getSize() == ArrayType::DYN_SIZE;
  }
  else {
    return false;
  }
}


Declaration *ClangImport::importVarOrTypedefDecl(CXCursor cxVarDecl,
  DeclaratorContext context)
{
  Variable *var = variableForDeclaration(cxVarDecl);
  Declaration *decl = makeDeclaration(var, context);
  Declarator *declarator = fl_first(decl->decllist);

  // Add qualifiers if needed.
  decl->dflags |= possiblyAddNameQualifiers_and_getStorageClass(
    declarator, cxVarDecl);

  // Get children, partitioned by category.
  PartitionedChildren children(cxVarDecl);

  // Process attributes, which are associated with the declarator.
  // (When an attribute is associated with the entire declaration, Clang
  // takes care of duplicating it as needed.)
  if (!children.m_attributes.empty()) {
    importDeclaratorAttributes(declarator, children.m_attributes);
    children.m_attributes.clear();
  }

  // The interpretation of 'children' depends on the node type.
  CXCursorKind declKind = clang_getCursorKind(cxVarDecl);
  switch (declKind) {
    case CXCursor_VarDecl:
      xassert(children.m_expressions.size() <= 1);
      if (children.m_expressions.size() == 1) {
        CXCursor first = children.m_expressions.front();
        if (isVariableLengthArray(var->type)) {
          // The child specifies the array length.
          D_array *arrayDeclarator =
            declarator->decl->getSecondFromBottom()->asD_array();
          arrayDeclarator->size = importExpression(first);
        }
        else {
          // The child specifies the initializer.
          declarator->init = importInitializer(first);
        }
      }
      children.m_expressions.clear();
      break;

    case CXCursor_FunctionDecl:
    case CXCursor_CXXMethod:
      // For a definition, the children declare the parameters.  I do
      // not need to look at them here.
      children.m_declarations.clear();
      break;

    case CXCursor_TypedefDecl:
      // If the type specifier of the typedef defines a type (like a
      // struct), that definition appears as a child, which we can
      // safely ignore here.
      children.m_declarations.clear();
      break;

    default:
      // For other kinds, I think there should not be children.
      break;
  }

  // Verify that all children have been consumed.
  xassert(children.empty());

  return decl;
}


DeclFlags ClangImport::possiblyAddNameQualifiers_and_getStorageClass(
  Declarator *declarator, CXCursor cxVarDecl)
{
  // Add qualifiers if needed.
  bool qualified = possiblyAddNameQualifiers(declarator, cxVarDecl);

  DeclFlags declFlags = importStorageClass(
    clang_Cursor_getStorageClass(cxVarDecl));

  if (qualified) {
    // If a declaration uses a qualifier, it is not allowed to also
    // say 'static'.
    declFlags &= ~DF_STATIC;
  }

  return declFlags;
}


bool ClangImport::possiblyAddNameQualifiers(Declarator *declarator,
  CXCursor cxVarDecl)
{
  SourceLoc loc = cursorLocation(cxVarDecl);
  PQName *declaratorName = declarator->getDeclaratorId();
  int loopCounter = 0;

  CXCursor lexParent = clang_getCursorLexicalParent(cxVarDecl);
  if (clang_getCursorKind(lexParent) == CXCursor_FunctionDecl) {
    // A qualified name cannot be declared local to a function.
    return false;
  }

  CXCursor semParent = clang_getCursorSemanticParent(cxVarDecl);
  while (!( clang_equalCursors(semParent, lexParent) ||
            clang_getCursorKind(semParent) == CXCursor_TranslationUnit )) {
    if (++loopCounter > 100) {
      // Guard against an infinite loop.
      xfatal("possiblyAddNameQualifiers: hit loop counter limit");
    }

    StringRef semParentName = cursorSpelling(semParent);

    // TODO: Handle template arguments.
    TemplateArgument *templArgs = nullptr;

    declaratorName = new PQ_qualifier(loc,
      semParentName, templArgs, declaratorName);

    semParent = clang_getCursorSemanticParent(semParent);
    xassert(!clang_isInvalid(clang_getCursorKind(semParent)));
  }

  declarator->setDeclaratorId(declaratorName);
  return loopCounter > 0;
}


void ClangImport::importDeclaratorAttributes(Declarator *declarator,
  std::vector<CXCursor> const &cxAttributes)
{
  for (CXCursor const &cxAttribute : cxAttributes) {
    Attribute *attr = importAttribute(cxAttribute);
    AttributeSpecifier *aspec =
      new AttributeSpecifier(attr, nullptr /*next*/);
    AttributeSpecifierList *asl =
      new AttributeSpecifierList(aspec, nullptr /*next*/);

    // I am simply assuming that the attribute is to be associated with
    // the outermost declarator since I can't really tell with Clang.
    declarator->decl->appendASL(asl);
  }
}


Attribute *ClangImport::importAttribute(CXCursor cxAttribute)
{
  SourceLoc loc = cursorLocation(cxAttribute);
  CXAttributeSyntaxKind kind = clang_getAttributeSyntaxKind(cxAttribute);
  WrapCXString attrName(clang_getAttributeName(cxAttribute));
  WrapCXString prettyAttrString(clang_getAttributePrettyString(cxAttribute));

  switch (kind) {
    default:
      printSubtree(cxAttribute);
      xunimp(stringb("importAttribute: kind: " << kind));

    case CXAttributeSyntaxKind_GNU:
      return importGNUAttribute(loc,
        attrName.c_str(), prettyAttrString.c_str());
  }

  // Not reached.
}


Attribute *ClangImport::importGNUAttribute(SourceLoc loc,
  char const *attrName, char const *prettyAttrString)
{
  ParseString parser(prettyAttrString);
  parser.skipWS();
  parser.parseString("__attribute__((");

  // Empty attributes are dropped by Clang.
  xassert(parser.cur() != ')');

  // Get the name from the pretty string, and confirm it matches
  // 'attrName'.
  {
    string parsedName = parser.parseCToken();
    if (parsedName != attrName) {
      // The "pretty" form does not include the surrounding underscores,
      // but the 'attrName' may.
      if (stringb("__" << parsedName << "__") != attrName) {
        xfailure(stringb("importGNUAttribute: attrName=\"" << attrName <<
                         "\" but parsedName=\"" << parsedName << "\""));
      }
    }

    // We use 'attrName' rather than 'parsedName' in the created AST
    // node to preserve the underscore information.
  }

  if (parser.cur() == ')') {
    // The attribute is just the name.
    parser.parseString("))");
    parser.parseEOS();
    return new AT_word(loc, addStringRef(attrName));
  }

  // The attribute is a name followed by some arguments.
  parser.parseString("(");
  FakeList<ArgExpression> *args = FakeList<ArgExpression>::emptyList();

  while (true) {
    StringRef arg = addStringRef(parser.parseCToken().c_str());
    args = fl_prepend(args,
      new ArgExpression(importAttributeArgument(loc, arg)));

    parser.skipWS();
    if (parser.cur() == ')') {
      break;
    }
    else if (parser.cur() == ',') {
      parser.parseString(",");
      parser.skipWS();
    }
    else {
      parser.throwErr(stringb("importGNUAttribute: Unexpected char: '" <<
                              parser.cur() << "'."));
    }
  }

  args = fl_reverse(args);

  parser.parseString(")))");
  parser.parseEOS();

  return new AT_func(loc, addStringRef(attrName), args);
}


Expression *ClangImport::importAttributeArgument(SourceLoc loc,
  StringRef arg)
{
  if (isASCIIDigit(arg[0])) {
    // Number.
    return new E_intLit(arg);
  }
  else if (arg[0] == '"') {
    // String.
    return new E_stringLit(arg);
  }
  else if (arg[0] == '\'') {
    // Character.
    return new E_charLit(arg);
  }
  else if (isCIdentifierCharacter(arg[0])) {
    // Identifier.
    return new E_variable(new PQ_name(loc, arg));
  }
  else {
    xfatal(stringb("importGNUAttribute: Unexpected arg: \"" <<
                   arg << "\"."));
    return nullptr; // Not reached.
  }
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
    xunimp(stringb("cxTypeKindToSimpleTypeId: Unhandled simple type kind: " << toString(kind)));
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
    return m_tfac.getSimpleType(cxTypeKindToSimpleTypeId(cxType.kind), cv);
  }

  switch (cxType.kind) {
    default:
      xfailure(stringb("Unknown cxType kind: " << toString(cxType.kind)));

    case CXType_Invalid: // 0
      xfailure("importType: cxType is invalid");

    // Codes 2 through 23 are handled above.

    case CXType_Pointer: // 101
      return m_tfac.makePointerType(cv,
        importType(clang_getPointeeType(cxType)));

    case CXType_LValueReference: // 103
      return m_tfac.makeReferenceType(
        importType(clang_getPointeeType(cxType)));

    // TODO: CXType_RValueReference = 104

    case CXType_Record: // 105
    case CXType_Enum: // 106
      // For a Record or Enum that does not go through Elaborated,
      // create a TypedefType, the same as the Typedef case.
      goto handleTypedef;

    case CXType_Elaborated: { // 119
      CXCursor cxTypeDecl = clang_getTypeDeclaration(cxType);
      Variable *typedefVar = existingVariableForDeclaration(cxTypeDecl);
      return typedefVar->type;
    }

    handleTypedef:
    case CXType_Typedef: { // 107
      CXCursor cxTypeDecl = clang_getTypeDeclaration(cxType);
      Variable *typedefVar = existingVariableForDeclaration(cxTypeDecl);
      return m_tfac.makeTypedefType(typedefVar, cv);
    }

    case CXType_FunctionNoProto: // 110
    case CXType_FunctionProto: // 111
      xassert(cv == CV_NONE);
      return importFunctionType(cxType, methodClass, methodCV);

    case CXType_ConstantArray: // 112
    case CXType_IncompleteArray: // 114
    case CXType_VariableArray: // 115
    case CXType_DependentSizedArray: // 116
      xassert(cv == CV_NONE);
      return importArrayType(cxType);
  }

  // Not reached.
}


FunctionType *ClangImport::importFunctionType(CXType cxFunctionType,
  CompoundType const * NULLABLE methodClass, CVFlags methodCV)
{
  Type *retType = importType(clang_getResultType(cxFunctionType));

  FunctionType *ft = m_tfac.makeFunctionType(retType);

  if (methodClass) {
    // Note: Even if 'cxFunctionType' refers to a method type, libclang
    // does *not* report it as cv-qualified, even though
    // 'clang_getTypeSpelling' will include the 'const'.  Instead, the
    // declaration itself can be 'clang_CXXMethod_isConst'.
    // Consequently, 'methodCV' must be passed in by the caller.
    addReceiver(ft, SL_UNKNOWN, methodClass, methodCV);
  }

  int numParams = clang_getNumArgTypes(cxFunctionType);
  for (int i=0; i < numParams; i++) {
    Type *paramType = importType(clang_getArgType(cxFunctionType, i));

    // The Clang AST does not appear to record the names of function
    // parameters for non-definitions anywhere other than as tokens
    // that would have to be parsed.
    //
    // TODO: That is wrong; non-definition FunctionProto nodes have
    // ParmDecl children.  I should get the names from them.
    Variable *paramVar = makeVariable(SL_UNKNOWN, nullptr /*name*/,
      paramType, DF_PARAMETER);

    ft->addParam(paramVar);
  }

  if (cxFunctionType.kind == CXType_FunctionNoProto) {
    // Note: If this declaration is also a definition, then C11
    // 6.7.6.3p14 says it accepts no parameters.  But the response
    // to Defect Report 317 says that, even so, the function type
    // still has no parameter info.
    //
    // https://www.open-std.org/jtc1/sc22/wg14/www/docs/dr_317.htm
    ft->setFlag(FF_NO_PARAM_INFO);
  }
  else if (clang_isFunctionTypeVariadic(cxFunctionType)) {
    ft->setFlag(FF_VARARGS);
  }

  m_tfac.doneParams(ft);
  return ft;
}


void ClangImport::addReceiver(FunctionType *methodType,
  SourceLoc loc, CompoundType const *containingClass, CVFlags cv)
{
  Type *qualifiedCT =
    m_tfac.makeCVAtomicType(legacyTypeNC(containingClass), cv);
  Type *recvRefType = m_tfac.makeReferenceType(qualifiedCT);
  methodType->addReceiver(makeVariable(
    loc,
    addStringRef("__receiver"), // Like Env::receiverName.
    recvRefType,
    DF_PARAMETER));
}


ArrayType *ClangImport::importArrayType(CXType cxArrayType)
{
  Type *eltType = importType(clang_getElementType(cxArrayType));

  int size;
  switch (cxArrayType.kind) {
    default:
      xfailure("bad array type");

    case CXType_ConstantArray: // 112
      checkedAssignment(size, clang_getArraySize(cxArrayType));
      break;

    case CXType_IncompleteArray: // 114
      size = ArrayType::NO_SIZE;
      break;

    case CXType_VariableArray: // 115
      size = ArrayType::DYN_SIZE;
      break;

    case CXType_DependentSizedArray: // 116
      // Elsa really should have this as a separate case too.  I will
      // just say the size is unspecified.
      size = ArrayType::NO_SIZE;
      break;
  }

  return m_tfac.makeArrayType(eltType, size);
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


static void assertEqualPQNames(
  PQName const *pq1,
  PQName const *pq2)
{
  // For expedience, just compare the string representations.
  xassert(pq1->toString() == pq2->toString());
}


// Assert that 'ts1' and 'ts2' say the same thing.
static void assertEqualTypeSpecifiers(
  TypeSpecifier const *ts1,
  TypeSpecifier const *ts2)
{
  xassert(ts1->cv == ts2->cv);
  xassert(ts1->kind() == ts2->kind());

  switch (ts1->kind()) {
    default:
      xfailure("bad TypeSpecifier kind");

    case TypeSpecifier::TS_NAME: {
      TS_name const *n1 = ts1->asTS_nameC();
      TS_name const *n2 = ts2->asTS_nameC();
      assertEqualPQNames(n1->name, n2->name);
      xassert(n1->typenameUsed == n2->typenameUsed);
      break;
    }

    case TypeSpecifier::TS_SIMPLE: {
      TS_simple const *s1 = ts1->asTS_simpleC();
      TS_simple const *s2 = ts2->asTS_simpleC();
      xassert(s1->id == s2->id);
      break;
    }

    case TypeSpecifier::TS_ELABORATED: {
      TS_elaborated const *e1 = ts1->asTS_elaboratedC();
      TS_elaborated const *e2 = ts2->asTS_elaboratedC();
      xassert(e1->keyword == e2->keyword);
      assertEqualPQNames(e1->name, e2->name);
      break;
    }

    case TypeSpecifier::TS_CLASSSPEC: {
      TS_classSpec const *c1 = ts1->asTS_classSpecC();
      TS_classSpec const *c2 = ts2->asTS_classSpecC();
      xassert(c1->keyword == c2->keyword);
      assertEqualPQNames(c1->name, c2->name);

      // I will just assume that the bases and fields agree if the
      // names agree.

      break;
    }

    case TypeSpecifier::TS_ENUMSPEC: {
      TS_enumSpec const *e1 = ts1->asTS_enumSpecC();
      TS_enumSpec const *e2 = ts2->asTS_enumSpecC();
      xassert(e1->name == e2->name);

      // Similarly, assume the enumerators agree.

      break;
    }

    case TypeSpecifier::TS_TYPEOF: {
      // TODO: Actually compare these.
      break;
    }
  }
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
      printSubtree(cxStmt);
      xunimp(stringb("importStatement: Unhandled stmtKind: " << toString(stmtKind)));

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
      // Overall declaration to build.
      Declaration *decl = nullptr;

      // When a single statement declares multiple things, they show up
      // as multiple children here.
      for (CXCursor const &child : children) {
        CXCursorKind childKind = clang_getCursorKind(child);
        xassert(childKind == CXCursor_VarDecl ||
                childKind == CXCursor_TypedefDecl);
        if (!decl) {
          // First declaration.
          decl = importVarOrTypedefDecl(child, DC_S_DECL);
        }
        else {
          // Subsequent declaration.
          Declaration *next = importVarOrTypedefDecl(child, DC_S_DECL);

          // We will merge 'next' onto 'decl'.  First, check that the
          // declaration specifiers and type specifiers agree.
          xassert(decl->dflags == next->dflags);
          assertEqualTypeSpecifiers(decl->spec, next->spec);

          // Now join the declarator lists.
          xassert(fl_count(next->decllist) == 1);
          decl->decllist = fl_prepend(decl->decllist, fl_first(next->decllist));

          // Clean up 'next'.  This won't get everything because AST
          // nodes do not automatically delete their children, but it
          // gets what is easy.
          delete next->spec;
          next->spec = nullptr;
          next->decllist = FakeList<Declarator>::emptyList();
          delete next;
        }
      }

      // Should have seen at least one declaration.
      xassert(decl);

      // When there are multiple, we build the list in reverse order.
      decl->decllist = fl_reverse(decl->decllist);

      return new S_decl(loc, decl);
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


std::string ClangImport::evalAsString(CXCursor cxExpr)
{
  CXEvalResult cxEvalResult = clang_Cursor_Evaluate(cxExpr);
  xassert(clang_EvalResult_getKind(cxEvalResult) == CXEval_StrLiteral);
  std::string ret(clang_EvalResult_getAsStr(cxEvalResult));
  clang_EvalResult_dispose(cxEvalResult);

  return ret;
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
      printSubtree(cxExpr);
      xunimp(stringb("importExpression: Unhandled exprKind: " << toString(exprKind)));

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
        // Note: When multiple conversions take place at the same
        // location, Clang uses multiple conversion nodes, whereas Elsa
        // combines them.  For now at least, I'm not collapsing them the
        // way Elsa would.
        StandardConversion conv = describeAsStandardConversion(
          /*dest*/ type, /*src*/ childType, getSpecialExpr(child));
        ret = new E_implicitStandardConversion(conv,
          importExpression(child));
      }
      break;
    }

    case CXCursor_DeclRefExpr: { // 101
      CXCursor decl = clang_getCursorReferenced(cxExpr);

      // The expression might be referring to an implicitly declared
      // function in C, so we need to allow for the possibility that the
      // declaration has not yet been seen.
      Variable *var = variableForDeclaration(decl);

      ret = makeE_variable(var);
      break;
    }

    case CXCursor_MemberRefExpr: { // 102
      Expression *object = importExpression(getOnlyChild(cxExpr));
      if (object->type->isPointerType()) {
        // The Clang AST uses the same node for "." and "->", with the
        // latter distinguished only by the fact that the child
        // expression in that case has pointer type.  So I explicitly
        // insert the dereference here.
        object = makeE_deref(object);
      }

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

      if (E_implicitStandardConversion *calleeISC =
            callee->ifE_implicitStandardConversion()) {
        if (calleeISC->conv == SC_FUNC_TO_PTR) {
          // For ordinary calls to a named function, Clang inserts an
          // implicit conversion to the function's address.  I suppose
          // that is done to make the AST more uniform, but I don't like
          // it because it clutters the --print-isc output.  Discard the
          // ISC.
          callee = calleeISC->expr;
          calleeISC->expr = nullptr;
          delete calleeISC;
        }
      }

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
        addStringRef(stringbc(value)));
      intLit->i = value;
      ret = intLit;
      break;
    }

    case CXCursor_StringLiteral: // 109
      ret = importStringLiteral(cxExpr);
      break;

    case CXCursor_CharacterLiteral: // 110
      ret = importCharacterLiteral(cxExpr);
      break;

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
          printSubtree(childCursor);
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

    case CXCursor_ArraySubscriptExpr: { // 113
      CXCursor cxLHS, cxRHS;
      getTwoChildren(cxLHS, cxRHS, cxExpr);

      Expression *lhs = importExpression(cxLHS);
      Expression *rhs = importExpression(cxRHS);

      // One of the operands should be a pointer, and that determines
      // the type of the addition node.
      E_binary *bin = new E_binary(lhs, BIN_PLUS, rhs);
      if (lhs->type->isPointerType()) {
        bin->type = lhs->type;
      }
      else {
        xassert(rhs->type->isPointerType());
        bin->type = rhs->type;
      }

      // The Elsa AST represents 'a[b]' as '*(a+b)'.
      ret = new E_deref(bin);
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

    case CXCursor_ConditionalOperator: { // 116
      std::vector<CXCursor> children = getChildren(cxExpr);
      xassert(children.size() == 3);
      ret = new E_cond(
        importExpression(children[0]),
        importExpression(children[1]),
        importExpression(children[2])
      );
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


E_stringLit *ClangImport::importStringLiteral(CXCursor cxExpr)
{
  // The spelling has something like the original quoted syntax,
  // although it is different at least because (1) it concatenates all
  // of the adjacent tokens, and (2) it normalizes octal escape
  // sequences to use three digits.  Nevertheless, it is fine for my
  // purposes.
  StringRef quotedRef = cursorSpelling(cxExpr);

  E_stringLit *slit = new E_stringLit(quotedRef);

  unsigned length = clang_stringLiteralElement(cxExpr,
    CXStringLiteralElement_length);
  unsigned charByteWidth = clang_stringLiteralElement(cxExpr,
    CXStringLiteralElement_charByteWidth);
  xassert(charByteWidth > 0);

  size_t numBytes = length * charByteWidth;

  // Check for overflow.  This is valid because the multiplication
  // was done using unsigned arithmetic.
  xassert(numBytes / charByteWidth == length);

  slit->m_stringData.setAllocated(numBytes);
  clang_getStringLiteralBytes(cxExpr,
    slit->m_stringData.getData(), numBytes);

  return slit;
}


// Create the token denoting a character literal of 'kind' with 'value'.
static std::string getCharLitToken(unsigned value,
  CXCharacterLiteralKind kind)
{
  std::ostringstream litText;

  // If we print any integers, use uppercase hex.
  litText << std::uppercase << std::hex;

  // String literal prefix character.
  switch (kind) {
    case CXCharacterLiteralKind_Ascii:
      break;

    case CXCharacterLiteralKind_Wide:
      litText << 'L';
      break;

    default:
      xunimp(stringb("importCharacterLiteral: unhandled kind: " << kind));
  }

  litText << '\'';

  if (value == 0) {
    litText << "\\0";
  }
  else if (32 <= value && value <= 126) {
    if (value == '\\' || value == '\'') {
      litText << '\\';
    }
    litText << (char)value;
  }
  else if (kind == CXCharacterLiteralKind_Wide) {
    // Express as a single hex escape sequence.
    litText << "\\x" << value;
  }
  else if (value >= (unsigned)(-0x80) || value <= 0xFF) {
    // Express as a single two-digit hex escape sequence.
    litText << "\\x" << (value & 0xFF);
  }
  else {
    // Use a sequence of hex escape sequences, one for each byte.
    //
    // I'm not really sure if this is right.  This all depends on
    // "implementation-defined" behavior.  According to
    // https://en.cppreference.com/w/cpp/language/character_literal,
    // this should be at least approximately right for compilers other
    // than MSVC.
    std::vector<unsigned char> bytes;
    while (value != 0) {
      bytes.push_back(value & 0xFF);
      value >>= 8;
    }
    for (int i = bytes.size()-1; i >= 0; --i) {
      litText << "\\x" << (unsigned)bytes[i];
    }
  }

  litText << '\'';

  return litText.str();
}


E_charLit *ClangImport::importCharacterLiteral(CXCursor cxExpr)
{
  CXCharacterLiteralKind kind = static_cast<CXCharacterLiteralKind>(
    clang_characterLiteralElement(cxExpr, CXCharacterLiteralElement_kind));

  unsigned value = clang_characterLiteralElement(cxExpr,
    CXCharacterLiteralElement_value);

  E_charLit *charLit =
    new E_charLit(addStringRef(getCharLitToken(value, kind).c_str()));
  charLit->c = value;

  return charLit;
}


SpecialExpr ClangImport::getSpecialExpr(CXCursor cxExpr)
{
  switch (clang_getCursorKind(cxExpr)) {
    default:
      break;

    case CXCursor_IntegerLiteral:
      if (evalAsLongLong(cxExpr) == 0) {
        return SE_ZERO;
      }
      break;

    case CXCursor_StringLiteral:
      return SE_STRINGLIT;
  }

  return SE_NONE;
}


StandardConversion ClangImport::describeAsStandardConversion(
  Type const *destType, Type const *srcType, SpecialExpr srcSpecial)
{
  // Use the Elsa type checker rules.
  string errorMsg;
  StandardConversion sc = getStandardConversion(
    &errorMsg,
    m_elsaParse.m_lang,
    srcSpecial,
    srcType,
    destType);

  if (sc == SC_ERROR) {
    if (destType->isVoid()) {
      // Assume this is something like the ?: exception that allows an
      // implicit conversion to void.
      return SC_ADH_TO_VOID;
    }

    if (destType->isIntegerType() && srcType->isPointerType()) {
      // This shows up for in/c/t0010.c.  Clang issues a warning on the
      // command line, but allows it.
      return SC_ADH_PTR_TO_INT;
    }

    if (destType->isPointerType() && srcType->isIntegerType()) {
      // Another from in/c/t0010.c.
      return SC_ADH_INT_TO_PTR;
    }

    xfatal(stringb(
      "describeAsStandardConversion: destType='" << destType->toString() <<
      "' srcType='" << srcType->toString() <<
      "' srcSpecial=" << toString(srcSpecial) <<
      " error=\"" << errorMsg << "\""));
  }

  return sc;
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
  SourceLoc loc = cursorLocation(cxInit);
  std::vector<CXCursor> children = getChildren(cxInit);

  CXCursorKind initKind = clang_getCursorKind(cxInit);
  switch (initKind) {
    case CXCursor_UnexposedExpr: // 100
      if (children.empty()) {
        // This is an 'ImplicitValueInitExpr', which in Elsa is represented
        // with a NULL initialzer.
        return nullptr;
      }
      break;

    case CXCursor_CallExpr: // 103
      if (children.empty()) {
        // This is a 'CXXConstructExpr', which in Elsa is also represented
        // with a NULL initialzer.
        return nullptr;
      }

    case CXCursor_InitListExpr: { // 119
      IN_compound *inc = new IN_compound(loc, nullptr /*inits*/);
      for (CXCursor const &child : children) {
        inc->inits.append(importInitializer(child));
      }
      return inc;
    }

    default:
      break;
  }

  return new IN_expr(loc, importExpression(cxInit));
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
    typeId->decl->type = m_tfac.getSimpleType(ST_ELLIPSIS);
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
  return m_tfac.makeVariable(loc, name, type, flags);
}


StringRef ClangImport::addStringRef(char const *str)
{
  return m_elsaParse.m_stringTable.add(str);
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


void ClangImport::printSubtree(CXCursor cursor)
{
  ClangPrint clangPrint(cout);

  static char const *maxTokensStr = getenv("MAX_TOKENS");
  if (maxTokensStr) {
    clangPrint.m_maxTokensToPrint = atoi(maxTokensStr);
  }

  clangPrint.printCursor(cursor, 0 /*indent*/);
}


// EOF
