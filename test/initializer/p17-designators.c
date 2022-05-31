// p17-designators.c
// C11 6.7.9/17, 18, 20: Designated initializers.

// Test a designator operating in a 2D structure, picking a spot and
// then continuing from there.
//
//  0  1  2  3
//  4  5  6  7
//  8  9 10 11
int arr34a[3][4] = {
  0, 1,
  [1][3] = 7, 8, 9,
  [1][0] = 4, 5,
  [2][2] = 10, 11,
  [0][2] = 2, 3,
  [1][2] = 6,
};

static int test_arr34a()
{
  for (int i=0; i < 3; i++) {
    for (int j=0; j < 4; j++) {
      if (!( arr34a[i][j] == i*4 + j )) {
        return 0;
      }
    }
  }

  return 1;
}


// Mix of brace levels.
int arr34b[3][4] = {
  0, 1, 2, 3,
  { 4, 5, 6, 7 },
  8, 9, 10, 11
};

static int test_arr34b()
{
  for (int i=0; i < 3; i++) {
    for (int j=0; j < 4; j++) {
      if (!( arr34b[i][j] == i*4 + j )) {
        return 0;
      }
    }
  }

  return 1;
}


int arr45a[4][5] = {
  0, 1, 2, 3, 4,
  { 5, 6, 7, 8, 9 },
  10, 11, 12, 13, 14,
  { 15, 16, 17, 18, 19 },
};

static int test_arr45a()
{
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 5; j++) {
      if (!( arr45a[i][j] == i*5 + j )) {
        return 0;
      }
    }
  }

  return 1;
}


// Using both single-level and two-level designators to jump around.
int arr45b[4][5] = {
  [0][3] = 3, 4,
  { 5, 6, [3] = 8, 9, [2] = 7 },
  10, 11,
  [3] = { 15, 16, 17, 18, 19 },

  [2][2] = 12, 13, 14,
  [0][0] = 0, 1, 2,
};

static int test_arr45b()
{
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 5; j++) {
      if (!( arr45b[i][j] == i*5 + j )) {
        return 0;
      }
    }
  }

  return 1;
}


// Single-level designator to jump around.
int arr45c[4][5] = {
  [1] = { 5, 6, 7, 8, 9 },
  { 10, 11, 12, 13, 14 },
  15, 16, 17, 18, 19,
  [0] = 0, 1, 2, 3, 4,
};

static int test_arr45c()
{
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 5; j++) {
      if (!( arr45c[i][j] == i*5 + j )) {
        return 0;
      }
    }
  }

  return 1;
}


typedef struct S {
  int x;
  int a[3];
  int b[5];
  int y;
} S;

// Reference value.
S s1 = {
  1,
  { 2, 3, 4 },
  { 5, 6, 7, 8, 9 },
  10
};

S s2 = {
  .a[1] = 3, 4, 5, 6,
  .y = 10,
  .b[2] = 7, 8, 9,
  .x = 1, 2,
};

S s3 = {
  .a = { 2, 3, 4 },
  { 5, 6, 7, 8, 9 },
  10,
  .x = 1,
};

static int check_s(S *s)
{
  return
    s->x == 1 &&
    s->a[0] == 2 &&
    s->a[1] == 3 &&
    s->a[2] == 4 &&
    s->b[0] == 5 &&
    s->b[1] == 6 &&
    s->b[2] == 7 &&
    s->b[3] == 8 &&
    s->b[4] == 9 &&
    s->y == 10 &&
    1;
}

static int test_s()
{
  return
    check_s(&s1) &&
    check_s(&s2) &&
    check_s(&s3) &&
    1;
}


/* S again, for reference:
typedef struct S {
  int x;
  int a[3];
  int b[5];
  int y;
} S;
*/

typedef struct T {
  int c[2];
  S s1;
  int d;
  S s2;
  int e;
} T;

// Reference value.
T t1 = {
  { 20, 21 },
  { 1, { 2, 3, 4 }, { 5, 6, 7, 8, 9 }, 10 },
  22,
  { 1, { 2, 3, 4 }, { 5, 6, 7, 8, 9 }, 10 },
  23,
};

T t2 = {
  20,
  .s1.a[2] = 4, 5, 6, 7, 8, 9, 10, 22, 1, 2,
  .s2.b = { 5, [2] = 7, 8, 9, [1] = 6 },
  10, 23,
  .c[1] = 21, 1, 2, 3,
  .s2.a[1] = 3, 4,
};

static int check_t(T *t)
{
  return
    t->c[0] == 20 &&
    t->c[1] == 21 &&
    check_s(&(t->s1)) &&
    t->d == 22 &&
    check_s(&(t->s2)) &&
    t->e == 23 &&
    1;
}

static int test_t()
{
  return
    check_t(&t1) &&
    check_t(&t2) &&
    1;
}


typedef union U {
  int i;
  char c[4];
} U;

U uarr[5] = {
  0x44434241,
  { .c = "ABCD" },
  [2] = { .c = { [2] = 'C', 'D', [0] = 'A', 'B' }  },
  0x44434241,
  0x44434241,
};

// Can't just immediately name a field; would have to open a brace or
// use an array index designator first.
//ERROR(field-desig-in-array): U uarr_wrong1[1] = { .i = 5 };

static int check_u(U *u)
{
  // Assumes little endian.
  return
    u->i == 0x44434241 &&
    u->c[0] == 'A' &&
    u->c[1] == 'B' &&
    u->c[2] == 'C' &&
    u->c[3] == 'D' &&
    1;
}

static int test_uarr()
{
  return
    check_u(uarr+0) &&
    check_u(uarr+1) &&
    check_u(uarr+2) &&
    check_u(uarr+3) &&
    check_u(uarr+4) &&
    1;
}


int main()
{
  return
    test_arr34a() &&
    test_arr34b() &&
    test_arr45a() &&
    test_arr45b() &&
    test_arr45c() &&
    test_s() &&
    test_t() &&
    test_uarr() &&
    1? 0 : 1;
}


// EOF
