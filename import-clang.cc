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


TopForm *ImportClang::importTopForm(CXCursor cxTopForm)
{
  CXSourceLocation cxLoc = clang_getCursorLocation(cxTopForm);
  SourceLoc loc = importSourceLocation(cxLoc);

  CXCursorKind cxKind = clang_getCursorKind(cxTopForm);
  switch (cxKind) {
    default:
      xfailure(stringb("Unknown cxKind: " << cxKind));

    case CXCursor_VarDecl: {
      Declaration *decl = importVarDecl(cxTopForm, DC_TF_DECL);
      return new TF_decl(loc, decl);
    }
  }

  // Not reached.
}


Declaration *ImportClang::importVarDecl(CXCursor cxVarDecl,
  DeclaratorContext context)
{
  Type *type = importType(clang_getCursorType(cxVarDecl));
  StringRef name = importString(clang_getCursorSpelling(cxVarDecl));
  Variable *var = makeVariable(
    importSourceLocation(clang_getCursorLocation(cxVarDecl)),
    name,
    type,
    DF_NONE);      // TODO: Wrong

  return makeDeclaration(var, context);
}


Type *ImportClang::importType(CXType cxType)
{
  switch (cxType.kind) {
    default:
      xfailure(stringb("Unknown cxType kind: " << cxType.kind));

    case CXType_Int:
      return m_elsaParse.m_typeFactory.getSimpleType(ST_INT, CV_NONE);
  }

  // Not reached.
}



Variable *ImportClang::makeVariable(SourceLoc loc, StringRef name,
  Type *type, DeclFlags flags)
{
  return m_elsaParse.m_typeFactory.makeVariable(loc, name, type, flags);
}


// EOF
