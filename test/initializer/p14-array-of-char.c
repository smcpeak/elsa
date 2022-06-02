// p14-array-of-char.c
// C11 6.7.9/14: Initialize array of char with string literal.

// Also test C11 6.7.9/22, initializing array of unknown size, in the
// case where an array of char is initialized with a string literal.

// Dimensions tested:
//
//   - array size: { specified, unspecified(err?) }
//   - init size: { short, just-right, nul-trunc, too-long(err) }
//   - element type: { char, unsigned char, signed char }
//   - level of braces around string lit: { none, one, two(err) }
//   - object context: { top-level, struct member, union member, array element }
//   - designator: { absent, present, swapped }
//
// Not every combination is tested, but a substantial fraction are.  The
// function names reflect the tested combination, and are all easily
// reviewed by looking at what main() calls.
//
// The possibility of the array being inside a function is tested in
// array-of-char-in-func.c.

// TODO: Test UTF-8 string literals.
// TODO: Test wchar_t string literals.


char str_hello[6] = "hello";

static int test_str_hello()
{
  return
    str_hello[0] == 'h' &&
    str_hello[1] == 'e' &&
    str_hello[2] == 'l' &&
    str_hello[3] == 'l' &&
    str_hello[4] == 'o' &&
    str_hello[5] == '\0' &&
    sizeof(str_hello) == 6 &&
    1;
}

// Initializer is too long.
//ERROR(init-too-long): char str_hello_too_long[4] = "hello";


// Grouping parens are not allowed.
//ERROR(grouping-parens-strlit-init): char str_hello_parens[6] = ("hello");
//NOTWORKING(clang): Rule not enforced.


// Initializer shorter than declared size.
char str_short_init[6] = "hi";

static int test_str_short_init()
{
  return
    str_short_init[0] == 'h' &&
    str_short_init[1] == 'i' &&
    str_short_init[2] == '\0' &&
    str_short_init[3] == '\0' &&
    str_short_init[4] == '\0' &&
    str_short_init[5] == '\0' &&
    sizeof(str_short_init) == 6 &&
    1;
}


// Unspecified size is set by the initializer.
char str_unspec_size[] = "hello";

static int test_str_unspec_size()
{
  return
    str_unspec_size[0] == 'h' &&
    str_unspec_size[1] == 'e' &&
    str_unspec_size[2] == 'l' &&
    str_unspec_size[3] == 'l' &&
    str_unspec_size[4] == 'o' &&
    str_unspec_size[5] == '\0' &&
    sizeof(str_unspec_size) == 6 &&
    1;
}


// If there is not room for the NUL, it is dropped.
char str_trunc_nul[5] = "hello";

static int test_str_trunc_nul()
{
  return
    str_trunc_nul[0] == 'h' &&
    str_trunc_nul[1] == 'e' &&
    str_trunc_nul[2] == 'l' &&
    str_trunc_nul[3] == 'l' &&
    str_trunc_nul[4] == 'o' &&
    sizeof(str_trunc_nul) == 5 &&
    1;
}


// Works for 'unsigned char'.
unsigned char str_unsigned_char[3] = "hi";

static int test_str_unsigned_char()
{
  return
    str_unsigned_char[0] == 'h' &&
    str_unsigned_char[1] == 'i' &&
    str_unsigned_char[2] == '\0' &&
    sizeof(str_unsigned_char) == 3 &&
    1;
}


// Works for 'signed char'.
signed char str_signed_char[3] = "hi";

static int test_str_signed_char()
{
  return
    str_signed_char[0] == 'h' &&
    str_signed_char[1] == 'i' &&
    str_signed_char[2] == '\0' &&
    sizeof(str_signed_char) == 3 &&
    1;
}


// Optionally enclosed in braces.
char str_braces[3] = { "hi" };

static int test_str_braces()
{
  return
    str_braces[0] == 'h' &&
    str_braces[1] == 'i' &&
    str_braces[2] == '\0' &&
    sizeof(str_braces) == 3 &&
    1;
}


// Too many braces.
//
// GCC diagnoses this seemingly by accident, as it initially gives a
// warning, but then complains about "hi" being a pointer initializing a
// char.
//
// Clang does something similar.  And, in fact, Elsa does too, now.
//
//ERROR(str-too-many-braces): char str_too_many_braces[3] = { { "hi" } };


// Truncating NUL works in braces too.
char str_braces_trunc_nul[3] = { "hix" };

static int test_str_braces_trunc_nul()
{
  return
    str_braces_trunc_nul[0] == 'h' &&
    str_braces_trunc_nul[1] == 'i' &&
    str_braces_trunc_nul[2] == 'x' &&
    sizeof(str_braces_trunc_nul) == 3 &&
    1;
}


// Initializer too long in braces.
//ERROR(init-too-long-braces): char str_braces_too_long[3] = { "hixy" };


typedef struct S {
  int x;
  char arr[3];
} S;


// Initialize an array inside a struct.
S s_baseline = { 1, "hi" };

static int test_s_baseline()
{
  return
    s_baseline.x = 1 &&
    s_baseline.arr[0] == 'h' &&
    s_baseline.arr[1] == 'i' &&
    s_baseline.arr[2] == '\0' &&
    sizeof(s_baseline.arr) == 3 &&
    1;
}


// Struct member with NUL truncation.
S s_nul_trunc = { 1, "hix" };

static int test_s_nul_trunc()
{
  return
    s_nul_trunc.x = 1 &&
    s_nul_trunc.arr[0] == 'h' &&
    s_nul_trunc.arr[1] == 'i' &&
    s_nul_trunc.arr[2] == 'x' &&
    sizeof(s_nul_trunc.arr) == 3 &&
    1;
}


// Struct member with extra braces.
S s_braces = { 1, { "hi" } };

static int test_s_braces()
{
  return
    s_braces.x = 1 &&
    s_braces.arr[0] == 'h' &&
    s_braces.arr[1] == 'i' &&
    s_braces.arr[2] == '\0' &&
    sizeof(s_braces.arr) == 3 &&
    1;
}


// Struct member with too many braces.
//ERROR(member-too-many-braces): S s_too_many_braces = { 1, { { "hi" } } };


// Use a field designator before the initializer.
S s_designator = { .x = 1, .arr = "hi" };

static int test_s_designator()
{
  return
    s_designator.x = 1 &&
    s_designator.arr[0] == 'h' &&
    s_designator.arr[1] == 'i' &&
    s_designator.arr[2] == '\0' &&
    sizeof(s_designator.arr) == 3 &&
    1;
}


// Field designator with order swapped.
S s_designator_swap = { .arr = "hi", .x = 1 };

static int test_s_designator_swap()
{
  return
    s_designator_swap.x = 1 &&
    s_designator_swap.arr[0] == 'h' &&
    s_designator_swap.arr[1] == 'i' &&
    s_designator_swap.arr[2] == '\0' &&
    sizeof(s_designator_swap.arr) == 3 &&
    1;
}


// Field designator with extra braces.
S s_designator_braces = { .x = 1, .arr = { "hi" } };

static int test_s_designator_braces()
{
  return
    s_designator_braces.x = 1 &&
    s_designator_braces.arr[0] == 'h' &&
    s_designator_braces.arr[1] == 'i' &&
    s_designator_braces.arr[2] == '\0' &&
    sizeof(s_designator_braces.arr) == 3 &&
    1;
}


// Field designator with too many braces.
//ERROR(field-designator-too-many-braces): S s_designator_too_many_braces = { .x = 1, .arr = { { "hi" } } };


typedef union U {
  char arr[4];
  int x;
} U;


// Initialize union member with string literal.
U u_baseline = { "ABC" };

static int test_u_baseline()
{
  return
    u_baseline.arr[0] == 'A' &&
    u_baseline.arr[1] == 'B' &&
    u_baseline.arr[2] == 'C' &&
    u_baseline.arr[3] == '\0' &&
    u_baseline.x == 0x00434241 &&      // Assumes little-endian.
    sizeof(u_baseline.arr) == 4 &&
    1;
}


// Union member with extra braces.
U u_braces = { { "ABC" } };

static int test_u_braces()
{
  return
    u_braces.arr[0] == 'A' &&
    u_braces.arr[1] == 'B' &&
    u_braces.arr[2] == 'C' &&
    u_braces.arr[3] == '\0' &&
    sizeof(u_braces.arr) == 4 &&
    1;
}


// Use a designator.
U u_designator = { .arr = "ABC" };

static int test_u_designator()
{
  return
    u_designator.arr[0] == 'A' &&
    u_designator.arr[1] == 'B' &&
    u_designator.arr[2] == 'C' &&
    u_designator.arr[3] == '\0' &&
    sizeof(u_designator.arr) == 4 &&
    1;
}


// Array of array (AoA) of char.
char aoa_baseline[2][3] = { "ab", "yz" };

static int test_aoa_baseline()
{
  return
    aoa_baseline[0][0] == 'a' &&
    aoa_baseline[0][1] == 'b' &&
    aoa_baseline[0][2] == '\0' &&
    aoa_baseline[1][0] == 'y' &&
    aoa_baseline[1][1] == 'z' &&
    aoa_baseline[1][2] == '\0' &&
    sizeof(aoa_baseline) == 6 &&
    sizeof(aoa_baseline[0]) == 3 &&
    1;
}


// AoA with truncated NUL.
char aoa_nul_trunc[2][3] = { "abc", "xyz" };

static int test_aoa_nul_trunc()
{
  return
    aoa_nul_trunc[0][0] == 'a' &&
    aoa_nul_trunc[0][1] == 'b' &&
    aoa_nul_trunc[0][2] == 'c' &&
    aoa_nul_trunc[1][0] == 'x' &&
    aoa_nul_trunc[1][1] == 'y' &&
    aoa_nul_trunc[1][2] == 'z' &&
    sizeof(aoa_nul_trunc) == 6 &&
    sizeof(aoa_nul_trunc[0]) == 3 &&
    1;
}


// Use an array index designator.
char aoa_designator[2][3] = { [0] = "ab", [1] = "yz" };

static int test_aoa_designator()
{
  return
    aoa_designator[0][0] == 'a' &&
    aoa_designator[0][1] == 'b' &&
    aoa_designator[0][2] == '\0' &&
    aoa_designator[1][0] == 'y' &&
    aoa_designator[1][1] == 'z' &&
    aoa_designator[1][2] == '\0' &&
    sizeof(aoa_designator) == 6 &&
    sizeof(aoa_designator[0]) == 3 &&
    1;
}


// Use swapped array index designators.
char aoa_designator_swap[2][3] = { [1] = "yz", [0] = "ab" };

static int test_aoa_designator_swap()
{
  return
    aoa_designator_swap[0][0] == 'a' &&
    aoa_designator_swap[0][1] == 'b' &&
    aoa_designator_swap[0][2] == '\0' &&
    aoa_designator_swap[1][0] == 'y' &&
    aoa_designator_swap[1][1] == 'z' &&
    aoa_designator_swap[1][2] == '\0' &&
    sizeof(aoa_designator_swap) == 6 &&
    sizeof(aoa_designator_swap[0]) == 3 &&
    1;
}


// AoA with unspecified major dimension.
char aoa_unspec_major_dim[][3] = { "ab", "yz" };

static int test_aoa_unspec_major_dim()
{
  return
    aoa_unspec_major_dim[0][0] == 'a' &&
    aoa_unspec_major_dim[0][1] == 'b' &&
    aoa_unspec_major_dim[0][2] == '\0' &&
    aoa_unspec_major_dim[1][0] == 'y' &&
    aoa_unspec_major_dim[1][1] == 'z' &&
    aoa_unspec_major_dim[1][2] == '\0' &&
    sizeof(aoa_unspec_major_dim) == 6 &&
    sizeof(aoa_unspec_major_dim[0]) == 3 &&
    1;
}


// AoA with unspecified minor dimension.
//ERROR(aoa-unspec-minor-dim): char aoa_unspec_minor_dim[2][] = { "ab", "yz" };


// AoA with both dimensions unspecified.
//ERROR(aoa-unspec-both-dim): char aoa_unspec_both_dim[][] = { "ab", "yz" };


// AoA with extra braces.
char aoa_braces[2][3] = { { "ab" }, { "yz" } };

static int test_aoa_braces()
{
  return
    aoa_braces[0][0] == 'a' &&
    aoa_braces[0][1] == 'b' &&
    aoa_braces[0][2] == '\0' &&
    aoa_braces[1][0] == 'y' &&
    aoa_braces[1][1] == 'z' &&
    aoa_braces[1][2] == '\0' &&
    sizeof(aoa_braces) == 6 &&
    sizeof(aoa_braces[0]) == 3 &&
    1;
}


// AoA with too many braces on the first initializer.
//ERROR(aoa-too-many-braces): char aoa_too_many_braces[2][3] = { { { "ab" } }, { "yz" } };


int main()
{
  if (
    test_str_hello() &&
    test_str_short_init() &&
    test_str_unspec_size() &&
    test_str_trunc_nul() &&
    test_str_unsigned_char() &&
    test_str_signed_char() &&
    test_str_braces() &&
    test_str_braces_trunc_nul() &&
    test_s_baseline() &&
    test_s_nul_trunc() &&
    test_s_braces() &&
    test_s_designator() &&
    test_s_designator_swap() &&
    test_s_designator_braces() &&
    test_u_baseline() &&
    test_u_braces() &&
    test_u_designator() &&
    test_aoa_baseline() &&
    test_aoa_nul_trunc() &&
    test_aoa_designator() &&
    test_aoa_designator_swap() &&
    test_aoa_unspec_major_dim() &&
    test_aoa_braces() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
