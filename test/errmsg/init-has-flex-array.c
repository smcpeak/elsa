// init-has-flex-array.c
// Initialize a structure with a flexible array member.

// These aren't diagnosed yet, so the test is disabled.

typedef struct HasFlexArray {
  int x;
  int a[];
} HasFlexArray;

HasFlexArray hfa1 = { 1, { 2,3 } };

HasFlexArray hfa2 = { 1, 2, 3 };

// EOF
