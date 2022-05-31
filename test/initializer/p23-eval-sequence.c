// p23-eval-sequence.c
// C11 6.7.9/23: Testing evaluation order of initializers.

// C11 6.7.9/23: "The evaluations of the initialization list expressions
// are indeterminately sequenced with respect to one another and thus
// the order in which any side effects occur is unspecified[152].
// [152]: In particular, the evaluation order need not be the same as
// the order of subobject initialization."


extern int printf(char const *format, ...);

static int get(int v)
{
  printf("  called get(%d)\n", v);
  return v;
}


static int test_array_plain()
{
  printf("test_array_plain:\n");

  int arr[2] = {
    get(1),
    get(2)
  };

  return
    arr[0] == 1 &&
    arr[1] == 2 &&
    1;
}


static int test_array_desig()
{
  printf("test_array_desig:\n");

  int arr[2] = {
    [0] = get(1),
    [1] = get(2)
  };

  return
    arr[0] == 1 &&
    arr[1] == 2 &&
    1;
}


// Both GCC and Clang call 'get(1)' before 'get(2)'.
static int test_array_swap()
{
  printf("test_array_swap:\n");

  int arr[2] = {
    [1] = get(2),
    [0] = get(1)
  };

  return
    arr[0] == 1 &&
    arr[1] == 2 &&
    1;
}


typedef struct S {
  int x;
  int y;
} S;


static int test_struct_plain()
{
  printf("test_struct_plain:\n");

  S s = {
    get(1),
    get(2),
  };

  return
    s.x == 1 &&
    s.y == 2 &&
    1;
}


static int test_struct_desig()
{
  printf("test_struct_desig:\n");

  S s = {
    .x = get(1),
    .y = get(2),
  };

  return
    s.x == 1 &&
    s.y == 2 &&
    1;
}


// Again, both compilers call 'get(1)' before 'get(2)'.
static int test_struct_swap()
{
  printf("test_struct_swap:\n");

  S s = {
    .y = get(2),
    .x = get(1),
  };

  return
    s.x == 1 &&
    s.y == 2 &&
    1;
}


int main()
{
  return
    test_array_plain() &&
    test_array_desig() &&
    test_array_swap() &&
    test_struct_plain() &&
    test_struct_desig() &&
    test_struct_swap() &&
    1? 0 : 1;
}

// EOF
