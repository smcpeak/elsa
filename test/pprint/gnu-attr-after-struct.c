// gnu-attr-after-struct.c
// Attributes after "struct" or "union".

struct __attribute__((deprecated)) S1 {};

struct __attribute__((deprecated)) S1 s1;

const struct __attribute__((deprecated)) S2 {} s2a;

const struct __attribute__((deprecated)) S2 s2b;

// EOF
