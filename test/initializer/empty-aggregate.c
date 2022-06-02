// empty-aggregate.c
// Test with empty aggregates: zero-length array or empty struct.

// Zero-length arrays are prohibited by C11 6.7.6.2p1: "If the
// expression is a constant expression, it shall have a value greater
// than zero."  (And p5 precludes zero-length VLAs.)

// Empty structs are prohibited by the grammar in C11 6.7.2.1p1, which
// has "struct-declaration-list" inside the braces, without any "opt"
// notation.  (But C++ allows them.)

// Empty unions are prohibited for the same reason.  I do not know their
// status in C++.

// However, these are allowed as GNU extensions:
//
// * https://gcc.gnu.org/onlinedocs/gcc/Zero-Length.html
// * https://gcc.gnu.org/onlinedocs/gcc/Empty-Structures.html
//
// The GCC docs do not acknowledge empty unions, but rejects them with
// -pedantic-errors, as does Clang.


int arr0[0] = {};

static int test_arr0()
{
  _Static_assert(sizeof(arr0) == 0, "");
  return 1;
}


int arr20[2][0] = {};

static int test_arr20()
{
  _Static_assert(sizeof(arr20) == 0, "");
  return 1;
}


int arr02[0][2] = {};

static int test_arr02()
{
  _Static_assert(sizeof(arr02) == 0, "");
  return 1;
}


int arr00[0][0] = {};

static int test_arr00()
{
  _Static_assert(sizeof(arr00) == 0, "");
  return 1;
}


char string0[0] = "";

static int test_string0()
{
  _Static_assert(sizeof(string0) == 0, "");
  return 1;
}


char array_of_string0[2][0] = { "", "" };

static int test_array_of_string0()
{
  _Static_assert(sizeof(array_of_string0) == 0, "");
  return 1;
}


typedef struct EmptyStruct {
} EmptyStruct;

EmptyStruct empty_struct = {};

static int test_empty_struct()
{
  _Static_assert(sizeof(empty_struct) == 0, "");
  return 1;
}


typedef struct IESI {
  int x;
  EmptyStruct empty;
  int y;
} IESI;

// Somewhat curiously, GCC requires explicit braces when initializing an
// empty structure, rather than just skipping over them.  Its error
// message is a bit vague (actually a warning, because it just swallows
// the next initializer).  Clang gives a clear error message.  My guess
// is the GCC limitation was unintentional and Clang is enforcing GCC's
// accidental rule.
IESI iesi1 = { 1, {}, 2 };

static int test_iesi()
{
  return
    iesi1.x == 1 &&
    iesi1.y == 2 &&
    1;
}


typedef union EmptyUnion {
} EmptyUnion;

EmptyUnion empty_union = {};

static int test_empty_union()
{
  _Static_assert(sizeof(empty_union) == 0, "");
  return 1;
}


typedef struct IEUI {
  int x;
  EmptyUnion empty;
  int y;
} IEUI;

IEUI ieui1 = { 1, {}, 2 };

static int test_ieui()
{
  return
    ieui1.x == 1 &&
    ieui1.y == 2 &&
    1;
}


typedef struct I_arr0_I {
  int x;
  int arr0[0];
  int y;
} I_arr0_I;

I_arr0_I iai1 = { 1, {}, 2 };

static int test_iai()
{
  return
    iai1.x == 1 &&
    iai1.y == 2 &&
    1;
}


int main()
{
  return
    test_arr0() &&
    test_arr20() &&
    test_arr02() &&
    test_arr00() &&
    test_string0() &&
    test_array_of_string0() &&
    test_empty_struct() &&
    test_empty_union() &&
    test_iesi() &&
    test_ieui() &&
    test_iai() &&
    1? 0 : 1;
}


// EOF
