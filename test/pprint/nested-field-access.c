// nested-field-access.c
// "a.b.c" syntax.

struct A {
  int x;
};

struct B {
  struct A a;
};

struct B b;

int f()
{
  return b.a.x;
}

// EOF
