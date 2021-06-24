// ast_build.h
// Class ElsaASTBuild.

#ifndef AST_BUILD_H
#define AST_BUILD_H

#include "cc_ast.h"                    // C++ AST
#include "elsaparse-fwd.h"             // ElsaParse

#include <utility>                     // std::pair


// Interface for ElsaASTBuild to obtain a location.
class SourceLocProvider {
public:      // methods
  // The default implementation returns SL_UNKNOWN.
  virtual SourceLoc provideLoc() const;
};


// Implements SourceLocProvider by returning a member variable.
class MemberSourceLocProvider : public SourceLocProvider {
public:      // data
  // The value to return.
  SourceLoc m_locForProvider;

public:      // methods
  explicit MemberSourceLocProvider(SourceLoc locForProvider);

  SourceLoc provideLoc() const override;
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

  // HACK: When true, which is the default, use the 'typedefVar' carried
  // by NamedAtomicType to name such a type, rather than the elaborated
  // name (with the 'class' or whatever keyword).
  //
  // This results in more compact code when printed, but is sometimes
  // WRONG because I don't actually know whether the typedef is
  // accessible (or even exists, in the case of C!).
  bool m_useTypedefsForNamedAtomics;

public:      // methods
  ElsaASTBuild(StringTable &stringTable, TypeFactory &tfac,
               SourceLocProvider &locProvider);

  SourceLoc loc() const
    { return m_locProvider.provideLoc(); }

  // wrap an expression in a list
  FakeList<ArgExpression> *makeExprList1(Expression *e);

  // wrap a pair of expressions in a list
  FakeList<ArgExpression> *makeExprList2(Expression *e1, Expression *e2);

  // Given a Variable, create a D_name that refers to it.  This D_name
  // must have additional IDeclarators wrapped around it in order to
  // properly express the type of 'var'.
  D_name *makeD_name(Variable *var);

  // Stack a D_func on top of 'base' that expresses that 'base' has
  // type 'ftype', except that the return type of 'ftype' is ignored
  // because that is not part of the function declarator syntax.
  D_func *makeD_func(FunctionType const *ftype, IDeclarator *base);

  // Given a base declarator (which should be a D_name), add more
  // IDeclarators on top of it in order to denote 'type'.  Stop when we
  // reach an atomic type, returning it.
  //
  // Although the returned value is known to be CVAtomicType, it could
  // have TypedefType wrapped around that, which I do not want to lose,
  // so it is declared as simply 'Type'.
  //
  // Ensures: 'return->isCVAtomicType()'.
  Type const *buildUpDeclarator(
    Type const *type, IDeclarator *&idecl /*INOUT*/);

  // Given a Variable representing a typedef'd type, construct a type
  // specifier that uses it.
  TS_name *makeTS_name(Variable *typedefVar);

  // Express 'type' as a type specifier.
  //
  // Requires: 'type->isCVAtomicType()', but that could be by way of a
  // TypedefType.
  TypeSpecifier *makeTypeSpecifier(Type const *type);

  // Given 'var', return a TypeSpecifier and a Declarator (both as owner
  // pointers) that declares that variable.
  std::pair<TypeSpecifier*, Declarator*>
    makeTSandDeclarator(Variable *var, DeclaratorContext context);

  // Return a syntactic ASTTypeId denoting semantic 'type'.
  //
  // 'name' is used as the name of the innermost D_name.  If it is NULL
  // then the declarator is "abstract", meaning it does not declare any
  // name with the denoted type.
  //
  // 'type' is not 'const' because the returned syntax contains a
  // non-const pointer to it.
  ASTTypeId *makeASTTypeId(Type *type, PQName *name,
    DeclaratorContext context);

  // Return an ASTTypeId to represent the parameter denoted by 'var'.
  ASTTypeId *makeParameter(Variable *var);

  // Return a declaration for a variable.
  Declaration *makeDeclaration(Variable *var, DeclaratorContext context);

  // Make a syntactic exception specification.
  ExceptionSpec *makeExceptionSpec(FunctionType::ExnSpec *srcSpec);

  // Make an E_intLit.
  E_intLit *makeE_intLit(int n);

  PQName *makePQName(Variable *var);

  // Make an E_funCall invoking an unqualified name.
  E_funCall *makeNamedFunCall1(
    Variable *callee, Expression *arg1);
  E_funCall *makeNamedFunCall2(
    Variable *callee, Expression *arg1, Expression *arg2);

  E_binary *makeE_binary(
    Expression *e1, BinaryOp op, Expression *e2);

  E_variable *makeE_variable(Variable *var);

  E_assign *makeE_assign(
    Expression *target, BinaryOp op, Expression *src);

  E_cast *makeE_cast(Type *type, Expression *src);

  E_deref *makeE_deref(Expression *ptr);
};


// Run tests of this module after parsing.
void test_astbuild(ElsaParse &elsaParse);

#endif // AST_BUILD_H
