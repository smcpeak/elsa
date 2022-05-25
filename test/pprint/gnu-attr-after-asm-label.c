// gnu-attr-after-asm-label.c
// Attribute after a GNU asm label.

// Asm labels:
// https://gcc.gnu.org/onlinedocs/gcc/Asm-Labels.html
//
// I do not currently retain these asm labels.
int foo asm("myfoo") = 2;

// Putting attributes after them is then just the usual ability to
// associate attributes with declarators that happen to also have an
// asm label.
//
// The point of this test, for now, is to show that the attributes *are*
// retained, even if the asm label is not.
int foo2 asm("myfoo2") __attribute__((unused)) = 2;

// EOF
