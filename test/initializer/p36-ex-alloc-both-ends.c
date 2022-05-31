// p36-ex-alloc-both-ends.c
// Allocating from both ends using array index designator.

enum { MAX1 = 100 };

int a1[MAX1] = {
  1, 3, 5, 7, 9, [MAX1-5] = 8, 6, 4, 2, 0
};

enum { MAX2 = 7 };

int a2[MAX2] = {
  1, 3, 5, 7, 9, [MAX2-5] = 8, 6, 4, 2, 0
};

int main()
{
  _Static_assert(sizeof(a1) / sizeof(a1[0]) == MAX1, "");
  _Static_assert(sizeof(a2) / sizeof(a2[0]) == MAX2, "");

  return
    a1[0] == 1 &&
    a1[1] == 3 &&
    a1[2] == 5 &&
    a1[3] == 7 &&
    a1[4] == 9 &&
    a1[5] == 0 &&
    // ...
    a1[94] == 0 &&
    a1[95] == 8 &&
    a1[96] == 6 &&
    a1[97] == 4 &&
    a1[98] == 2 &&
    a1[99] == 0 &&

    a2[0] == 1 &&
    a2[1] == 3 &&
    a2[2] == 8 &&
    a2[3] == 6 &&
    a2[4] == 4 &&
    a2[5] == 2 &&
    a2[6] == 0 &&

    1? 0 : 1;
}

// EOF
