// p03-complete-object.c
// C11 6.7.9/3: "The type of the entity to be initialized shall be an
// array of unknown size or a complete object type that is not a
// variable length array type."


typedef struct Complete {
  int x;
  int y;
} Complete;

Complete c1 = { 1,2 };

static int test_c1()
{
  return
    c1.x == 1 &&
    c1.y == 2 &&
    1;
}


typedef struct Incomplete Incomplete;

//ERROR(incomplete-struct): Incomplete ic1 = { 1,2 };


int arr1[3] = { 1,2,3 };

int arr2[] = { 1,2,3 };

static int test_arr1_arr2()
{
  return
    arr1[0] == 1 &&
    arr1[1] == 2 &&
    arr1[2] == 3 &&
    arr2[0] == 1 &&
    arr2[1] == 2 &&
    arr2[2] == 3 &&
    sizeof(arr2) / sizeof(arr2[0]) == 3 &&
    1;
}


int arr23a[2][3] = {
  { 0, 1, 2 },
  { 3, 4, 5 }
};

int arr23b[][3] = {
  { 0, 1, 2 },
  { 3, 4, 5 }
};

static int test_arr23ab()
{
  for (int i=0; i<2; i++) {
    for (int j=0; j<3; j++) {
      if (arr23a[i][j] != i*3 + j) {
        return 0;
      }
      if (arr23b[i][j] != i*3 + j) {
        return 0;
      }
    }
  }

  return sizeof(arr23b) / sizeof(arr23b[0]) == 2;
}


// Only the first dimension can be unspecified.
//ERROR(unspec-multidim): int arr23c[][] = { { 0, 1, 2 }, { 3, 4, 5 } };
//ERROR(unspec-inner-dim): int arr23d[2][] = { { 0, 1, 2 }, { 3, 4, 5 } };


int init_variable_length_array(int sz)
{
  //ERROR(init-vla): int arr3[sz] = { 1,2,3 };
  //NOTWORKING(elsa): Rule not enforced.
  return sz;
}


typedef struct HasFlexArray {
  int x;
  int a[];
} HasFlexArray;

// This is not legal in C11: 6.7.6.2/4: "If the size is not present, the
// array type is an incomplete type."
//
// However, GCC allows defining and initializing a structure with a
// flexible array member as an extension:
// https://gcc.gnu.org/onlinedocs/gcc/Zero-Length.html
//
// For the moment, my approach is to use -pendantic-errors with GCC.
//
//ERROR(init-flex-array): HasFlexArray hfa1 = { 1, { 2,3 } };
//NOTWORKING(elsa): Rule not enforced.

typedef struct ContainsHFA {
  int z;

  // Disallowed by C11 6.7.2.1p3.
  //ERROR(init-nested-flex-array): HasFlexArray hfa;
} ContainsHFA;


int main()
{
  if (1 &&
    test_c1() &&
    test_arr1_arr2() &&
    test_arr23ab() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
