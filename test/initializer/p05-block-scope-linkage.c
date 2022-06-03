// p05-block-scope-linkage.c
// C11 6.7.9/5: "If the declaration of an identifier has block scope,
// and the identifier has external or internal linkage, the declaration
// shall have no initializer for the identifier.

// GCC issues a warning for initialized 'extern', and I don't want to
// pollute my tests with that warning, nor is it my current focus, so
// I'll just comment this.
/*extern*/ int x = 3;

static int test_x()
{
  extern int x;
  return x == 3;
}


int zero;

static int test_zero()
{
  extern int zero
  //ERROR(extern-has-init): = 0
    ;
  return zero == 0;
}


static int s_three = 3;

static int test_s_three()
{
  extern int s_three
  //ERROR(static-extern-has-init): = 3
    ;
  return s_three == 3;
}


int main()
{
  if (1 &&
    test_x() &&
    test_zero() &&
    test_s_three() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
