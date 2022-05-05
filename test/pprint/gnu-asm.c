// gnu-asm.c
// Exercise GNU assembly definition.

unsigned long long f()
{
  unsigned long long a = 0x0ab0000000234432;
  unsigned long long b = 0x0000000000000001;
  unsigned long long c = 0x0000000032123001;
  unsigned long long r1;
  int i = 0;

  __asm__ __volatile__(
    "mov    %1, %%rax   \n\t"
    "xor    %2, %%rax   \n\t"
    "xor    %3, %%rax   "
    : "=&a" (r1)
    : "g" (a), "g" (b), "g"(c));

  return r1;
}

// EOF
