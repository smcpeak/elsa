// array-of-char-in-func.c
// C11 6.7.9/14: Like array-of-char.c, but inside functions (block scope).

// This set of tests was made by copying those tests from
// array-of-char.c that seemed potentially interesting to also exercise
// at block scope.  The function names are all the same so it is easy to
// correlate them.


static int test_str_hello()
{
  char str_hello[6] = "hello";

  // Initializer is too long.
  //ERROR(init-too-long): char str_hello_too_long[4] = "hello";
  //NOTWORKING(elsa): Rule not enforced.

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


static int test_str_short_init()
{
  // Initializer shorter than declared size.
  char str_short_init[6] = "hi";

  return
    str_short_init[0] == 'h' &&
    str_short_init[1] == 'i' &&
    str_short_init[2] == '\0' &&

    // These elements *are* definitely zero-initialized.  C11 6.7.9/10,
    // regarding automatic storage duration, only applies if the entire
    // declared object has no initializer.  Here, C11 6.7.9/19 applies:
    // "all subobjects that are not initialized explicitly shall be
    // initialized implicitly the same as objects that have static
    // storage duration."  Also C11 6.7.9/21 applies, explicitly
    // covering string literals.
    str_short_init[3] == '\0' &&
    str_short_init[4] == '\0' &&
    str_short_init[5] == '\0' &&

    sizeof(str_short_init) == 6 &&
    1;
}


static int test_str_unspec_size()
{
  // Unspecified size is set by the initializer.
  char str_unspec_size[] = "hello";

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


static int test_str_trunc_nul()
{
  // If there is not room for the NUL, it is dropped.
  char str_trunc_nul[5] = "hello";

  return
    str_trunc_nul[0] == 'h' &&
    str_trunc_nul[1] == 'e' &&
    str_trunc_nul[2] == 'l' &&
    str_trunc_nul[3] == 'l' &&
    str_trunc_nul[4] == 'o' &&
    sizeof(str_trunc_nul) == 5 &&
    1;
}


static int test_str_braces()
{
  // Optionally enclosed in braces.
  char str_braces[3] = { "hi" };

  return
    str_braces[0] == 'h' &&
    str_braces[1] == 'i' &&
    str_braces[2] == '\0' &&
    sizeof(str_braces) == 3 &&
    1;
}



typedef struct S {
  int x;
  char arr[3];
} S;


static int test_s_baseline()
{
  // Initialize an array inside a struct.
  S s_baseline = { 1, "hi" };

  return
    s_baseline.x = 1 &&
    s_baseline.arr[0] == 'h' &&
    s_baseline.arr[1] == 'i' &&
    s_baseline.arr[2] == '\0' &&
    sizeof(s_baseline.arr) == 3 &&
    1;
}


static int test_s_designator()
{
  // Use a field designator before the initializer.
  S s_designator = { .x = 1, .arr = "hi" };

  return
    s_designator.x = 1 &&
    s_designator.arr[0] == 'h' &&
    s_designator.arr[1] == 'i' &&
    s_designator.arr[2] == '\0' &&
    sizeof(s_designator.arr) == 3 &&
    1;
}


typedef union U {
  char arr[4];
  int x;
} U;


static int test_u_baseline()
{
  // Initialize union member with string literal.
  U u_baseline = { "ABC" };

  return
    u_baseline.arr[0] == 'A' &&
    u_baseline.arr[1] == 'B' &&
    u_baseline.arr[2] == 'C' &&
    u_baseline.arr[3] == '\0' &&
    u_baseline.x == 0x00434241 &&      // Assumes little-endian.
    sizeof(u_baseline.arr) == 4 &&
    1;
}


static int test_aoa_baseline()
{
  // Array of array (AoA) of char.
  char aoa_baseline[2][3] = { "ab", "yz" };

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


static int test_aoa_nul_trunc()
{
  // AoA with truncated NUL.
  char aoa_nul_trunc[2][3] = { "abc", "xyz" };

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


static int test_aoa_unspec_major_dim()
{
  // AoA with unspecified major dimension.
  char aoa_unspec_major_dim[][3] = { "ab", "yz" };

  // AoA with unspecified minor dimension.
  //ERROR(aoa-unspec-minor-dim): char aoa_unspec_minor_dim[2][] = { "ab", "yz" };
  //NOTWORKING(elsa): Rule not enforced.


  // AoA with both dimensions unspecified.
  //ERROR(aoa-unspec-both-dim): char aoa_unspec_both_dim[][] = { "ab", "yz" };
  //NOTWORKING(elsa): Rule not enforced.

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


int main()
{
  if (
    test_str_hello() &&
    test_str_short_init() &&
    test_str_unspec_size() &&
    test_str_trunc_nul() &&
    test_str_braces() &&
    test_s_baseline() &&
    test_s_designator() &&
    test_u_baseline() &&
    test_aoa_baseline() &&
    test_aoa_nul_trunc() &&
    test_aoa_unspec_major_dim() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
