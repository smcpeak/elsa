// t-vararg-promote.c
// Test implicit promotions of arguments to vararg functions as C.

int f(int, ...);

int f2(int, int);

void g(char c)
{
  f(1);
  f(1, 2);

  // TODO: This produces the wrong output because we do not run the code
  // tha would insert an ISC.
  f(1, c);
}

// EOF
