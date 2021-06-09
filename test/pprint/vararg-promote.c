// t-vararg-promote.c
// Test implicit promotions of arguments to vararg functions as C.

int f(int, ...);

int f2(int, int);

void g(char c)
{
  f(1);
  f(1, 2);
  f(1, c);
}

// EOF
