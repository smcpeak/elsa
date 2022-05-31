// p25-ex-array-unspec-size.c
// Array with unspecified size has size set by initializer.

int x[] = { 1, 3, 5 };

int main()
{
  _Static_assert(sizeof(x) / sizeof(x[0]) == 3, "");

  return
    x[0] == 1 &&
    x[1] == 3 &&
    x[2] == 5 &&
    1? 0 : 1;
}

// EOF
