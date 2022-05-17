// weak-alias.c
// GCC "weak alias" attribute.
// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.htm

int __f()
{
  return 6;
}

// This is basically a definition of f(), which behaves the same as
// __f().
int f() __attribute__ ((weak, alias ("__f")));

// EOF
