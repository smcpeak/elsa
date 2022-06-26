// typedef-multi-declarator.c
// Typedef with multiple declarators.

// One declarator.
int f1()
{
  typedef int a;
  a x = 3;
  return x;
}


// Two declarators.
int f2()
{
  typedef int a, b;
  a x = 3;
  b y = 4;
  return x + y;
}


// EOF
