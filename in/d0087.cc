// this had stopped working and we hadn't a test to reveal it

struct X {
  // sm: this is invalid C++!  see 9.2 para 13
  //ERROR(1): int X;

  int Y;     // ok
};


// 2022-06-02: There was a test here for zero-sized arrays, but that
// is now tested by test/errmsg/zero-size-array.{c,cc}.  It is only
// diagnosed with -pedantic-errors, which is not active during this
// test.
