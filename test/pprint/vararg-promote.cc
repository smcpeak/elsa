// t-vararg-promote.cc
// Test implicit promotions of arguments to vararg functions.

int f(int, ...);

int f2(int, int);

void g()
{
  f(1);
  f(1, 2);
  f(1, false);
}

// EOF
