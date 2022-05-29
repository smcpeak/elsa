// static-assert.c
// Test _Static_assert in C.

void f()
{
  _Static_assert(1==1);
  //ERROR(1): _Static_assert(0==1);
  //ERROR(2): _Static_assert(0==1, "zero equals one");
  //ERROR(3): _Static_assert(1==1, 1234 /*invalid*/);
  //ERROR(4): _Static_assert(1==1, "too many args here", 4);
  //ERROR(5): _Static_assert();
}

// EOF
