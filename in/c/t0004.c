// t0004.c
// non-inner struct

struct Outer {
  struct Inner {     // not really!
    int x;
  } is;
  enum InnerEnum { InnerEnumerator } ie;
  int y;
};

int foo(struct Inner *i)
{
  enum InnerEnum gotcha;
  return i->x + InnerEnumerator;
}
