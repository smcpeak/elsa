// array-plus-int.c
// Test how we parse 'arr+1' where 'arr' is an array, and related constructs.

void f()
{
  char arr[5];

  char *cp;
  void *vp;

  cp = arr;        // SC_ARRAY_TO_PTR
  vp = arr;        // SC_ARRAY_TO_PTR|SC_PTR_CONV

  // These should also have SC_ARRAY_TO_PTR but currently don't.
  cp = arr+1;
  vp = arr+1;

  cp = arr-1;
  vp = arr-1;
}

// EOF
