// long-parameters.c
// Pretty print function definition with long parameter decls.

// This one wraps to a point indented relative to the start of 'void',
// which seems fine.
void foobarbaz(const int __attribute__ ((__unused__)) x,
  const int __attribute__ ((__unused__)) y);

// This one is currently wrapping to indent relative to 'foobarbaz',
// which looks weird.
void foobarbaz(const int __attribute__ ((__unused__)) x,
       const int __attribute__ ((__unused__)) y)
{
  return;
}

// EOF
