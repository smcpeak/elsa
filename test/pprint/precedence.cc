// pprint/precedence.cc
// Exercising precedence-aware printing.

class C {
public:
  static int w;
  static int g();
  typedef int INT;
  static int *arr;
  static int *p;
  int *q;
  int i;
  int C::*ptm;
};

void g(int);

void scope_qualifier()
{
  // OPREC_SCOPE_QUALIFIER
  C::w;
  (C::w);
}

void postfix(int x, C c, C *cp, int **pp)
{
  // OPREC_POSTFIX on OPREC_SCOPE_QUALIFIER
  C::w++;
  C::w--;
  +C::INT(x);       // functional cast ('+' needed for disambiguation)
  C::g();
  C::arr[0];
  c.C::p;
  cp->C::p;

  // OPREC_POSTFIX on OPREC_POSTFIX
  pp[0][1];
}

void prefix(int x, C c, C *cp, int *pp)
{
  // OPREC_PREFIX on OPREC_POSTFIX
  ++c.w;
  ++(c.w);
  --c.w;
  +c.w;
  -c.w;
  !c.w;
  ~c.w;
  (int)c.w;
  *c.q;
  &c.w;
  sizeof(c.w);
  new int;
  new int[2];
  delete c.q;
  delete[] c.q;

  // OPREC_POSTFIX on OPREC_PREFIX
  (++x)++;
  (+pp)[0];
  (&g)(0);
  (+cp)->i;
}

void ptr_to_memb(C *cp, C &cr)
{
  // OPREC_PTR_TO_MEMB on OPREC_POSTFIX
  int C::*ptm = &C::i;
  cp->*cp->ptm;
  cp->*cr.ptm;
  cr.*cp->ptm;
  cr.*cr.ptm;
}

void multiply(int x, int y, int z)
{
  // OPREC_MULTIPLY on OPREC_MULTIPLY.
  x*y*z;
  x*y/z%x;
  x*(y*z);
  x*(y/z)%x;

  // OPREC_ADD on OPREC_MULTIPLY.
  x+y*z;
  x*y+z;

  // OPREC_MULTIPLY on OPREC_ADD.
  (x+y)*z;
  x/(y-z);
}

void shift(int x, int y, int z)
{
  // OPREC_SHIFT on OPREC_SHIFT.
  x << y << z;
  x >> y >> z;
  x << y >> z;
  x << (y >> z);

  // OPREC_SHIFT on OPREC_ADD.
  x << y+z;
  x+y >> z;

  // OPREC_ADD on OPREC_SHIFT.
  x + (y<<z);
  (x>>y) + z;
}

void relational(int x, int y, int z)
{
  // OPREC_RELATIONAL on OPREC_RELATIONAL.
  x < y < z;
  x < (y < z);
  x < y > z;
  x > y < z;
  x > (y > z);

  // OPREC_RELATIONAL on OPREC_SHIFT.
  x < y << z;
  x >> y > z;

  // OPREC_SHIFT on OPREC_RELATIONAL.
  (x < y) << z;
  x >> (y > z);
}

void equality(int x, int y, int z)
{
  // OPREC_EQUALITY on OPREC_EQUALITY.
  x == y == z;
  x == (y == z);
  x != y != z;
  x == (y != z);

  // OPREC_EQUALITY on OPREC_RELATIONAL.
  x == y > z;
  x > y == z;

  // OPREC_EQUALITY on OPREC_ADD.
  x + y == z;
  x == y + z;
}

void bitwise(int x, int y, int z)
{
  // OPREC_BIT_AND on OPREC_EQUALITY.
  x & y == z;
  x == y & z;

  // OPREC_BIT_AND on OPREC_ADD.
  x & y + z;
  x + y & z;

  // OPREC_ADD on OPREC_BIT_AND.
  (x & y) + z;
  x + (y & z);

  // OPREC_BIT_OR on OPREC_ADD.
  x | y + z;
  x + y | z;

  // OPREC_ADD on OPREC_BIT_OR.
  (x | y) + z;
  x + (y | z);

  // OPREC_BIT_OR on OPREC_BIT_AND.
  x & y | z;
  x | y & z;

  // OPREC_BIT_AND on OPREC_BIT_OR.
  x & (y | z);
  (x | y) & z;

  // OPREC_BIT_XOR on OPREC_ADD.
  x ^ y + z;
  x + y ^ z;

  // OPREC_ADD on OPREC_BIT_XOR.
  (x ^ y) + z;
  x + (y ^ z);
}

void logical(int x, int y, int z)
{
  // OPREC_LOGICAL_AND on OPREC_LOGICAL_AND.
  x && y && z;
  x && (y && z);

  // OPREC_LOGICAL_AND on OPREC_BIT_OR.
  x && y & z;
  x & y && z;

  // OPREC_LOGICAL_OR on OPREC_LOGICAL_OR.
  x || y || z;
  x || (y || z);

  // OPREC_LOGICAL_OR on OPREC_LOGICAL_AND.
  x && y || z;
  x || y && z;

  // OPREC_LOGICAL_AND on OPREC_LOGICAL_OR.
  x && (y || z);
  (x || y) && z;
}

// Arrange for operator|| to yield an lvalue.
int& operator|| (C const &c1, C const &c2);

void assign(int x, int y, int z, C c1, C c2)
{
  // OPREC_ASSIGN on OPREC_ASSIGN.
  x = y = z;
  x = (y = z);
  (x = y) = z;
  x += y += z;
  x += (y += z);
  (x = y) += z;

  // OPREC_ASSIGN on OPREC_LOGICAL_OR.
  x = y || z;
  c1 || c2 = z;
  x += y || z;
  c1 || c2 += z;

  // OPREC_LOGICAL_OR on OPREC_ASSIGN.
  (x = y) || z;
  x || (y = z);
  (x += y) || z;
  x || (y += z);
}

void conditional(int u, int w, int x, int y, int z)
{
  // Conditional on conditional.
  w? x : y? z : u;
  (w? x : y)? z : u;
  w? x? y : z : u;

  // Conditional on assignment.
  (w = x)? y : z;
  w? x = y : z;
  w? x : y = z;

  // Assignment on conditional.
  w = x? y : z;
  (w? x : y) = z;
}

void comma(int x, int y, int z)
{
  // Comma on comma.
  x, y, z;
  x, (y, z);

  // Comma on assignment.
  x = y, z;
  x, y = z;

  // Assignment on comma.
  x = (y, z);
  (x, y) = z;
}

void test_throw(int x, int y)
{
  // Throw on throw.  Not legal.
  //throw throw;
  //throw throw x;

  // Throw on OPREC_ADD.
  throw x+y;

  // Throw on OPREC_COMMA.
  throw (x, y);

  // OPREC_COMMA on throw.
  x, throw y;
  throw x, y;
}

// EOF
