// invalid-field-desig.c
// Field designator does not name a member.

struct S {
  int x;
  int y;
} s = { .z = 3 };

// EOF
