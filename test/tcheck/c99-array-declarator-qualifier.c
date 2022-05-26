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

// 'static', then optional CV, then required expression.
int g1(int a[static 3]);
int g2(int a[static volatile 3]);
int g1o(int [static 3]);
int g2o(int [static volatile 3]);

// CV, then 'static', then required expression.
int h1(int a[const static 3]);
int h2(int a[const volatile static 3]);
int h1o(int [const static 3]);
int h2o(int [const volatile static 3]);

// Interestingly, it is not legal to intermix CV and 'static'.
//int nope1(int a[const static volatile 3]);

// I'm missing the case of '*' as the size.  For the moment I don't need
// it.

// EOF
