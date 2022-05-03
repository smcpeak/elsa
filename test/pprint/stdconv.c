// stdconv.c
// Examples with implicit standard conversions.

void f(void *p)
{
  p = 0;
  if (p == 0) {}
  if (0 == p) {}
}

// EOF
