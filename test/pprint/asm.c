// asm.c
// Exercise asm definition.

asm("asm at top level");

void f()
{
  asm("asm inside a function");
}

// EOF
