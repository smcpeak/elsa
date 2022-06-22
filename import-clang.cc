// import-clang.cc
// Code for import-clang.h.

#include "import-clang-internal.h"     // this module

// elsa
#include "libclang-additions.h"        // clang_getUnaryExpressionOperator, etc.

// smbase
#include "map-utils.h"                 // insertMapUnique
#include "exc.h"                       // xfatal
#include "str.h"                       // string


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

  ImportClang importClang(cxIndex, cxTU, elsaParse);
  importClang.importTranslationUnit();
}


// --------------------------- WrapCXTokens ----------------------------
WrapCXTokens::WrapCXTokens(CXTranslationUnit cxTU, CXSourceLocation cxLoc)
  : m_cxTU(cxTU),
    m_cxTokens(clang_getToken(cxTU, cxLoc)),
    m_numTokens(1)
{}


WrapCXTokens::WrapCXTokens(CXTranslationUnit cxTU, CXSourceRange cxRange)
  : m_cxTU(cxTU),
    m_cxTokens(nullptr),
    m_numTokens(0)
{
  clang_tokenize(cxTU, cxRange, &m_cxTokens, &m_numTokens);
}


WrapCXTokens::~WrapCXTokens()
{
  clang_disposeTokens(m_cxTU, m_cxTokens, m_numTokens);
}


CXToken WrapCXTokens::operator[] (unsigned i) const
{
  xassert(i < m_numTokens);
  return m_cxTokens[i];
}


CXToken WrapCXTokens::front() const
{
  return (*this)[0];
}


CXToken WrapCXTokens::back() const
{
  return (*this)[size()-1];
}


WrapCXString WrapCXTokens::stringAt(unsigned index)
{
  xassert(index < m_numTokens);
  return WrapCXString(clang_getTokenSpelling(m_cxTU, m_cxTokens[index]));
}


// --------------------------- ImportClang -----------------------------
ImportClang::ImportClang(CXIndex cxIndex,
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


void ImportClang::importTranslationUnit()
{
  CXCursor cursor = clang_getTranslationUnitCursor(m_cxTranslationUnit);
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


std::vector<CXCursor> ImportClang::getChildren(CXCursor cursor)
{
  std::vector<CXCursor> children;
  clang_visitChildren(cursor, childCollector, &children);
  return children;
}


StringRef ImportClang::importString(CXString cxString)
{
  WrapCXString wcxString(cxString);
  return m_elsaParse.m_stringTable.add(wcxString.c_str());
}


StringRef ImportClang::importFileName(CXFile cxFile)
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


SourceLoc ImportClang::importSourceLocation(CXSourceLocation cxLoc)
{
  // TODO: It would probably be more efficient to use the offset.
  CXFile cxFile;
  unsigned line;
  unsigned column;
  clang_getFileLocation(cxLoc, &cxFile, &line, &column, nullptr /*offset*/);

  StringRef fname = importFileName(cxFile);
  return sourceLocManager->encodeLineCol(fname, line, column);
}


SourceLoc ImportClang::cursorLocation(CXCursor cxCursor)
{
  return importSourceLocation(clang_getCursorLocation(cxCursor));
}


StringRef ImportClang::cursorSpelling(CXCursor cxCursor)
{
  return importString(clang_getCursorSpelling(cxCursor));
}


TopForm *ImportClang::importTopForm(CXCursor cxTopForm)
{
  SourceLoc loc = cursorLocation(cxTopForm);

  CXCursorKind cxKind = clang_getCursorKind(cxTopForm);
  switch (cxKind) {
    default:
      xfailure(stringb("Unknown cxKind: " << cxKind));

    case CXCursor_EnumDecl: {
      if (clang_isCursorDefinition(cxTopForm)) {
        Declaration *decl = importEnumDefinition(cxTopForm);
        return new TF_decl(loc, decl);
      }
      else {
        xunimp("enum forward declaration");
      }
    }

    importDecl:
    case CXCursor_TypedefDecl:
    case CXCursor_VarDecl: {
      Declaration *decl = importVarOrTypedefDecl(cxTopForm, DC_TF_DECL);
      return new TF_decl(loc, decl);
    }

    case CXCursor_FunctionDecl: {
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


Declaration *ImportClang::importEnumDefinition(CXCursor cxEnumDefn)
{
  SourceLoc loc = cursorLocation(cxEnumDefn);

  // This call creates the EnumType.
  Variable *enumVar = variableForDeclaration(cxEnumDefn);
  EnumType const *enumType = enumVar->type->asNamedAtomicType()->asEnumType();

  // Add the enumerators to 'enumerators'.
  FakeList<Enumerator> *enumerators = FakeList<Enumerator>::emptyList();
  for (CXCursor const &cxEnumerator : getChildren(cxEnumDefn)) {
    xassert(clang_getCursorKind(cxEnumerator) == CXCursor_EnumConstantDecl);

    Enumerator *enumerator = importEnumerator(cxEnumerator, enumType);

    enumerators = fl_prepend(enumerators, enumerator);
  }

  // The list was built in reverse order due to prepending.
  enumerators = fl_reverse(enumerators);

  TS_enumSpec *enumTS = new TS_enumSpec(loc, enumVar->name, enumerators);
  enumTS->etype = legacyTypeNC(enumType);

  // Wrap up the type specifier in a declaration with no declarators.
  return new Declaration(DF_NONE, enumTS, nullptr /*decllist*/);
}


Enumerator *ImportClang::importEnumerator(CXCursor cxEnumerator,
  EnumType const *enumType)
{
  // Get the enumerator name and value.
  //
  // This duplicates work done in 'importEnumType'.  I'm not sure how
  // best to avoid that; the possibility of forward-declared enums
  // means I might need the semantics before I see the syntax.
  StringRef name = cursorSpelling(cxEnumerator);
  long long llValue = clang_getEnumConstantDeclValue(cxEnumerator);
  int intValue = (int)llValue;
  if (intValue != llValue) {
    xunimp(stringb("Enumerator value out of representation range: " << llValue));
  }

  // Get the expression that specifies the value, if any.
  Expression * NULLABLE expr = nullptr;
  std::vector<CXCursor> children = getChildren(cxEnumerator);
  if (!children.empty()) {
    xassert(children.size() == 1);
    expr = importExpression(children.front());
  }

  // Get the EnumType::Value for this enumerator.
  EnumType::Value const *valueObject = enumType->getValue(name);
  xassert(valueObject);

  // Build the enumerator.
  SourceLoc loc = cursorLocation(cxEnumerator);
  Enumerator *enumerator = new Enumerator(loc, name, expr);
  enumerator->var = valueObject->decl;
  enumerator->enumValue = intValue;
  return enumerator;
}


Function *ImportClang::importFunctionDefinition(CXCursor cxFuncDefn)
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
      if (childKind == CXCursor_ParmDecl) {
        Variable *paramVar = variableForDeclaration(child, DF_PARAMETER);
        defnFuncType->addParam(paramVar);
      }
      else if (childKind == CXCursor_CompoundStmt) {
        // The final child is the function body.
        xassert(!foundBody);
        foundBody = true;
        cxFunctionBody = child;
      }
      else {
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


Variable *ImportClang::variableForDeclaration(CXCursor cxDecl,
  DeclFlags declFlags)
{
  // Go to the canonical cursor to handle the case of an entity that is
  // declared multiple times.
  cxDecl = clang_getCanonicalCursor(cxDecl);

  // CXCursor is a structure with several elements, so does not appear
  // usable as a map key.  Use the location instead.
  SourceLoc loc = cursorLocation(cxDecl);

  StringRef name = cursorSpelling(cxDecl);

  Variable *&var = m_locToVariable[loc];
  if (!var) {
    Type *type;

    CXCursorKind declKind = clang_getCursorKind(cxDecl);
    if (declKind == CXCursor_TypedefDecl) {
      declFlags |= DF_TYPEDEF;

      // For a typedef, the 'type' is the underlying type.
      type = importType(clang_getTypedefDeclUnderlyingType(cxDecl));
    }

    else if (declKind == CXCursor_EnumDecl) {
      declFlags |= DF_TYPEDEF;

      // Use the definition if it is available.
      CXCursor cxDefn = clang_getCursorDefinition(cxDecl);
      if (!clang_Cursor_isNull(cxDefn)) {
        cxDecl = cxDefn;
      }

      EnumType *enumType = importEnumType(cxDecl);

      xassert(!var);
      var = enumType->typedefVar;
      return var;
    }

    else {
      // For anything else, the type is what the declaration says.
      type = importType(clang_getCursorType(cxDecl));
    }

    xassert(!var);
    var = makeVariable_setScope(loc, name, type, declFlags, cxDecl);
  }

  else {
    // Detect location collisions.
    xassert(var->name == name);
  }

  return var;
}


Declaration *ImportClang::importVarOrTypedefDecl(CXCursor cxVarDecl,
  DeclaratorContext context)
{
  Variable *var = variableForDeclaration(cxVarDecl);
  return makeDeclaration(var, context);
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

  Entry const table[] = {
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


Type *ImportClang::importType(CXType cxType)
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

  if (CXType_Void <= cxType.kind && cxType.kind <= CXType_LongDouble) {
    return tfac.getSimpleType(cxTypeKindToSimpleTypeId(cxType.kind));
  }

  switch (cxType.kind) {
    default:
      xfailure(stringb("Unknown cxType kind: " << cxType.kind));

    case CXType_Invalid:
      xfailure("importType: cxType is invalid");

    case CXType_Pointer:
      return tfac.makePointerType(cv,
        importType(clang_getPointeeType(cxType)));

    case CXType_Typedef: {
      CXCursor cxTypeDecl = clang_getTypeDeclaration(cxType);
      Variable *typedefVar = variableForDeclaration(cxTypeDecl);
      return tfac.makeTypedefType(typedefVar, cv);
    }

    case CXType_FunctionProto: {
      xassert(cv == CV_NONE);
      Type *retType = importType(clang_getResultType(cxType));

      FunctionType *ft = tfac.makeFunctionType(retType);

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

    case CXType_Enum: {
      CXCursor cxTypeDecl = clang_getTypeDeclaration(cxType);
      Variable *typedefVar = variableForDeclaration(cxTypeDecl);
      return typedefVar->type;
    }
  }

  // Not reached.
}


EnumType *ImportClang::importEnumType(CXCursor cxEnumDefn)
{
  SourceLoc loc = cursorLocation(cxEnumDefn);
  StringRef enumName = cursorSpelling(cxEnumDefn);
  TypeFactory &tfac = m_elsaParse.m_typeFactory;

  // Set up the initially empty EnumType.
  EnumType *enumType = tfac.makeEnumType(enumName);
  Type *type = tfac.makeCVAtomicType(enumType, CV_NONE);
  enumType->typedefVar =
    makeVariable_setScope(loc, enumName, type, DF_NONE, cxEnumDefn);

  // Get all of the enumerators and add them to 'enumType'.
  for (CXCursor const &cxEnumerator : getChildren(cxEnumDefn)) {
    xassert(clang_getCursorKind(cxEnumerator) == CXCursor_EnumConstantDecl);

    // Get the enumerator name and value.
    StringRef enumeratorName = cursorSpelling(cxEnumerator);
    long long llValue = clang_getEnumConstantDeclValue(cxEnumerator);
    int intValue = (int)llValue;
    if (intValue != llValue) {
      xunimp(stringb("Enumerator value out of representation range: " << llValue));
    }

    Variable *enumeratorVar = makeVariable_setScope(
      cursorLocation(cxEnumerator),
      enumeratorName,
      type,
      DF_ENUMERATOR,
      cxEnumerator);

    enumType->addValue(enumeratorName, intValue, enumeratorVar);
  }

  return enumType;
}


S_compound *ImportClang::importCompoundStatement(CXCursor cxCompStmt)
{
  SourceLoc loc = cursorLocation(cxCompStmt);
  S_compound *comp = new S_compound(loc, nullptr /*stmts*/);

  std::vector<CXCursor> children = getChildren(cxCompStmt);
  for (CXCursor const &child : children) {
    comp->stmts.append(importStatement(child));
  }

  return comp;
}


Statement *ImportClang::importStatement(CXCursor cxStmt)
{
  if (clang_Cursor_isNull(cxStmt)) {
    // This is for a case like a missing initializer in a 'for'
    // statement.
    return new S_skip(SL_UNKNOWN, MI_IMPLICIT);
  }

  SourceLoc loc = cursorLocation(cxStmt);
  CXCursorKind stmtKind = clang_getCursorKind(cxStmt);
  std::vector<CXCursor> children = getChildren(cxStmt);

  switch (stmtKind) {
    default:
      xunimp(stringb("Unhandled stmtKind: " << stmtKind));

    case CXCursor_DeclRefExpr:      // 101
    case CXCursor_CallExpr:         // 103
    case CXCursor_IntegerLiteral:   // 106
    case CXCursor_ParenExpr:        // 111
    case CXCursor_UnaryOperator:    // 112
    case CXCursor_BinaryOperator:   // 114
    case CXCursor_CStyleCastExpr: { // 117
      FullExpression *fullExpr = importFullExpression(cxStmt);
      return new S_expr(loc, fullExpr);
    }

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
      CXCursor conditionExpr, childStmt;
      getTwoChildren(conditionExpr, childStmt, cxStmt);

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

    case CXCursor_ReturnStmt: { // 214
      FullExpression *retval = nullptr;
      if (!children.empty()) {
        xassert(children.size() == 1);
        retval = importFullExpression(children.front());
      }
      return new S_return(loc, retval);
    }

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


FullExpression *ImportClang::importFullExpression(CXCursor cxExpr)
{
  return new FullExpression(importExpression(cxExpr));
}


CXCursor ImportClang::getOnlyChild(CXCursor cxNode)
{
  std::vector<CXCursor> children = getChildren(cxNode);
  xassert(children.size() == 1);
  return children.front();
}


void ImportClang::getTwoChildren(CXCursor &c1, CXCursor &c2,
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


long long ImportClang::evalAsLongLong(CXCursor cxExpr)
{
  CXEvalResult cxEvalResult = clang_Cursor_Evaluate(cxExpr);
  xassert(clang_EvalResult_getKind(cxEvalResult) == CXEval_Int);
  long long value = clang_EvalResult_getAsLongLong(cxEvalResult);
  clang_EvalResult_dispose(cxEvalResult);

  return value;
}


Expression *ImportClang::importExpression(CXCursor cxExpr)
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

    case CXCursor_UnexposedExpr: { // 1
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
      Variable *var = variableForDeclaration(decl);
      ret = makeE_variable(var);
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
  }

  ret->type = legacyTypeNC(type);
  return ret;
}


StandardConversion ImportClang::describeAsStandardConversion(
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


Condition *ImportClang::importCondition(CXCursor cxCond)
{
  // For now, assume it is always an expression.
  return new CN_expr(importFullExpression(cxCond));
}


Variable *ImportClang::makeVariable_setScope(SourceLoc loc,
  StringRef name, Type *type, DeclFlags flags, CXCursor cxDecl)
{
  Variable *var = makeVariable(loc, name, type, flags);

  CXCursor cxSemParent = clang_getCursorSemanticParent(cxDecl);
  if (clang_getCursorKind(cxSemParent) == CXCursor_TranslationUnit) {
    var->m_containingScope = m_globalScope;
  }

  return var;
}


Variable *ImportClang::makeVariable(SourceLoc loc, StringRef name,
  Type *type, DeclFlags flags)
{
  return m_elsaParse.m_typeFactory.makeVariable(loc, name, type, flags);
}


// EOF
