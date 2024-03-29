// p22-array-unspec-size.c
// C11 6.7.9/22: Array size determined by largest index.


int arr2[] = { 1, 2 };

int arr1000[] = { [999] = 999 };

int arr10[] = { 1, [8] = 8, 9 };

static int test_arr2()
{
  _Static_assert(sizeof(arr2   ) / sizeof(arr2   [0]) ==    2, "");
  _Static_assert(sizeof(arr1000) / sizeof(arr1000[0]) == 1000, "");
  _Static_assert(sizeof(arr10  ) / sizeof(arr10  [0]) ==   10, "");

  for (int i=0; i < 1000; i++) {
    if (arr1000[i] != (i==999? 999 : 0)) {
      return 0;
    }
  }

  for (int i=1; i < 8; i++) {
    if (arr10[i] != 0) {
      return 0;
    }
  }

  return
    arr10[0] == 1 &&
    arr10[8] == 8 &&
    arr10[9] == 9 &&
    1;
}


typedef struct S {
  int x;
  int y;
} S;

// Use a nested designator to set the array size.
S s3[] = { [2].x = 3 };

// Nested designator, then walk forward to expand it.
S s4[] = { [2].x = 4, 5, 6, 7 };

static int test_s3_s4()
{
  _Static_assert(sizeof(s3) / sizeof(s3[0]) == 3, "");
  _Static_assert(sizeof(s4) / sizeof(s4[0]) == 4, "");

  for (int i=0; i < 3; i++) {
    if (!( s3[i].x == (i==2? 3 : 0) &&
           s3[i].y == 0 )) {
      return 0;
    }
  }

  for (int i=0; i < 4; i++) {
    if (!( s4[i].x == (i==2? 4 : i==3? 6 : 0) &&
           s4[i].y == (i==2? 5 : i==3? 7 : 0) )) {
      return 0;
    }
  }

  return 1;
}


int main()
{
  return
    test_arr2() &&
    test_s3_s4() &&
    1? 0 : 1;
}


// EOF
