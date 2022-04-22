// logical-op-ptr.c
// Logical operators applied to pointers.

int f(int a, _Bool b, void *p, void *q)
{
  int r;

  r = a && p;
  r = p && a;
  r = p && q;

  r = a || p;
  r = p || a;
  r = p || q;

  r = b && p;
  r = p && b;
  r = b || p;
  r = p || b;

  return r;
}

// EOF
