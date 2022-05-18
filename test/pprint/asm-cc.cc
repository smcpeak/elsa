// asm.cc
// Exercise asm definition in C++.

asm("asm at top level");
__asm__("second asm at top level");

void f()
{
  asm("asm inside a function");
  __asm__("second asm inside a function");
}

// EOF
