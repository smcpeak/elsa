// anon-union-in-struct.c
// An anonymous union inside a struct.

// Anonymous union in struct.
typedef struct {
  int x;
  union {
    int y;
    char z;
    struct {
      int q;
    } st;

    // Not allowed because it would create an ambiguity.
    //ERROR(ambig-x): int x;
  };
} S1;

int f1()
{
  S1 s;

  s.y = 0;
  if (!(
    s.z == 0 &&
    s.st.q == 0 &&
  1)) {
    return 0;
  }

  s.y = -1;
  if (!(
    s.z != 0 &&
    s.st.q == -1 &&
  1)) {
    return 0;
  }

  return 1;
}


// Anonymous struct in struct.
struct S2 {
  struct {
    int x;
    int y;
  };
  int z;
};

int f2()
{
  struct S2 s2 = { 1,2,3 };

  return
    s2.x == 1 &&
    s2.y == 2 &&
    s2.z == 3 &&
    1;
}


// Two anonymous unions.
struct S3 {
  union {
    int x;
    int y;
  };
  union {
    int a;
    int b;

    // Another ambiguity.
    //ERROR(ambig-y): int y;
  };
};

int f3()
{
  struct S3 s3 = { { 1 }, { 2 } };

  struct S3 s3b = s3;
  s3b.y = 3;
  s3b.b = 4;

  return
    s3.x == 1 &&
    s3.y == 1 &&
    s3.a == 2 &&
    s3.b == 2 &&
    s3b.x == 3 &&
    s3b.y == 3 &&
    s3b.a == 4 &&
    s3b.b == 4 &&
    1;
}


// Nested anonymous.
struct S4 {
  union {                    // (1)
    union {                  // (2)
      int x;
    };
    struct {                 // (3)
      int y;
    };
    int z;
  };
  struct {                   // (4)
    union {                  // (5)
      int a;
    };
    struct {                 // (6)
      int b;
    };
    int c;
  };
};

int f4()
{
  struct S4 s4 = {
    { 1 },                   // (1): 'x' in (2).
    {                        // (4)
      { 2 },                 //   'a' in (5).
      { 3 },                 //   'b' in (6).
      4                      // 'c' in (4).
    }
  };

  return
    s4.x == 1 &&
    s4.y == 1 &&
    s4.z == 1 &&
    s4.a == 2 &&
    s4.b == 3 &&
    s4.c == 4 &&
    1;
}


int main()
{
  return
    f1() &&
    f2() &&
    f3() &&
    f4() &&
    1? 0 : 1;
}

// EOF
