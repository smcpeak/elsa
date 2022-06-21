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

  // If true, the string needs to be disposed.
  bool m_valid;

public:      // methods
  WrapCXString(CXString cxString)
    : m_cxString(cxString),
      m_valid(true)
  {}

  // Allow moving, invalidating the source.
  WrapCXString(WrapCXString &&obj)
    : m_cxString(obj.m_cxString),
      m_valid(obj.m_valid)
  {
    obj.m_valid = false;
  }

  ~WrapCXString()
  {
    if (m_valid) {
      clang_disposeString(m_cxString);
    }
  }

  char const *c_str() const
  {
    xassert(m_valid);
    return clang_getCString(m_cxString);
  }
};


class WrapCXTokens {
  NO_OBJECT_COPIES(WrapCXTokens);

public:
  // The TU, since that is needed to dispose and to get the string.
  CXTranslationUnit m_cxTU;

  // Pointer to an array of 'm_numTokens' tokens.
  CXToken *m_cxTokens;

  // Number of tokens in 'm_cxTokens'.
  unsigned m_numTokens;

public:
  // Use 'clang_getToken' to get one token.
  WrapCXTokens(CXTranslationUnit cxTU, CXSourceLocation cxLoc);

  // Use 'clang_tokenize' to get a sequence of tokens.
  WrapCXTokens(CXTranslationUnit cxTU, CXSourceRange cxRange);

  ~WrapCXTokens();

  unsigned size() const { return m_numTokens; }

  // Return the string form of token 'index'.
  WrapCXString stringAt(unsigned index);
};


// Manage the process of importing.
class ImportClang : public SourceLocProvider, ElsaASTBuild {
public:      // data
  // ---- Clang AST ----
  // Container for Clang translation-wide data.
  CXIndex m_cxIndex;

  // Root of the Clang AST.
  CXTranslationUnit m_cxTranslationUnit;

  // ---- Elsa AST ----
  // Container for Elsa translation-wide data.
  ElsaParse &m_elsaParse;

  // Object representing the global scope.  Variables that have global
  // scope will have their 'm_containingScope' set to this.
  Scope *m_globalScope;

  // ---- Maps between Clang and Elsa ----
  // Map from Clang file pointer to its name.
  std::map<CXFile /*cxFile*/, StringRef /*fname*/> m_cxFileToName;

  // Map from SourceLoc of a declaration to its associated Variable.
  std::map<SourceLoc, Variable *> m_locToVariable;

public:      // methods
  ImportClang(CXIndex cxIndex,
              CXTranslationUnit cxTranslationUnit,
              ElsaParse &elsaParse);

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

  StringRef cursorSpelling(CXCursor cxCursor);

  TopForm *importTopForm(CXCursor cxTopForm);

  Declaration *importEnumDefinition(CXCursor cxEnumDefn);

  Enumerator *importEnumerator(CXCursor cxEnumerator,
    EnumType const *enumType);

  Function *importFunctionDefinition(CXCursor cxFuncDefn);

  // Get the Variable that represents the entity declared at 'cxDecl'.
  // 'declFlags' is added to whatever is derived from the Clang AST.
  Variable *variableForDeclaration(CXCursor cxDecl,
    DeclFlags declFlags = DF_NONE);

  Declaration *importVarOrTypedefDecl(CXCursor cxVarDecl,
    DeclaratorContext context);

  Type *importType(CXType cxType);

  EnumType *importEnumType(CXCursor cxEnumDefn);

  S_compound *importCompoundStatement(CXCursor cxFunctionBody);

  Statement *importStatement(CXCursor cxStmt);

  FullExpression *importFullExpression(CXCursor cxExpr);

  // Assert that 'cxNode' has exactly one child, and return it.
  CXCursor getOnlyChild(CXCursor cxNode);

  // Assert that 'cxNode' has two children, and return them.
  void getTwoChildren(CXCursor &c1, CXCursor &c2,
    CXCursor cxNode);

  Expression *importExpression(CXCursor cxExpr);

  // Describe the conversion from 'srcType' to 'destType' as a
  // standard conversion.
  StandardConversion describeAsStandardConversion(
    Type const *destType, Type const *srcType);

  // Make a Variable, and set its 'm_containingScope' according to
  // 'cxDecl'.
  Variable *makeVariable_setScope(SourceLoc loc,
    StringRef name, Type *type, DeclFlags flags, CXCursor cxDecl);

  Variable *makeVariable(SourceLoc loc, StringRef name,
    Type *type, DeclFlags flags);
};


#endif // ELSA_IMPORT_CLANG_INTERNAL_H