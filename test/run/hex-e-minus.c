// hex-e-minus.c
// Hexadecimal literal that ends in 'e', minus something else.

int f(int x)
{
  // The risk here is failing to put a space before '-' when printing,
  // causing the lexer to treat it as part of the number.
  if (x >= 0x43e - 0x3be) {    // subtraction yields 128 decimal
    return 1;
  }
  else {
    return 0;
  }
}

int g(int x)
{
  // The same issue can happen with '+'.
  if (x >= 0Xe + 0x2) {        // addition yields 16 decimal
    return 1;
  }
  else {
    return 0;
  }
}

int main()
{
  return
    f(127) == 0 &&
    f(128) == 1 &&
    g(15) == 0 &&
    g(16) == 1 &&
    1? 0 : 1;
}

// EOF
