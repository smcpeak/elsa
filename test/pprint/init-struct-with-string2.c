// init-struct-with-string2.c
// Another test, this time with unsigned mismatch.

struct S {
  unsigned char const *str;
};

void test3()
{
  struct S s = { "hello" };
}
