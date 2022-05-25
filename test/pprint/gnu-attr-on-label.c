// gnu-attr-on-label.c
// Attribute applied to a label.

int f(int x)
{
  if (x == 1) {
    goto label1;
  }
  if (x == 2) {
    goto label2;
  }
  x++;

label1: __attribute__((unused))
  x++;

label2: __attribute__((unused)) __attribute__((hot))
  return x;
}

// EOF
