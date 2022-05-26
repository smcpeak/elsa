// c99-array-declarator-qualifier.c
// C99 feature: cv-qualifiers inside array declarator brackets.

// Currently, this just checks that Elsa does not choke on them.

// Optional qualifiers then optional size.
int f1(int a[]);
int f2(int a[3]);
int f3(int a[const]);
int f4(int a[const 4]);

// Same, but with the parameter name omitted.
int f1o(int []);
int f2o(int [3]);
int f3o(int [const]);
int f4o(int [const 4]);

// EOF
