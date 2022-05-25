// ptr-as-condition.c
// Use a pointer as the condition of control flow constructs.

// The point is there should be an implicit conversion to 'bool' or
// '_Bool' when that is done.

void f(int *p)
{
  if (p) {}
  if (!p) {}

  p && p;
  p || p;
  p? p : p;

  for (; p; ) {
    break;
  }

  while (p) {
    break;
  }

  do {
    break;
  } while(p);
}

// EOF
