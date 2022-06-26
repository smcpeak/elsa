// t0018.c
// ambiguity involving attributed labels vs. implicit-int

void foo(int x, int y)
{
  // Clang simply discards this attribute!  So I cannot print it out.
  label: __attribute__ ((__unused__))

  foo (x, y - 0 );
}
