// ast_build.h
// code for ast_build.cc

#include "ast_build.h"      // this module


FakeList<ArgExpression> *makeExprList1(Expression *e)
{
  return FakeList<ArgExpression>::makeList(new ArgExpression(e));
}

FakeList<ArgExpression> *makeExprList2(Expression *e1, Expression *e2)
{
  return fl_prepend(makeExprList1(e2), new ArgExpression(e1));
}


// EOF
