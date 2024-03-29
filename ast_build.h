// ast_build.h
// Class ElsaASTBuild.

#ifndef AST_BUILD_H
#define AST_BUILD_H

#include "cc-ast.h"                    // C++ AST
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

  // Language options.  One reason this is needed is when creating a
  // function declarator, we use a single 'void' parameter to indicate
  // the absence of parameters in C only.
  CCLang const &m_lang;

  // Used when an AST node needs a location.
  SourceLocProvider &m_locProvider;

public:      // methods
  ElsaASTBuild(StringTable &stringTable, TypeFactory &tfac,
               CCLang const &lang, SourceLocProvider &locProvider);

  SourceLoc loc() const
    { return m_locProvider.provideLoc(); }

  // wrap an expression in a list
  FakeList<ArgExpression> *makeExprList1(Expression *e);

  // Wrap multiple expressions in a list.
  FakeList<ArgExpression> *makeExprList2(Expression *e1, Expression *e2);
  FakeList<ArgExpression> *makeExprList3(
    Expression *e1, Expression *e2, Expression *e3);
  FakeList<ArgExpression> *makeExprList4(
    Expression *e1, Expression *e2, Expression *e3, Expression *e4);

  // Given a Variable, create a D_name or D_bitfield that refers to it.
  // This D_name must have additional IDeclarators wrapped around it in
  // order to properly express the type of 'var'.
  IDeclarator *makeInnermostDeclarator(Variable *var);

  // Stack a D_func on top of 'base' that expresses that 'base' has
  // type 'ftype', except that the return type of 'ftype' is ignored
  // because that is not part of the function declarator syntax.
  D_func *makeD_func(FunctionType const *ftype, IDeclarator *base);

  // Given a base declarator (which should be a D_name), add more
  // IDeclarators on top of it in order to denote 'type'.  Stop when we
  // reach a TypedefType or an atomic type, returning it.
  Type const *buildUpDeclarator(
    Type const *type, IDeclarator *&idecl /*INOUT*/);

  // Return the lvalue version of 'type'.  Normally that means wrapping
  // it in a ReferenceType, but if it is already a ReferenceType then
  // we return 'type' itself.
  Type const *makeLvalueType(Type const *type);

  // Given a Variable representing a typedef'd type, construct a type
  // specifier that uses it.
  TS_name *makeTS_name(Variable *typedefVar);

  // Express 'type' as a type specifier.
  //
  // Requires: 'type->isCVAtomicType()', but that could be by way of a
  // TypedefType.
  TypeSpecifier *makeTypeSpecifier(Type const *type);

  // Create a class definition.
  TS_classSpec *makeTS_classSpec(CompoundType const *ct);

  // Given 'var', return a TypeSpecifier and a Declarator (both as owner
  // pointers) that declares that variable.
  std::pair<TypeSpecifier*, Declarator*>
    makeTSandDeclarator(Variable *var, DeclaratorContext context);

  // Given 'var' and 'type', return a TypeSpecifier and a Declarator
  // (both as owner pointers) that declares that variable with the given
  // type.
  //
  // Whereas 'makeTSandDeclarator' uses 'var->type', this takes an
  // explicit type, which is needed for the case of a function that has
  // a prototype, since 'var->type' will have parameter names that come
  // from the prototype, whereas 'type' can have those from the
  // definition.
  //
  std::pair<TypeSpecifier*, Declarator*>
    makeTSandDeclaratorForType(Variable *var, Type const *type,
                               DeclaratorContext context);

  // Return a syntactic ASTTypeId denoting semantic 'type'.
  //
  // 'name' is used as the name of the innermost D_name.  If it is NULL
  // then the declarator is "abstract", meaning it does not declare any
  // name with the denoted type.
  //
  // 'type' is not 'const' because the returned syntax contains a
  // non-const pointer to it.
  ASTTypeId *makeASTTypeId(Type const *type, PQName * NULLABLE name,
    DeclaratorContext context);

  // Make an ASTTypeId denoting a SimpleTypeId.
  ASTTypeId *makeSimpleTypeTypeId(SimpleTypeId tid);

  // Return an ASTTypeId to represent the parameter denoted by 'var'.
  ASTTypeId *makeParameter(Variable *var);

  // Return a declaration for a variable.
  Declaration *makeDeclaration(Variable *var, DeclaratorContext context);

  // Make a syntactic exception specification.
  ExceptionSpec *makeExceptionSpec(FunctionType::ExnSpec *srcSpec);

  PQName *makePQName(Variable *var);

  // Make an E_intLit.
  E_intLit *makeE_intLit(int n);

  // Make an E_stringLit that denotes 'text'.
  E_stringLit *makeE_stringLit(char const *text);

  E_variable *makeE_variable(Variable *var);

  E_funCall *makeE_funCall(
    Expression *func, FakeList<ArgExpression> *args);

  // Make an E_funCall invoking an unqualified name.
  E_funCall *makeNamedFunCall0(
    Variable *callee);
  E_funCall *makeNamedFunCall1(
    Variable *callee, Expression *arg1);
  E_funCall *makeNamedFunCall2(
    Variable *callee, Expression *arg1, Expression *arg2);
  E_funCall *makeNamedFunCall3(
    Variable *callee, Expression *arg1, Expression *arg2, Expression *arg3);
  E_funCall *makeNamedFunCall4(
    Variable *callee, Expression *arg1, Expression *arg2, Expression *arg3, Expression *arg4);

  E_fieldAcc *makeE_fieldAcc(Expression *obj, Variable *field);

  E_sizeof *makeE_sizeof(Expression *expr);

  E_unary *makeE_unary(UnaryOp op, Expression *expr);

  E_effect *makeE_effect(EffectOp op, Expression *expr);

  // Compute the type that an E_binary should have with the given
  // children.
  Type const *typeForE_binary(
    Expression *e1, BinaryOp op, Expression *e2);

  E_binary *makeE_binary(
    Expression *e1, BinaryOp op, Expression *e2);

  // Same as 'makeE_binary(e1, BIN_COMMA, e2)'.
  E_binary *makeCommaExpr(Expression *e1, Expression *e2);

  // Convenience forms with more expressions.
  E_binary *makeCommaExpr3(Expression *e1, Expression *e2, Expression *e3);
  E_binary *makeCommaExpr4(Expression *e1, Expression *e2, Expression *e3, Expression *e4);

  E_addrOf *makeE_addrOf(Expression *expr);

  E_deref *makeE_deref(Expression *ptr);

  E_cast *makeE_cast(Type const *type, Expression *src);

  E_cond *makeE_cond(Expression *cond, Expression *th, Expression *el);

  E_assign *makeE_assign(
    Expression *target, BinaryOp op, Expression *src);

  // Make an ordinary assignment of a variable.
  E_assign *makeVarAssign(Variable *target, Expression *src);

  E_sizeofType *makeE_sizeofType(Type const *type);

  E_offsetof *makeE_offsetof(Type const *structType, Variable *field);

#ifdef GNU_EXTENSION
  E_compoundLit *makeE_compoundLit(ASTTypeId *typeId, IN_compound *init);
  E_compoundLit *makeE_compoundLit(Type const *type, IN_compound *init);

  E___builtin_va_arg *makeE___builtin_va_arg(SourceLoc loc,
    Expression *expr, ASTTypeId *atype);
#endif // GNU_EXTENSION

  S_expr *makeS_expr(SourceLoc loc, Expression *expr);

  FieldDesignator *makeFieldDesignator(SourceLoc loc,
    Variable const *var);
};


// Run tests of this module after parsing.
// Defined in test-astbuild.cc.
void test_astbuild(ElsaParse &elsaParse);


// Run a different test that makes and prints synthetic AST.
void test_print_astbuild();


#endif // AST_BUILD_H
