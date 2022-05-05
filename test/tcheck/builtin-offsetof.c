// builtin-offsetof.c
// __builtin_offsetof

struct S {
  int x;
  int y;
};

int f()
{
  // Non-existent field.
  //ERROR(1): __builtin_offsetof(struct S, z);

  // Not a struct.
  //ERROR(2): __builtin_offsetof(int, x);

  return __builtin_offsetof(struct S, x);
}

int g()
{
  return __builtin_offsetof(struct S, y);
}

// EOF
