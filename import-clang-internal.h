// import-clang-internal.h
// Class definitions for import-clang.cc.

#ifndef ELSA_IMPORT_CLANG_INTERNAL_H
#define ELSA_IMPORT_CLANG_INTERNAL_H

#include "import-clang.h"              // this module

// smbase
#include "sm-macros.h"                 // NO_OBJECT_COPIES
#include "srcloc.h"                    // SourceLoc
#include "strtable.h"                  // StringRef

// elsa
#include "ast_build.h"                 // ElsaASTBuild
#include "cc.ast.gen.h"                // Elsa AST
#include "elsaparse.h"                 // ElsaParse

// clang
#include "clang-c/Index.h"             // libclang

// libc++
#include <map>                         // std::map


// Dispose the index upon scope exit.
class DisposeCXIndex {
  NO_OBJECT_COPIES(DisposeCXIndex);

public:      // data
  CXIndex m_cxIndex;

public:      // methods
  DisposeCXIndex(CXIndex cxIndex)
    : m_cxIndex(cxIndex)
  {}

  ~DisposeCXIndex()
  {
    clang_disposeIndex(m_cxIndex);
  }
};


// Dispose the translation unit upon scope exit.
class DisposeCXTranslationUnit {
  NO_OBJECT_COPIES(DisposeCXTranslationUnit);

public:      // data
  CXTranslationUnit m_cxTranslationUnit;

public:      // methods
  DisposeCXTranslationUnit(CXTranslationUnit cxTranslationUnit)
    : m_cxTranslationUnit(cxTranslationUnit)
  {}

  ~DisposeCXTranslationUnit()
  {
    clang_disposeTranslationUnit(m_cxTranslationUnit);
  }
};


// Dispose a CXString when done, and in the meantime, provide access to
// the C string data.
class WrapCXString {
  NO_OBJECT_COPIES(WrapCXString);

public:      // data
  // The Clang string reference.
  CXString m_cxString;

public:      // methods
  WrapCXString(CXString cxString)
    : m_cxString(cxString)
  {}

  ~WrapCXString()
  {
    clang_disposeString(m_cxString);
  }

  char const *c_str() const
  {
    return clang_getCString(m_cxString);
  }
};


// Manage the process of importing.
class ImportClang : public SourceLocProvider, ElsaASTBuild {
public:      // data
  // Container for Clang translation-wide data.
  CXIndex m_cxIndex;

  // Root of the AST.
  CXTranslationUnit m_cxTranslationUnit;

  // Container for Elsa translation-wide data.
  ElsaParse &m_elsaParse;

  // Map from Clang file pointer to its name.
  std::map<CXFile /*cxFile*/, StringRef /*fname*/> m_cxFileToName;

  // Map from SourceLoc of a declaration to its associated Variable.
  std::map<SourceLoc, Variable *> m_locToVariable;

public:      // methods
  ImportClang(CXIndex cxIndex,
              CXTranslationUnit cxTranslationUnit,
              ElsaParse &elsaParse)
    : ElsaASTBuild(elsaParse.m_stringTable,
                   elsaParse.m_typeFactory,
                   elsaParse.m_lang,
                   *this),
      m_cxIndex(cxIndex),
      m_cxTranslationUnit(cxTranslationUnit),
      m_elsaParse(elsaParse),
      m_cxFileToName(),
      m_locToVariable()
  {}

  // Entry point to the importer.
  void importTranslationUnit();

  // Get the direct children of the node at 'cursor'.
  std::vector<CXCursor> getChildren(CXCursor cursor);

  // Convert a CXString to a StringRef.  This disposes 'cxString'.
  StringRef importString(CXString cxString);

  // Map 'cxFile' to a file name, using a map to remember entries
  // already seen.
  StringRef importFileName(CXFile cxFile);

  SourceLoc importSourceLocation(CXSourceLocation cxLoc);

  // Get the location of 'cxCursor'.
  SourceLoc cursorLocation(CXCursor cxCursor);

  TopForm *importTopForm(CXCursor cxTopForm);

  Function *importFunctionDefinition(CXCursor cxFuncDefn);

  // Get the Variable that represents the entity declared at 'cxDecl'.
  // 'declFlags' is added to whatever is derived from the Clang AST.
  Variable *variableForDeclaration(CXCursor cxDecl,
    DeclFlags declFlags = DF_NONE);

  Declaration *importVarOrTypedefDecl(CXCursor cxVarDecl,
    DeclaratorContext context);

  Type *importType(CXType cxType);

  S_compound *importCompoundStatement(CXCursor cxFunctionBody);

  Statement *importStatement(CXCursor cxStmt);

  FullExpression *importFullExpression(CXCursor cxExpr);

  Expression *importExpression(CXCursor cxExpr);

  // Construct Elsa AST.
  Variable *makeVariable(SourceLoc loc, StringRef name,
    Type *type, DeclFlags flags);
};


#endif // ELSA_IMPORT_CLANG_INTERNAL_H
