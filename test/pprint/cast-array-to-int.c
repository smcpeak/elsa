// cast-array-to-int.c
// Cast array variable to integer type.

typedef unsigned long uintptr_t;

extern int arr[];

uintptr_t f1()
{
  uintptr_t p;

  // TODO: There should be an array-to-pointer conversion here, but I'm
  // not sure how I want to represent that.
  p = (uintptr_t)arr;

  return p;
}

// EOF
