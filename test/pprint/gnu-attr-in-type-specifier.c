// gnu-attr-in-type-specifier.c
// GNU attributes in TypeSpecifiers.

int f(__attribute__((mode(byte))) int b1,
      int __attribute__((mode(byte))) b2);

int f(__attribute__((mode(byte))) int b1,
      int __attribute__((mode(byte))) b2)
{
  return b1+b2;
}

// EOF
