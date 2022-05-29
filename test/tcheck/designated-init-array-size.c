// designated-init-array-size.c
// Test using a sparse designated initializer to set array size.

void f()
{
  static int const arr[] = {
    [0] = 0,
    [1] = 1,
    2,   // at [2]
    3,   // at [3]
    [4] = 4,
    [10] = 10,
    //ERROR(1): .x = 5,
    11,  // at [11]
    120, // at [12], overridden
    [6] = 6,
    7,   // at [7]
    [12] = 12,
    [15] = 15,
  };

  _Static_assert(sizeof(arr) / sizeof(arr[0]) == 16);
}

typedef struct S {
  int x;
  int y;
} S;

void f2()
{
  // Incomplete brace enclosure.
  S arr2[2] = { 1,2, 3,4 };
  _Static_assert(sizeof(arr2) / sizeof(arr2[0]) == 2);

  S arr3[] = { 1,2, 3,4, 5,6 };
  _Static_assert(sizeof(arr3) / sizeof(arr3[0]) == 3);
}

// EOF
