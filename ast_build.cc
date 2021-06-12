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

  // This is not really safe, but will suffice for my immediate purpose.
  ret->i = (unsigned long)n;

  ret->type = m_typeFactory.getSimpleType(ST_INT);

  return ret;
}


PQ_name *ElsaASTBuild::makePQ_name(char const *name)
{
  return new PQ_name(loc(), m_stringTable.add(name));
}


// EOF
