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


TopForm *ImportClang::importTopForm(CXCursor cxTopForm)
{
  SourceLoc loc = cursorLocation(cxTopForm);

  CXCursorKind cxKind = clang_getCursorKind(cxTopForm);
  switch (cxKind) {
    default:
      xfailure(stringb("Unknown cxKind: " << cxKind));

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

  StringRef name = importString(clang_getCursorSpelling(cxDecl));

  Variable *&var = m_locToVariable[loc];
  if (!var) {
    Type *type;

    if (clang_getCursorKind(cxDecl) == CXCursor_TypedefDecl) {
      declFlags |= DF_TYPEDEF;

      // For a typedef, the 'type' is the underlying type.
      type = importType(clang_getTypedefDeclUnderlyingType(cxDecl));
    }
    else {
      // For anything else, the type is what the declaration says.
      type = importType(clang_getCursorType(cxDecl));
    }

    xassert(!var);
    var = makeVariable(loc, name, type, declFlags);
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

  switch (cxType.kind) {
    default:
      xfailure(stringb("Unknown cxType kind: " << cxType.kind));

    case CXType_Invalid:
      xfailure("importType: cxType is invalid");

    case CXType_Int:
      return tfac.getSimpleType(ST_INT, CV_NONE);

    case CXType_Typedef: {
      CXCursor cxTypeDecl = clang_getTypeDeclaration(cxType);
      Variable *typedefVar = variableForDeclaration(cxTypeDecl);

      if (clang_isConstQualifiedType(cxType) ||
          clang_isVolatileQualifiedType(cxType) ||
          clang_isRestrictQualifiedType(cxType)) {
        xunimp("cv qualifiers on typedef");
      }

      return tfac.makeTypedefType(typedefVar, CV_NONE);
    }

    case CXType_FunctionProto: {
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
  }

  // Not reached.
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

    case CXCursor_DeclStmt: {
      xassert(children.size() == 1);
      for (CXCursor const &child : children) {
        xassert(clang_getCursorKind(child) == CXCursor_VarDecl);
        Declaration *decl = importVarOrTypedefDecl(child, DC_S_DECL);
        return new S_decl(loc, decl);
      }
    }

    case CXCursor_ReturnStmt: {
      FullExpression *retval = nullptr;
      if (!children.empty()) {
        xassert(children.size() == 1);
        retval = importFullExpression(children.front());
      }
      return new S_return(loc, retval);
    }
  }

  // Not reached.
}


FullExpression *ImportClang::importFullExpression(CXCursor cxExpr)
{
  return new FullExpression(importExpression(cxExpr));
}


Expression *ImportClang::importExpression(CXCursor cxExpr)
{
  CXCursorKind exprKind = clang_getCursorKind(cxExpr);
  Type const *type = importType(clang_getCursorType(cxExpr));
  std::vector<CXCursor> children = getChildren(cxExpr);

  switch (exprKind) {
    default:
      xunimp(stringb("Unhandled exprKind: " << exprKind));

    case CXCursor_UnexposedExpr: {
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
        xunimp("non-trivial implicit conversion");
        return nullptr;
      }
    }

    case CXCursor_DeclRefExpr: {
      CXCursor decl = clang_getCursorReferenced(cxExpr);
      Variable *var = variableForDeclaration(decl);
      E_variable *evar = makeE_variable(var);
      evar->type = legacyTypeNC(type);
      return evar;
    }
  }

  // Not reached.
}


Variable *ImportClang::makeVariable(SourceLoc loc, StringRef name,
  Type *type, DeclFlags flags)
{
  return m_elsaParse.m_typeFactory.makeVariable(loc, name, type, flags);
}


// EOF
