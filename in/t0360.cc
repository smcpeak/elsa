// t0360.cc
// C++98 apparently wants arg-dep lookup here, but Elsa can't do it

class stringBuilder
{
  typedef void (*Manipulator) (stringBuilder & sb);
  void foo(Manipulator manip);
};

void stringBuilder::foo(Manipulator manip)
{
  manip (*this);
}
