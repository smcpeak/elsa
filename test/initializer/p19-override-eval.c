// p19-override-eval.c
// C11 6.7.9/19: Testing evaluation of overridden initializer expressions.

// C11 6.7.9/19: "The initialization shall occur in initializer list
// order, each initializer provided for a particular subobject
// overriding any previously listed initializer for the same
// subobject[151]; ... .  [151]: Any initializer for the subobject which
// is overridden and so not used to initialize that subobject might not
// be evaluated at all."


extern int printf(char const *format, ...);

static int get(int v)
{
  printf("  called get(%d)\n", v);
  return v;
}


static int test_array()
{
  printf("test_array:\n");

  // Both GCC and Clang issue a warning, explaining that get(1) will not
  // be called.
  int arr[2] = {
    [0] = get(1),
    [0] = get(2),
    [1] = 3
  };

  return
    arr[0] == 2 &&
    arr[1] == 3 &&
    1;
}


typedef struct S {
  int x;
  int y;
} S;

static int test_struct()
{
  printf("test_struct:\n");

  // As above, both GCC and Clang issue warnings.
  S s = {
    .x = get(1),
    .x = get(2),
    .y = 3
  };

  return
    s.x == 2 &&
    s.y == 3 &&
    1;
}


typedef union U {
  int x;
  char y;
} U;


// For this case, Clang warns but GCC is silent.
static int test_union_no_side_effect()
{
  U u = {
    .x = 0x0103,
    .y = 2
  };

  // Both compilers appear to treat the initialization of 'y' as
  // initializing the entire object 'u', zeroing out the remainder of
  // 'x', even though a naive sequence of assignments would not.
  return
    u.x == 2 &&
    u.y == 2 &&
    1;
}


// Both compilers warn.
static int test_union()
{
  printf("test_union:\n");

  U u = {
    .x = get(0x0103),
    .y = get(2)
  };

  return
    u.x == 2 &&
    u.y == 2 &&
    1;
}


int main()
{
  return
    test_array() &&
    test_struct() &&
    test_union_no_side_effect() &&
    test_union() &&
    1? 0 : 1;
}

// EOF
