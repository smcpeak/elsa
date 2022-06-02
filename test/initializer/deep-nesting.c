// deep-nesting.c
// Deeply nested designation.

struct A {
  struct B {
    struct C {
      struct D {
        int x;
      } d;
    } c;
  } b;
} a = { .b.c.d.x = 87 };

int main()
{
  return
    a.b.c.d.x == 87 &&
    1? 0 : 1;
}

// EOF
