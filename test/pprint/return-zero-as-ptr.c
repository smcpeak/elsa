// return-zero-as-ptr.c
// Return 0 from a function that returns a pointer.

// The goal here is to ensure that we get an implicit conversion to
// pointer so my instrumentor can see that.

void *retzero()
{
  return 0;
}

void *f2(int n, char *s)
{
  // Test that we get the right type out of this conditional expression.
  return n? (void*)s : 0;
}

void *f3(int n, char *s)
{
  return n? 0 : (void*)s;
}

// EOF
