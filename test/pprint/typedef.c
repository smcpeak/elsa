// typedef.c
// Print a typedef by name instead of what it expands to.

typedef struct Foo_struct {
  int x;
  int y;
} Foo;

Foo g_foo;
Foo foo1, foo2, *foo3, foo4[3], * const *foo5;
Foo * const foo6, foo7;
Foo foo8, * const foo9;

Foo f(Foo *x, void *p)
{
  Foo local_foo;
  x = (Foo*)p;
  return (Foo)*x;
}

// EOF
