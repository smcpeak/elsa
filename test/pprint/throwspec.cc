// throwspec.cc
// Exception specs on functions.
// Based on t0023.cc.

int foo() throw();

int foo() throw()
{
  return 3;
}

class Exc {};
class Exc2 {};

void bar() throw(Exc);
void bar2() throw(Exc, Exc2);

// EOF
