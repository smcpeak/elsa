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
#include "clang/AST/Decl.h"                                // NamedDecl, FunctionDecl, etc.
#include "clang/AST/DeclBase.h"                            // Decl
#include "clang/AST/Expr.h"                                // Expr
#include "clang/AST/Stmt.h"                                // Stmt, CompoundStmt, etc.
#include "clang/AST/Type.h"                                // QualType, FunctionType, Qualifiers, etc.
#include "clang/Basic/LLVM.h"                              // StringRef
#include "clang/Basic/SourceLocation.h"                    // FileID, SourceLocation
#include "clang/Basic/Specifiers.h"                        // StorageClass
#include "clang/Frontend/ASTUnit.h"                        // ASTUnit
#include "clang-c/Index.h"                                 // libclang (TODO: remove)

// libc++
#include <map>                         // std::map
#include <memory>                      // std::unique_ptr


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


// Children of a node, partitioned by node kind.
class PartitionedChildren {
public:      // data
  // 'clang_isDeclaration' is true.
  std::vector<CXCursor> m_declarations;

  // 'clang_isExpression' is true.
  std::vector<CXCursor> m_expressions;

  // 'clang_isStatement' is true.  In libclang, expressions are *not*
  // considered to be statements.
  std::vector<CXCursor> m_statements;

  // 'clang_isAttribute' is true.
  std::vector<CXCursor> m_attributes;

public:
  // Populate with children of 'cursor'.
  PartitionedChildren(CXCursor cursor);

  // True if all vectors are empty.
  //
  // The idea is, as each list is consumed, the client should clear that
  // list.  Then at the end it can check that the children are empty.
  bool empty() const;
};


// Manage the process of importing.
class ClangImport : public SourceLocProvider, ElsaASTBuild {
public:      // data
  // ---- Clang AST ----
  // Root of the Clang AST.
  std::unique_ptr<clang::ASTUnit> m_clangASTUnit;

  // ---- Elsa AST ----
  // Container for Elsa translation-wide data.
  ElsaParse &m_elsaParse;

  // Convenient access to the type factory 'm_elsaParse.m_typeFactory'.
  TypeFactory &m_tfac;

  // Object representing the global scope.  Variables that have global
  // scope will have their 'm_containingScope' set to this.
  Scope *m_globalScope;

  // ---- Maps between Clang and Elsa ----
  // Map from Clang file ID to its name.
  std::map<clang::FileID /*clangFileID*/,
           StringRef /*fname*/> m_clangFileIDToName;

  // Map from SourceLoc of a declaration to its associated Variable.
  std::map<SourceLoc, Variable *> m_locToVariable;

public:      // methods
  ClangImport(std::unique_ptr<clang::ASTUnit> &&clangASTUnit,
              ElsaParse &elsaParse);

  // Entry point to the importer.
  void importTranslationUnit();

  StringRef importStringRef(clang::StringRef clangString);

  // Map 'clangFileID' to a file name, using a map to remember entries
  // already seen.
  StringRef importFileName(clang::FileID clangFileID);

  SourceLoc importSourceLocation(clang::SourceLocation clangLoc);

  SourceLoc declLocation(clang::Decl const *clangDecl);

  SourceLoc stmtLocation(clang::Stmt const *clangStmt);

  SourceLoc exprLocation(clang::Expr const *clangExpr);

  // Get the location of 'cxCursor'.
  SourceLoc cursorLocation(CXCursor cxCursor);

  StringRef cursorSpelling(CXCursor cxCursor);

  TopForm *importTopForm(clang::Decl const *clangTopFormDecl);

  //Declaration *importDeclaration(CXCursor cxDecl,
  //  DeclaratorContext context);

  Variable *importEnumTypeAsForward(CXCursor cxEnumDecl);

  Declaration *importEnumDefinition(CXCursor cxEnumDefn);

  // Import the CompoundType declared by 'cxCompoundDecl', yielding its
  // typedefVar.  If it has not already been imported, create it as a
  // forward declaration.
  Variable *importCompoundTypeAsForward(CXCursor cxCompoundDecl);

  Declaration *importCompoundTypeDefinition(CXCursor cxCompoundDefn);

  Declaration *importCompoundTypeForwardDeclaration(
    CXCursor cxCompoundDecl);

  // Return the CV flags applied to the receiver for 'clangCXXMethodDecl'.
  CVFlags importMethodQualifiers(
    clang::CXXMethodDecl const *clangCXXMethodDecl);

  Function *importFunctionDefinition(
    clang::FunctionDecl const *clangFunctionDecl);

  // Get a reference to the Variable pointer for 'clangNamedDecl'.  If
  // the Variable has not yet been created, this returns a reference to
  // a NULL pointer, which the caller is obliged to fill in immediately.
  Variable *&variableRefForDeclaration(clang::NamedDecl const *clangNamedDecl);

  // Get or create the Variable that represents the entity declared at
  // 'clangNamedDecl'.  'declFlags' is added to whatever is derived from
  // the Clang AST.
  Variable *variableForDeclaration(clang::NamedDecl const *clangNamedDecl,
    DeclFlags declFlags = DF_NONE);

  // Get the associated Variable, asserting that it exists.
  Variable *existingVariableForDeclaration(
    clang::NamedDecl const *clangNamedDecl);

  Declaration *importNamedDecl(
    clang::NamedDecl const *clangNamedDecl,
    DeclaratorContext context);

  // If 'clangNamedDecl' can have a storage class, get it and return
  // that within DeclFlags.  Otherwise, return DF_NONE.
  DeclFlags declStorageClass(clang::NamedDecl const *clangNamedDecl);

  // Possibly add qualifiers to 'declarator', and also get the storage
  // class for 'clangNamedDecl' as would be used at its declaration site
  // (which depends on the presence of qualifiers).
  DeclFlags possiblyAddNameQualifiers_and_getStorageClass(
    Declarator *declarator, clang::NamedDecl const *clangNamedDecl);

  // If 'clangNamedDecl' requires qualifiers, add them to 'declarator',
  // which already has an unqualified name.  Return true if any
  // qualifiers were added.
  bool possiblyAddNameQualifiers(Declarator *declarator,
    clang::NamedDecl const *clangNamedDecl);

  // Get the identifier name of 'clangDeclContext', (for now) throwing
  // an exception if it does not have one.
  StringRef declContextName(clang::DeclContext const *clangDeclContext);

  // Given a set of attributes associated with 'declarator', parse them
  // and attach their parsed forms to 'declarator'.
  void importDeclaratorAttributes(Declarator *declarator,
    std::vector<CXCursor> const &cxAttributes);

  Attribute *importAttribute(CXCursor cxAttribute);

  // Import a GNU-syntax attribute described by the Clang
  // 'prettyAttrString'.  The Clang-reported attribute name is
  // 'attrName'.
  Attribute *importGNUAttribute(SourceLoc loc,
    char const *attrName, char const *prettyAttrString);

  // Import a single argument to an attribute, such as '"name"' in
  // '__attribute__((alias("name")))'.  The argument is expressed as its
  // C token syntax.
  //
  // Although the result is described as an Expression, currently that
  // Expression only contains syntax, without any of the semantic
  // annotations that the Elsa type checker would add.
  Expression *importAttributeArgument(SourceLoc loc,
    StringRef arg);

  CVFlags importQualifiers(clang::Qualifiers clangQualifiers);

  Type *importQualType(clang::QualType clangQualType);

  // Import 'clangQualType' as a type.  If 'methodClass' is not NULL,
  // and 'clangQualType' is a function type, then return a method type
  // with a receiver that is a 'methodCV' reference to 'methodClass'.
  Type *importQualType_maybeMethod(clang::QualType clangQualType,
    CompoundType const * NULLABLE methodClass, CVFlags methodCV);

  // Like above, but specific to the function type case.
  FunctionType *importFunctionType(
    clang::FunctionType const *clangFunctionType,
    CompoundType const * NULLABLE methodClass, CVFlags methodCV);

  // Add the receiver parameter to 'methodType'.
  void addReceiver(FunctionType *methodType,
    SourceLoc loc, CompoundType const *containingClass, CVFlags cv);

  ArrayType *importArrayType(CXType cxArrayType);

  S_compound *importCompoundStatement(
    clang::CompoundStmt const *clangCompoundStmt);

  Statement *importStatement(clang::Stmt const *clangStmt);

  FullExpression *importFullExpression(clang::Expr const *clangExpr);

  // Assert that 'cxNode' has exactly one child, and return it.
  CXCursor getOnlyChild(CXCursor cxNode);

  // Assert that 'cxNode' has two children, and return them.
  void getTwoChildren(CXCursor &c1, CXCursor &c2,
    CXCursor cxNode);

  long long evalAsLongLong(CXCursor cxExpr);

  std::string evalAsString(CXCursor cxExpr);

  Expression *importExpression(clang::Expr const *clangExpr);

  E_stringLit *importStringLiteral(CXCursor cxExpr);

  E_charLit *importCharacterLiteral(CXCursor cxExpr);

  Expression *importUnaryOperator(
    clang::UnaryOperator const *clangUnaryOperator);

  // Determine if 'clangExpr' is any of the special forms described by
  // the 'SpecialExpr' enumeration.
  SpecialExpr getSpecialExpr(clang::Expr const *clangExpr);

  // Describe the conversion from 'srcType' to 'destType' as a
  // standard conversion.
  StandardConversion describeAsStandardConversion(
    Type const *destType, Type const *srcType, SpecialExpr srcSpecial);

  Condition *importCondition(CXCursor cxCond);

  // Import a variable declaration in a context where the Elsa AST
  // represents it as an ASTTypeId.
  ASTTypeId *importASTTypeId(CXCursor cxDecl,
    DeclaratorContext context);

  Initializer *importInitializer(clang::Expr const *clangInitExpr);

  Handler *importHandler(CXCursor cxHandler);

  // Make a Variable, and set its 'm_containingScope' according to
  // 'clangNamedDecl'.
  Variable *makeVariable_setScope(SourceLoc loc,
    StringRef name, Type *type, DeclFlags flags,
    clang::NamedDecl const *clangNamedDecl);

  Variable *makeVariable(SourceLoc loc, StringRef name,
    Type *type, DeclFlags flags);

  StringRef addStringRef(char const *str);

  DeclFlags importStorageClass(clang::StorageClass storageClass);

  AccessKeyword importAccessKeyword(
    CX_CXXAccessSpecifier accessSpecifier);

  // Debug print.
  void printSubtree(CXCursor cursor);
};


#endif // ELSA_CLANG_IMPORT_INTERNAL_H
