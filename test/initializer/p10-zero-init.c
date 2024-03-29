// p10-zero-init.c
// C11 6.7.9/10: Implicit zero initialization for static duration.


// Pointer initialized to NULL.
int *ptr;

static int test_ptr()
{
  return ptr == 0;
}


// Arithmetic type to zero.
int int_val;
long long_val;
float float_val;

static int test_arithmetic()
{
  return
    int_val == 0 &&
    long_val == 0 &&
    float_val == 0 &&
    1;
}


// Aggregate (struct or array): recursively zero-init.
typedef struct S {
  int x;
  int y;
} S;

S struct_val;

int array_val[2];

static int test_aggregate()
{
  return
    struct_val.x == 0 &&
    struct_val.y == 0 &&
    array_val[0] == 0 &&
    array_val[1] == 0 &&
    1;
}


// Union: first element zero.
typedef union U {
  int x;
  char c;
} U;

U union_val;

static int test_union()
{
  return
    union_val.x == 0 &&
    1;
}


int main()
{
  if (1 &&
    test_ptr() &&
    test_arithmetic() &&
    test_aggregate() &&
    test_union() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
