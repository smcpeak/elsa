// p06-array-designator-expression.c
// C11 6.7.9/6: Array designator must be constant, etc.

int one()
{
  return 1;
}


int arr2a[2] = { [0] = 0, [1] = 1 };

//ERROR(non-const-desig-expr): int arr2b[2] = { [0] = 0, [one()] = 1 };

static int test_arr2a()
{
  return
    arr2a[0] == 0 &&
    arr2a[1] == 1 &&
    1;
}


typedef struct S {
  int x;
  int y;
} S;

S s1 = { 1, 2 };

//ERROR(index-for-non-array): S s2 = { [0] = 1, 2 };
//NOTWORKING(elsa): Rule not enforced.

static int test_s1()
{
  return
    s1.x == 1 &&
    s1.y == 2 &&
    1;
}


// Index can be large.
int arr1000a[] = { 0, 1, 2, [999] = 999 };

// But not negative.
//ERROR(negative-for-unspec-size): int arr1000b[] = { 0, 1, 2, [-1] = 999 };
//NOTWORKING(elsa): Rule not enforced.

static int test_arr1000()
{
  for (int i=0; i < 1000; i++) {
    int exp = 0;
    if (i <= 2 || i == 999) {
      exp = i;
    }
    if (arr1000a[i] != exp) {
      return 0;
    }
  }

  return sizeof(arr1000a) / sizeof(arr1000a[0]) == 1000;
}


int main()
{
  if (1 &&
    test_arr2a() &&
    test_s1() &&
    test_arr1000() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
