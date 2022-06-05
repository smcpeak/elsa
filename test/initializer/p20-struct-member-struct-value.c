// p20-struct-member-struct-value.c
// Initialize a struct member with a struct value.

// This exercises a provision that is only vague and implicit in C11
// 6.7.9p20 ("account for the elements or members"), but is explicit in
// C++14 8.5p13:
//
// "If the assignment-expression can initialize a member, the member is
// initialized.  Otherwise, if the member is itself a subaggregate,
// brace elision is assumed and the assignment-expression is considered
// for the initialization of the first member of the subaggregate."
//
// That is, we have to look at the *type* of an element in an
// initializer list to know how much of the object it initializes.

struct Inner {
  int x;
  int y;
};

struct Outer {
  struct Inner a;
  struct Inner b;
};


struct Outer fo(struct Inner inner)
{
  // This can accept two elements but not three if their type is
  // 'Inner'.
  struct Outer o = {
    inner,
    inner,
    //ERROR(too-many-inits1): inner,
  };
  return o;
}

int test_fo()
{
  struct Inner inner = { 1, 2 };
  struct Outer o = fo(inner);

  return
    o.a.x == 1 &&
    o.a.y == 2 &&
    o.b.x == 1 &&
    o.b.y == 2 &&
    1;
}


struct Outer fo2(int integer)
{
  // This can accept 4 but not 5 with type 'int'.
  struct Outer o = {
    integer,
    integer+1,
    integer+2,
    integer+3,
    //ERROR(too-many-inits2): integer+4,
  };
  return o;
}

int test_fo2()
{
  struct Outer o = fo2(3);

  return
    o.a.x == 3 &&
    o.a.y == 4 &&
    o.b.x == 5 &&
    o.b.y == 6 &&
    1;
}


struct Outer fo3a(struct Inner inner, int integer)
{
  struct Outer o = {
    inner,
    integer+1,
    integer+2,
    //ERROR(too-many-inits3a): integer+3,
  };
  return o;
}

int test_fo3a()
{
  struct Inner inner = { 1,2 };
  struct Outer o = fo3a(inner, 3);

  return
    o.a.x == 1 &&
    o.a.y == 2 &&
    o.b.x == 4 &&
    o.b.y == 5 &&
    1;
}


struct Outer fo3b(struct Inner inner, int integer)
{
  struct Outer o = {
    integer+1,
    integer+2,
    inner,
    //ERROR(too-many-inits3b): integer+3,
  };
  return o;
}

int test_fo3b()
{
  struct Inner inner = { 1,2 };
  struct Outer o = fo3b(inner, 3);

  return
    o.a.x == 4 &&
    o.a.y == 5 &&
    o.b.x == 1 &&
    o.b.y == 2 &&
    1;
}


int main()
{
  return
    test_fo() &&
    test_fo2() &&
    test_fo3a() &&
    test_fo3b() &&
    1? 0 : 1;
}


// EOF
