// t-const-lshift1.cc
// Left-shift in a constant context.

// This is fine.
typedef int a1[1 << 30];

// This is invalid.  The result is implementation-defined due to not
// being representable as 'int' (assuming sizeof(int)==4), but being
// representable as 'unsigned int'.  GCC does modular reduction to make
// it representable, but that yields -0x8000_0000, and a negative size
// is not allowed.
//ERROR(1): typedef int a2[1 << 31];

// This has undefined behavior because left-shifting a negative is
// always UB.  GCC seems to allow it, yielding -8, and then complaining
// that the size is negative.
//ERROR(2): typedef int a3[-1 << 3];

void foo()
{
  __elsa_constEval(1 << 30, 1073741824);
  __elsa_constEval(1 << 31, -2147483647 - 1);
}
