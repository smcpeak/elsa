// p35-ex-designator-to-clarify.c
// Designators can sometimes improve clarity.

struct { int a[3], b; } w[] =
      { [0].a = {1}, [1].a[0] = 2 };

// Same check as in p28-ex-inconsistent-brackets.c.
int main()
{
  return
    w[0].a[0] == 1 &&
    w[0].a[1] == 0 &&
    w[0].a[2] == 0 &&
    w[0].b == 0 &&

    w[1].a[0] == 2 &&
    w[1].a[1] == 0 &&
    w[1].a[2] == 0 &&
    w[1].b == 0 &&

    1? 0 : 1;
}

// EOF