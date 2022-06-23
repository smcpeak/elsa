// clang-import-internal.h
// Class definitions for clang-import.cc.

#ifndef ELSA_CLANG_IMPORT_INTERNAL_H
#define ELSA_CLANG_IMPORT_INTERNAL_H

#include "clang-import.h"              // this module

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


// Manage the process of importing.
class ClangImport : public SourceLocProvider, ElsaASTBuild {
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
  ClangImport(CXIndex cxIndex,
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

  Variable *importEnumTypeAsForward(CXCursor cxEnumDecl);

  Declaration *importEnumDefinition(CXCursor cxEnumDefn);

  // Import the CompoundType declared by 'cxCompoundDecl', yielding its
  // typedefVar.  If it has not already been imported, create it as a
  // forward declaration.
  Variable *importCompoundTypeAsForward(CXCursor cxCompoundDecl);

  Declaration *importCompoundTypeDefinition(CXCursor cxCompoundDefn);

  // Return the CV flags applied to the receiver for 'cxMethodDecl'.
  CVFlags importMethodCVFlags(CXCursor cxMethodDecl);

  Function *importFunctionDefinition(CXCursor cxFuncDefn);

  // Get a reference to the Variable pointer for 'cxDecl'.  If the
  // Variable has not yet been created, this returns a reference to a
  // NULL pointer, which the caller is obliged to fill in immediately.
  Variable *&variableRefForDeclaration(CXCursor cxDecl);

  // Get or create the Variable that represents the entity declared at
  // 'cxDecl'.  'declFlags' is added to whatever is derived from the
  // Clang AST.
  Variable *variableForDeclaration(CXCursor cxDecl,
    DeclFlags declFlags = DF_NONE);

  // Get the associated Variable, asserting that it exists.
  Variable *existingVariableForDeclaration(CXCursor cxDecl);

  Declaration *importVarOrTypedefDecl(CXCursor cxVarDecl,
    DeclaratorContext context);

  Type *importType(CXType cxType);

  // Import 'cxType' as a type.  If 'methodClass' is not NULL, and
  // 'cxType' is a function type, then return a method type with a
  // receiver that is a 'methodCV' reference to 'methodClass'.
  Type *importType_maybeMethod(CXType cxType,
    CompoundType const * NULLABLE methodClass, CVFlags methodCV);

  // Add the receiver parameter to 'methodType'.
  void addReceiver(FunctionType *methodType,
    SourceLoc loc, CompoundType const *containingClass, CVFlags cv);

  S_compound *importCompoundStatement(CXCursor cxFunctionBody);

  Statement *importStatement(CXCursor cxStmt);

  FullExpression *importFullExpression(CXCursor cxExpr);

  // Assert that 'cxNode' has exactly one child, and return it.
  CXCursor getOnlyChild(CXCursor cxNode);

  // Assert that 'cxNode' has two children, and return them.
  void getTwoChildren(CXCursor &c1, CXCursor &c2,
    CXCursor cxNode);

  long long evalAsLongLong(CXCursor cxExpr);

  Expression *importExpression(CXCursor cxExpr);

  // Describe the conversion from 'srcType' to 'destType' as a
  // standard conversion.
  StandardConversion describeAsStandardConversion(
    Type const *destType, Type const *srcType);

  Condition *importCondition(CXCursor cxCond);

  // Import a variable declaration in a context where the Elsa AST
  // represents it as an ASTTypeId.
  ASTTypeId *importASTTypeId(CXCursor cxDecl,
    DeclaratorContext context);

  Initializer *importInitializer(CXCursor cxInit);

  Handler *importHandler(CXCursor cxHandler);

  // Make a Variable, and set its 'm_containingScope' according to
  // 'cxDecl'.
  Variable *makeVariable_setScope(SourceLoc loc,
    StringRef name, Type *type, DeclFlags flags, CXCursor cxDecl);

  Variable *makeVariable(SourceLoc loc, StringRef name,
    Type *type, DeclFlags flags);

  DeclFlags importStorageClass(CX_StorageClass storageClass);

  AccessKeyword importAccessKeyword(
    CX_CXXAccessSpecifier accessSpecifier);

  // Debug print.
  void maybePrintType(char const *label, CXType cxType);
  void printSubtree(CXCursor cursor, int indent);
};


#endif // ELSA_CLANG_IMPORT_INTERNAL_H
