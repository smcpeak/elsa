// init-scalar.c
// C11 6.7.9/11: Initializer for a scalar.

static int test_scalars()
{
  // "The initializer for a scalar shall be a single expression, ..."
  int a = 1;

  // "... optionally enclosed in braces."
  int b = { 2 };

  // Clang will give an error with -pedantic-errors.
  //ERROR(double-braces): int c = { { 3 } };
  //NOTWORKING(gcc): GCC only warns.
  //NOTWORKING(elsa): Rule not enforced.

  // Same here.
  //ERROR(triple-braces): int d = { { { 4 } } };
  //NOTWORKING(gcc): GCC only warns.
  //NOTWORKING(elsa): Rule not enforced.

  // Conversions work like for assignment.
  unsigned e = (signed char)(-1);

  // Loss of precision going to 'int'.
  int f = 3.2;

  // Loss of precision going to 'float' means these will be equal.
  float g0 = 0x40000000;
  float g1 = 0x40000001;

  // Although even 'const' variables can be initialized, of course.
  int const h = 7;

  return 1 &&
    a == 1 &&
    b == 2 &&
    e == (unsigned)(-1) &&
    f == 3 &&
    g0 == 1.0737418e9f &&
    g1 == 1.0737418e9f &&
    h == 7 &&
    1;
}


int main()
{
  if (1 &&
    test_scalars() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
