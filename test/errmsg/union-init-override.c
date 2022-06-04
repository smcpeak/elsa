// union-init-override.c
// Exercise some cases of overriding union member initializers.


union U1 {
  int x;
  int y;
};

// Override with same field.
union U1 u1a = { .x = 1, .x = 2 };

// Override with different field
union U1 u1b = { .x = 1, .y = 2 };


struct S1 {
  int a;
  int b;
};

struct S2 {
  int c;
  int d;
};

union U2 {
  struct S1 s1;
  struct S2 s2;
  int e;
};

// Override level 1 with level 2.
union U2 u2a = { .e = 1, .s1.a = 2 };

// Override level 2 with level 1.
union U2 u2b = { .s1.a = 1, .e = 2 };

// Override level 2 with level 2, same union member.
union U2 u2c = { .s1.a = 1, .s1.a = 2 };

// Augment level 2 with level 2, no override.
union U2 u2d = { .s1.a = 1, .s1.b = 2 };

// Override level 2 with level 2, different union member.
union U2 u2e = { .s1.a = 1, .s2.c = 2 };


// EOF
