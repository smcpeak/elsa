// ast_build.h
// Class ElsaASTBuild.

#ifndef AST_BUILD_H
#define AST_BUILD_H

#include "cc_ast.h"                    // C++ AST
#include "elsaparse-fwd.h"             // ElsaParse


// Interface for ElsaASTBuild to obtain a location.
class SourceLocProvider {
public:
  // The default implementation returns SL_UNKNOWN.
  virtual SourceLoc provideLoc() const;
};


// Methods for constructing fragments of the C++ AST from an existing
// semantic description.  This is essentially the reverse of type
// checking, which converts syntax into semantics.
//
// This is meant for use during semantic elaboration and for source
// to source transformation and instrumentation.
//
// My intention is that the constructed AST satisfies the same
// invariants that it would after normal type checking.
//
class ElsaASTBuild {
public:      // data
  // Table for making StringRefs for the AST.
  StringTable &m_stringTable;

  // For making new Type objects.
  TypeFactory &m_typeFactory;

  // Used when an AST node needs a location.
  SourceLocProvider &m_locProvider;

public:      // methods
  ElsaASTBuild(StringTable &stringTable, TypeFactory &tfac,
               SourceLocProvider &locProvider);

  SourceLoc loc() const
    { return m_locProvider.provideLoc(); }

  // wrap an expression in a list
  FakeList<ArgExpression> *makeExprList1(Expression *e);

  // wrap a pair of expressions in a list
  FakeList<ArgExpression> *makeExprList2(Expression *e1, Expression *e2);

  // Return a syntactic ASTTypeId denoting semantic 'type'.
  //
  // 'name' is used as the name of the innermost D_name.  If it is NULL
  // then the declarator is "abstract", meaning it does not declare any
  // name with the denoted type.
  //
  // 'type' is not 'const' because the returned syntax contains a
  // non-const pointer to it.
  ASTTypeId *makeASTTypeId(Type *type, PQName *name = NULL);

  // Make a syntactic exception specification.
  ExceptionSpec *makeExceptionSpec(FunctionType::ExnSpec *srcSpec);

  // Make an E_intLit.
  E_intLit *makeE_intLit(int n);

  PQName *makePQName(Variable *var);

  // Make an E_funCall invoking an unqualified name.
  E_funCall *makeNamedFunCall2(
    Variable *callee, Expression *arg1, Expression *arg2);

  E_binary *makeE_binary(
    Expression *e1, BinaryOp op, Expression *e2);

  E_variable *makeE_variable(Variable *var);
};


// Run tests of this module after parsing.
void test_astbuild(ElsaParse &elsaParse);

#endif // AST_BUILD_H
