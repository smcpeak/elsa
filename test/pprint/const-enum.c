// const-enum.c
// "const" before "enum".

// Previously, this was printed with an extra "const", which was not
// followed by a space, making the output doubly wrong.
const enum E1 { enumerator1 } e1;

const enum E2 { enumerator2 } volatile e2;

const enum E1 e1b;
enum E1 const e1c;

const enum { enumerator3 } e3;
const enum { enumerator4 } volatile e4;

// EOF
