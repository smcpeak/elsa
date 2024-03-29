// init-struct-with-string.c
// Initializer a struct with a char* member with a string literal.

struct S {
  char const *str;
};

int test1()
{
  struct S s = { "hello" };
  int ch = s.str[1];
  return (ch == 'e'? 0 : 1);
}

int test2()
{
  struct S s = { &("hello"[0]) };
  int ch = s.str[1];
  return (ch == 'e'? 0 : 1);
}

int test3()
{
  char src[6] = "hello";
  struct S s = { src };
  int ch = s.str[1];
  return (ch == 'e'? 0 : 1);
}

int test4()
{
  char src[6] = "hello";
  struct S s = { &(src[0]) };
  int ch = s.str[1];
  return (ch == 'e'? 0 : 1);
}

// EOF
