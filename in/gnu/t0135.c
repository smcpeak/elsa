// gnu/t0135.c
// transparent union, with the attribute directly on the union

// This does not currently work because I only retain attributes that
// are associated with declarators, whereas in this case it is
// associated with a type specifier.

// 2022-05-22: Now they are retained and used, so the test passes!

typedef struct S {
  int blah;
} S;

union U {
  int *i;
  S *s;
} __attribute__((transparent_union));

int f(union U u)
{
  return *u.i;
}

int pass_int(int *p)
{
  return f(p /*implicitly initializes U.i*/);
}
