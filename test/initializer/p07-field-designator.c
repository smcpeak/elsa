// p07-field-designator.c
// C11 6.7.9/7: Field designator must name a field of current object.

typedef struct S {
  int x;
  int y;
} S;

S s1 = { 3,4 };

S s2 = { .x = 3, .y = 4 };

//ERROR(bad-field): S s3 = { .x = 3, .z = 4 };
//NOTWORKING(elsa): Rule not enforced.

static int test_s1_s2()
{
  return 1 &&
    s1.x == 3 &&
    s1.y == 4 &&
    s2.x == 3 &&
    s2.y == 4 &&
    1;
}


int arr2a[] = { 1,2 };

// GCC's error message talks about records.  Perhaps the designator
// could apply to a structure element of the array?
//ERROR(field-for-array): int arr2b[] = { 1, .y = 2 };

static int test_arr2a()
{
  return 1 &&
    arr2a[0] == 1 &&
    arr2a[1] == 2 &&
    1;
}


S sarr2a[2] = { 1, 2, 3, 4 };

// Despite the error message, this does not seem to be accepted.
//ERROR(field-for-array-nest1): S sarr2b[2] = { .x = 1,      2, 3, 4 };
//ERROR(field-for-array-nest2): S sarr2c[2] = {      1, .y = 2, 3, 4 };
//NOTWORKING(elsa): Rule not enforced.

static int test_sarr2a()
{
  return 1 &&
    sarr2a[0].x == 1 &&
    sarr2a[0].y == 2 &&
    sarr2a[1].x == 3 &&
    sarr2a[1].y == 4 &&
    1;
}


typedef union U {
  int i;
  char c;
} U;

U u1 = { 0x0302 };

U u2 = { .i = 0x0302 };

U u3 = { .c = -1 };

//ERROR(bad-field2): U u4 = { .x = -1 };
//NOTWORKING(elsa): Rule not enforced.

// Permissible to initialize multiple union elements, and last wins.
U u5 = { .c = -1, .i = 0x0302 };

static int test_u()
{
  // I am assuming little endian here...
  return 1 &&
    u1.i == 0x0302 &&
    u1.c == 0x02 &&
    u2.i == 0x0302 &&
    u2.c == 0x02 &&
    u3.i == 0xFF &&
    u3.c == -1 &&
    u5.i == 0x0302 &&
    u5.c == 0x02 &&
    1;
}


int main()
{
  if (1 &&
    test_s1_s2() &&
    test_arr2a() &&
    test_sarr2a() &&
    test_u() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
