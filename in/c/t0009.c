// t0009.c
// oink/test2-cc/voidconditional.c

// sm: I do not believe this is valid C99, but gcc accepts it ...

void driv(void)
{
  // Both GCC-9.3 and Clang-14.0 warn with -pedantic.
  1 ? 1 : driv();
}


// With -std=c11, GCC accepts this, while Clang does not.
void main(void)
{
  driv();
}

