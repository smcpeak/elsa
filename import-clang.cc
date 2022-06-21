// import-clang.cc
// Code for import-clang.h.

#include "import-clang-internal.h"     // this module

// smbase
#include "map-utils.h"                 // insertMapUnique
#include "exc.h"                       // xfatal
#include "str.h"                       // string


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

  switch (cxType.kind) {
    default:
      xfailure(stringb("Unknown cxType kind: " << cxType.kind));

    case CXType_Invalid:
      xfailure("importType: cxType is invalid");

    case CXType_Void: return tfac.getSimpleType(ST_VOID, cv);
    case CXType_Int:  return tfac.getSimpleType(ST_INT, cv);
    case CXType_UInt: return tfac.getSimpleType(ST_UNSIGNED_INT, cv);

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


S_compound *ImportClang::importCompoundStatement(CXCursor cxFunctionBody)
{
  SourceLoc loc = cursorLocation(cxFunctionBody);
  S_compound *comp = new S_compound(loc, nullptr /*stmts*/);

  std::vector<CXCursor> children = getChildren(cxFunctionBody);
  for (CXCursor const &child : children) {
    comp->stmts.append(importStatement(child));
  }

  return comp;
}


Statement *ImportClang::importStatement(CXCursor cxStmt)
{
  SourceLoc loc = cursorLocation(cxStmt);
  CXCursorKind stmtKind = clang_getCursorKind(cxStmt);
  std::vector<CXCursor> children = getChildren(cxStmt);

  switch (stmtKind) {
    default:
      xunimp(stringb("Unhandled stmtKind: " << stmtKind));

    case CXCursor_UnaryOperator: { // 112
      FullExpression *fullExpr = importFullExpression(cxStmt);
      return new S_expr(loc, fullExpr);
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


static UnaryOp stringToUnaryOp(char const *str)
{
  // Should be exactly one character.
  xassert(str[0] != 0 && str[1] == 0);

  switch (*str) {
    default:  xfailure("Unexpected unary operator character.");
    case '+': return UNY_PLUS;
    case '-': return UNY_MINUS;
    case '!': return UNY_NOT;
    case '~': return UNY_BITNOT;
  }

  // Not reached.
}


Expression *ImportClang::importExpression(CXCursor cxExpr)
{
  CXCursorKind exprKind = clang_getCursorKind(cxExpr);
  Type const *type = importType(clang_getCursorType(cxExpr));
  std::vector<CXCursor> children = getChildren(cxExpr);

  Expression *ret = nullptr;

  switch (exprKind) {
    default:
      xunimp(stringb("Unhandled exprKind: " << exprKind));

    case CXCursor_UnexposedExpr: { // 1
      // This is used for implicit conversions (perhaps among other
      // things).
      xassert(children.size() == 1);
      CXCursor child = children.front();
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

    case CXCursor_IntegerLiteral: { // 106
      // Evaluate the literal as an integer.
      CXEvalResult cxEvalResult = clang_Cursor_Evaluate(cxExpr);
      xassert(clang_EvalResult_getKind(cxEvalResult) == CXEval_Int);
      long long value = clang_EvalResult_getAsLongLong(cxEvalResult);
      clang_EvalResult_dispose(cxEvalResult);

      // Make an E_intLit to hold that integer.
      StringRef valueStringRef =
        m_elsaParse.m_stringTable.add(stringbc(value));
      E_intLit *intLit = new E_intLit(valueStringRef);
      intLit->i = value;
      if ((long long)(intLit->i) != value) {
        xunimp("Integer literal too large to represent.");
      }

      ret = intLit;
      break;
    }

    case CXCursor_UnaryOperator: { // 112
      // Get the operator.
      //
      // It seems that examining the tokens is the only way!
      UnaryOp unOp;
      {
        CXSourceLocation cxLoc = clang_getCursorLocation(cxExpr);
        CXToken *cxToken = clang_getToken(m_cxTranslationUnit, cxLoc);
        WrapCXString cxString(clang_getTokenSpelling(m_cxTranslationUnit, *cxToken));
        unOp = stringToUnaryOp(cxString.c_str());
        clang_disposeTokens(m_cxTranslationUnit, cxToken, 1 /*numTokens*/);
      }

      std::vector<CXCursor> children = getChildren(cxExpr);
      xassert(children.size() == 1);
      ret = new E_unary(unOp, importExpression(children.front()));
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

  xunimp(stringb(
    "describeAsStandardConversion: destType='" << destType->toString() <<
    "' srcType='" << srcType->toString() << "'"));
  return SC_IDENTITY;
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
