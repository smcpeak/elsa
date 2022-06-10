// anon-union-in-struct.c
// An anonymous union inside a struct.

struct S {
  int op;
  union {
    struct {
      int nr;
    } entry;
    struct {
      int rval;
    } exit;
  };
};

int f(struct S *s)
{
  return s->op + s->entry.nr;
}

// EOF
