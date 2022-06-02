// p02-init-non-contained.c
// C11 6.7.9/2: "No initializer shall attempt to provide a value for an
// object not contained within the entity being initialized."


int arr2a[2] = { 0, 1 };

int arr2b[2] = { [0] = 0, [1] = 1 };

// Too many initializers.
//ERROR(too-many-array-inits): int arr2c[2] = { 0, 1, 2 };

//ERROR(negative-desig-expr): int arr2d[2] = { [0] = 0, [-1] = 1 };

//ERROR(too-large-desig-expr): int arr2e[2] = { [2] = 2 };

static int test_arr2ab()
{
  return
    arr2a[0] == 0 &&
    arr2a[1] == 1 &&
    arr2b[0] == 0 &&
    arr2b[1] == 1 &&
    1;
}


int arr23a[2][3] = { { 0, 1, 2 }, { 3, 4, 5 } };

// Extra init in inner array.
//ERROR(array-extra-inner1): int arr23b[2][3] = { { 0, 1, 2, 22 }, { 3, 4, 5 } };
//ERROR(array-extra-inner2): int arr23c[2][3] = { { 0, 1, 2 }, { 3, 4, 5, 55 } };

// Extra outer init.
//ERROR(array-extra-outer): int arr23d[2][3] = { { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 } };

static int test_arr23a()
{
  return
    arr23a[0][0] == 0 &&
    arr23a[0][1] == 1 &&
    arr23a[0][2] == 2 &&
    arr23a[1][0] == 3 &&
    arr23a[1][1] == 4 &&
    arr23a[1][2] == 5 &&
    1;
}


// Missing braces.
int mba23a[2][3] = { 0, 1, 2, 3, 4, 5 };

// Elsa gives two weird errors.
//ERROR(mba-too-many): int mba23b[2][3] = { 0, 1, 2, 3, 4, 5, 6 };

static int test_mba23a()
{
  return
    mba23a[0][0] == 0 &&
    mba23a[0][1] == 1 &&
    mba23a[0][2] == 2 &&
    mba23a[1][0] == 3 &&
    mba23a[1][1] == 4 &&
    mba23a[1][2] == 5 &&
    1;
}


// Outer designators.
int oda23a[2][3] = { [0] = { 0, 1, 2 }, [1] = { 3, 4, 5 } };

// Out of bounds outer designator.
//ERROR(array-oob-outer-desig): int oda23b[2][3] = { [0] = { 0, 1, 2 }, [1] = { 3, 4, 5 }, [2] = { 6, 7, 8 } };

static int test_oda23a()
{
  return
    oda23a[0][0] == 0 &&
    oda23a[0][1] == 1 &&
    oda23a[0][2] == 2 &&
    oda23a[1][0] == 3 &&
    oda23a[1][1] == 4 &&
    oda23a[1][2] == 5 &&
    1;
}


// Inner designators.
int ida23a[2][3] = { { [0] = 0, [1] = 1, [2] = 2 }, { [0] = 3, [1] = 4, [2] = 5 } };

//ERROR(array-oob-inner-desig1): int ida23b[2][3] = { { [0] = 0, [1] = 1, [2] = 2, [3] = 22 }, { [0] = 3, [1] = 4, [2] = 5 } };
//ERROR(array-oob-inner-desig2): int ida23c[2][3] = { { [0] = 0, [1] = 1, [2] = 2 }, { [0] = 3, [1] = 4, [2] = 5, [3] = 55 } };

static int test_ida23a()
{
  return
    ida23a[0][0] == 0 &&
    ida23a[0][1] == 1 &&
    ida23a[0][2] == 2 &&
    ida23a[1][0] == 3 &&
    ida23a[1][1] == 4 &&
    ida23a[1][2] == 5 &&
    1;
}


// Combined designators.
int cda23a[2][3] = {
  [0][0] = 0, [0][1] = 1, [0][2] = 2,
  [1][0] = 3, [1][1] = 4, [1][2] = 5
};

// Index goes out of bounds.
//ERROR(cda-oob-inner): int cda23b[2][3] = { [0][0] = 0, [0][1] = 1, [0][3] = 2, [1][0] = 3, [1][1] = 4, [1][2] = 5 };
//ERROR(cda-oob-outer): int cda23c[2][3] = { [0][0] = 0, [0][1] = 1, [0][2] = 2, [2][0] = 3, [1][1] = 4, [1][2] = 5 };

static int test_cda23a()
{
  return
    cda23a[0][0] == 0 &&
    cda23a[0][1] == 1 &&
    cda23a[0][2] == 2 &&
    cda23a[1][0] == 3 &&
    cda23a[1][1] == 4 &&
    cda23a[1][2] == 5 &&
    1;
}


typedef struct S {
  int x;
  int y;
} S;

S s1 = { 1, 2 };

//ERROR(too-many-struct-inits): S s1a = { 1, 2, 3 };

S s2 = { .x = 1, .y = 2 };

//ERROR(bad-field): S s2a = { .x = 1, .y = 2, .z = 3 };

//ERROR(struct-extra-init-after-desig): S s2b = { .x = 1, .y = 2, 3 };

static int test_s1_s2()
{
  return
    s1.x == 1 &&
    s1.y == 2 &&
    s2.x == 1 &&
    s2.y == 2 &&
    1;
}


// Array of Struct.
S plain_aos1[2] = { { 1, 2 }, { 3, 4 } };

//ERROR(aos-outer-too-many): S plain_aos1b[2] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };

//ERROR(aos-inner-too-many1): S plain_aos1c[2] = { { 1, 2, 22 }, { 3, 4 } };
//ERROR(aos-inner-too-many2): S plain_aos1d[2] = { { 1, 2 }, { 3, 4, 44 } };

static int test_plain_aos1()
{
  return
    plain_aos1[0].x == 1 &&
    plain_aos1[0].y == 2 &&
    plain_aos1[1].x == 3 &&
    plain_aos1[1].y == 4 &&
    1;
}


// Designated Outer, Array of Struct.
S do_aos1[2] = { [0] = { 1, 2 }, [1] = { 3, 4 } };

//ERROR(do-aos-oob-outer): S do_aos1b[2] = { [0] = { 1, 2 }, [1] = { 3, 4 }, [2] = { 5, 6 } };

//ERROR(do-aos-extra-inner): S do_aos1c[2] = { [0] = { 1, 2, 3 }, [1] = { 3, 4 } };

static int test_do_aos1()
{
  return
    do_aos1[0].x == 1 &&
    do_aos1[0].y == 2 &&
    do_aos1[1].x == 3 &&
    do_aos1[1].y == 4 &&
    1;
}


// Combined designators.
S comb_aos[2] = {
  [0].x = 1, [0].y = 2,
  [1].x = 3, [1].y = 4
};

//ERROR(comb-aos-oob-outer): S comb_aos_b[2] = { [0].x = 1, [0].y = 2, [2].x = 3, [1].y = 4 };

static int test_comb_aos()
{
  return
    comb_aos[0].x == 1 &&
    comb_aos[0].y == 2 &&
    comb_aos[1].x == 3 &&
    comb_aos[1].y == 4 &&
    1;
}


int main()
{
  return
    test_arr2ab() &&
    test_arr23a() &&
    test_mba23a() &&
    test_oda23a() &&
    test_ida23a() &&
    test_cda23a() &&
    test_s1_s2() &&
    test_plain_aos1() &&
    test_do_aos1() &&
    test_comb_aos() &&
    1? 0 : 1;
}

// EOF
