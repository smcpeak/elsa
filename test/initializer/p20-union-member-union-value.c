// p20-union-member-union-value.c
// A union member can be initialized with a union value.

union U {
  int x;
  int y;
};

struct S {
  union U u;
};

struct S fa(union U u)
{
  struct S s = { u };
  return s;
}

struct S fb(union U u)
{
  struct S s = { .u = u };
  return s;
}

struct S fc(int x)
{
  struct S s = {
    x
    //ERROR(too-many): , 5
  };
  return s;
}

struct S fd(int x)
{
  struct S s = { .u = x };
  return s;
}

static int test_f()
{
  union U u = { 3 };

  struct S sa = fa(u);
  struct S sb = fb(u);
  struct S sc = fc(3);
  struct S sd = fd(3);

  return
    sa.u.x == 3 &&
    sb.u.x == 3 &&
    sc.u.x == 3 &&
    sd.u.x == 3 &&
    1;
}


struct S2 {
  struct S s;
};

struct S2 f2a(struct S s)
{
  struct S2 s2 = { { s.u } };
  return s2;
}

struct S2 f2b(struct S s)
{
  // GCC warns about but accepts this.  Clang does not warn.
  struct S2 s2 = { s.u };
  return s2;
}

struct S2 f2c(struct S s)
{
  struct S2 s2 = { .s.u = s.u };
  return s2;
}

static int test_f2()
{
  struct S s = { { 4 } };

  struct S2 s2a = f2a(s);
  struct S2 s2b = f2b(s);
  struct S2 s2c = f2c(s);

  return
    s2a.s.u.x == 4 &&
    s2b.s.u.x == 4 &&
    s2c.s.u.x == 4 &&
    1;
}


int main()
{
  return
    test_f() &&
    test_f2() &&
    1? 0 : 1;
}


// EOF
