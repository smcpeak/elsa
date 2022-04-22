// func-symbol.c
// Test '__func__'.

int printf(char const *format, ...);

static void test1()
{
  char const *n = __func__;
  printf("name: %s\n", n);
}

static void test2()
{
  printf("name: %s\n", __func__);
}

int main()
{
  test1();
  test2();
  return 0;
}

// EOF
