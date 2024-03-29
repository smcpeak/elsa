// typelistiter.h                       see license.txt for copyright and terms of use
// iterate over a sequence of objects, looking at their types
// author: dsw

#ifndef TYPELISTITER_H
#define TYPELISTITER_H

#include "typelistiter-fwd.h"          // forwards for this module

// elsa
#include "overload.h"                  // Type, GrowArray, ArgumentInfo, etc.

// ast
#include "fakelist.h"                  // FakeList


// unifies the process of iterating over a list of types
class TypeListIter {
  public:
  // can't share the common dtor in the superclass because it calls a
  // virtual abstract method
  virtual ~TypeListIter() {}

  // iterator actions
  virtual bool isDone() const = 0;
  virtual void adv() = 0;
  virtual Type *data() const = 0;
};

class TypeListIter_FakeList : public TypeListIter {
  FakeList<ArgExpression> *curFuncArgs;

  public:
  TypeListIter_FakeList(FakeList<ArgExpression> *funcArgs0)
    : curFuncArgs(funcArgs0)
  {}
  virtual ~TypeListIter_FakeList() {
    // this is not always the case if argument list processing aborts
    // part way through due to an error
//      xassert(isDone());
  }

  virtual bool isDone() const override;
  virtual void adv() override;
  virtual Type *data() const override;
};

class TypeListIter_GrowArray : public TypeListIter {
  GrowArray<ArgumentInfo> &args;
  int i;

  public:
  TypeListIter_GrowArray(GrowArray<ArgumentInfo> &args0)
    : args(args0), i(0)
  {}
  virtual ~TypeListIter_GrowArray() {
    // this is not always the case if argument list processing aborts
    // part way through due to an error
//      xassert(isDone());
  }

  virtual bool isDone() const override;
  virtual void adv() override;
  virtual Type *data() const override;
};

#endif // TYPELISTITER_H
