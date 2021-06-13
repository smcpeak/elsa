// ast_build.h
// code for ast_build.cc

#include "ast_build.h"      // this module


SourceLoc SourceLocProvider::provideLoc() const
{
  return SL_UNKNOWN;
}


ElsaASTBuild::ElsaASTBuild(StringTable &stringTable, TypeFactory &tfac,
                           SourceLocProvider &locProvider)
  : m_stringTable(stringTable),
    m_typeFactory(tfac),
    m_locProvider(locProvider)
{}


FakeList<ArgExpression> *ElsaASTBuild::makeExprList1(Expression *e)
{
  return FakeList<ArgExpression>::makeList(new ArgExpression(e));
}

FakeList<ArgExpression> *ElsaASTBuild::makeExprList2(Expression *e1, Expression *e2)
{
  return fl_prepend(makeExprList1(e2), new ArgExpression(e1));
}


ASTTypeId *ElsaASTBuild::makeASTTypeId(Type *type)
{
  Declarator *decl = new Declarator(new D_name(loc(), NULL /*name*/),
                                    NULL /*init*/);

  // This is not necessary for my current purpose (instrumentation with
  // pretty-print), but is easy to do.
  decl->type = type;

  return new ASTTypeId(new TS_type(loc(), type), decl);
}


E_intLit *ElsaASTBuild::makeE_intLit(int n)
{
  E_intLit *ret = new E_intLit(m_stringTable.add(stringb(n).c_str()));

  // TODO: This is not really safe, but will suffice for my immediate purpose.
  ret->i = (unsigned long)n;

  ret->type = m_typeFactory.getSimpleType(ST_INT);

  return ret;
}


PQName *ElsaASTBuild::makePQName(Variable *var)
{
  // TODO: This is only right when the name is unqualified.
  return new PQ_name(loc(), var->name);
}


E_funCall *ElsaASTBuild::makeNamedFunCall2(
  Variable *callee, Expression *arg1, Expression *arg2)
{
  FakeList<ArgExpression> *args = makeExprList2(arg1, arg2);
  E_funCall *call = new E_funCall(
    makeE_variable(callee),
    args);
  call->type = callee->type->asRval()->asFunctionType()->retType;
  return call;
}


E_binary *ElsaASTBuild::makeE_binary(
  Expression *e1, BinaryOp op, Expression *e2)
{
  return new E_binary(e1, op, e2);
}


E_variable *ElsaASTBuild::makeE_variable(Variable *var)
{
  E_variable *evar = new E_variable(makePQName(var));
  evar->var = var;
  evar->type = var->type;
  return evar;
}


// EOF
