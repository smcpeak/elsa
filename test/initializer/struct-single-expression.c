// struct-single-expression.c
// C11 6.7.9/13: Initialize struct/union with single expression.

typedef struct S {
  int x;
  int y;
} S;

S makeS()
{
  S s = { 1,2 };
  return s;
}

// Not allowed, this is not automatic storage.
//ERROR(not-auto-struct): S s1 = makeS();
//NOTWORKING(elsa): Rule not enforced.

static int test_auto_struct()
{
  // Allowed, this is automatic.
  S s = makeS();

  return
    s.x == 1 &&
    s.y == 2 &&
    1;
}


typedef union U {
  int x;
  int y;
} U;

U makeU(int x)
{
  U u = { x };
  return u;
}

// Not allowed.
//ERROR(not-auto-union): U u1 = makeU(8);
//NOTWORKING(elsa): Rule not enforced.

static int test_auto_union()
{
  // Allowed.
  U u = makeU(8);

  return
    u.x == 8 &&
    u.y == 8 &&
    1;
}


int main()
{
  if (
    test_auto_struct() &&
    test_auto_union() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
