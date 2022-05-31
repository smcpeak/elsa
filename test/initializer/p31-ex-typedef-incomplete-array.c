// p31-ex-typedef-incomplete-array.c
// typedef at block scope with unspecified array size.

int main()
{
  typedef int A[];           // OK - declared with block scope

  A a1 = { 1, 2 }, b1 = { 3, 4, 5 };

  int a2[] = { 1, 2 }, b2[] = { 3, 4, 5 };

  _Static_assert(sizeof(a1) / sizeof(a1[0]) == 2, "");
  _Static_assert(sizeof(b1) / sizeof(b1[0]) == 3, "");
  _Static_assert(sizeof(a2) / sizeof(a2[0]) == 2, "");
  _Static_assert(sizeof(b2) / sizeof(b2[0]) == 3, "");

  return
    a1[0] == 1 &&
    a1[1] == 2 &&

    b1[0] == 3 &&
    b1[1] == 4 &&
    b1[2] == 5 &&

    a2[0] == 1 &&
    a2[1] == 2 &&

    b2[0] == 3 &&
    b2[1] == 4 &&
    b2[2] == 5 &&

    1? 0 : 1;
}

// EOF
