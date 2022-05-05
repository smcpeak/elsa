// builtin-offsetof.c
// __builtin_offsetof

struct S {
  int x;
  int y;
};

int f()
{
  return __builtin_offsetof(struct S, x);
}

int g()
{
  return __builtin_offsetof(struct S, y);
}

// EOF
