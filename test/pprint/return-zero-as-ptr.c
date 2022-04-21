// return-zero-as-ptr.c
// Return 0 from a function that returns a pointer.

// The goal here is to ensure that we get an implicit conversion to
// pointer so my instrumentor can see that.

void *retzero()
{
  return 0;
}

// EOF
