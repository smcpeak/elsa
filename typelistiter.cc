// typelistiter.cc
// code for typelistiter.h

#include "typelistiter.h"      // this module
#include "cc_ast.h"            // C++ ast components


// -------- TypeListIter_FakeList --------

bool TypeListIter_FakeList::isDone() const
{
  return curFuncArgs == 0;
}

void TypeListIter_FakeList::adv()
{
  xassert(!isDone());
  curFuncArgs = fl_butFirst(curFuncArgs);
}

Type *TypeListIter_FakeList::data() const
{
  xassert(!isDone());
  return fl_first(curFuncArgs)->getType();
}


// -------- TypeListIter_GrowArray --------

bool TypeListIter_GrowArray::isDone() const
{
  return i == args.allocatedSize();
}

void TypeListIter_GrowArray::adv()
{
  xassert(!isDone());
  ++i;
}

Type *TypeListIter_GrowArray::data() const
{
  xassert(!isDone());
  return args[i].type;
}


// EOF
