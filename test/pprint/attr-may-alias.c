// attr-may-alias.c
// Exercise the 'may_alias' GCC attribute.

// This code is closely modeled on the code in the GCC manual:
// https://gcc.gnu.org/onlinedocs/gcc-4.0.2/gcc/Type-Attributes.html

// A pointer to this type is not assumed to only point at objects
// declared with 'short'.
typedef short __attribute__((__may_alias__)) short_a;
//typedef short short_a;

int main()
{
  // The original example used hexadecimal, but my run-compare-expect.py
  // script mangles large hex values, so I use decimal here instead.
  int a = 12345678;

  short_a *b = (short_a *) &a;

  // Under strict aliasing rules, the compiler would assume that 'b'
  // does not point at 'a', and hence this would have no effect on the
  // subsequent test of 'a'.
  //
  // Except, in my testing with GCC-9.3.0, even without may_alias and
  // with -O2 -fstrict-aliasing, the compiler always lets this modify
  // the value of 'a'.  So I do not have a working run-time test of this
  // feature.
  b[1] = 0;

  if (a == 12345678) {
    // If we get here, the compiler has applied strict aliasing despite
    // the attribute.
    return 1;
  }

  return 0;
}


// Associate the attribute with a declarator.
// Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
int * /**/  __attribute__((may_alias)) /**/     /**/                    a1;
int * const __attribute__((may_alias)) /**/     /**/                    a2;
int * const __attribute__((may_alias)) volatile /**/                    a3;
int * /**/  __attribute__((may_alias)) volatile __attribute__((unused)) a4;
int * /**/  __attribute__((may_alias)) /**/     __attribute__((unused)) a5;


// EOF
