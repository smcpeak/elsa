// gnu-attr-after-enum.c
// Attributes right after "enum".

// This marks 'E1' as deprecated.
enum __attribute__((deprecated)) E1 { enumerator1 };

// Provokes GCC warning.
enum E1 e1;

// The attribute is associated with the declaration as a whole, meaning
// each declarator, but there are no declarators here, so it is ignored.
__attribute__((deprecated)) enum E2 { enumerator2 };

// No warning.
enum E2 e2;

enum __attribute__((deprecated)) E3 { enumerator3 };
const enum __attribute__((deprecated)) E4 { enumerator4 } e4;

// Attribute within an elaborated type specifier.
enum __attribute__((deprecated)) E3 e3;

// Attribute on anonymous enum.
enum __attribute__((deprecated)) { enumerator5 } e5;

// This marks both 'e6a' and 'e6b' as deprecated.
__attribute__((deprecated)) enum E6 { enumerator6 } e6a, e6b;

// But E6 itself is *not* deprecated.
enum E6 e6c;

void f()
{
  e6a = enumerator6;
  e6b = enumerator6;
}


// EOF
