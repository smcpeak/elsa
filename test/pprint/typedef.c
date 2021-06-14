// typedef.c
// Print a typedef by name instead of what it expands to.

typedef struct Foo_struct {
  int x;
  int y;
} Foo;

Foo g_foo;

Foo f(Foo *x, void *p)
{
  Foo local_foo;
  x = (Foo*)p;
  return (Foo)x;
}

// EOF
