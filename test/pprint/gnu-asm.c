// gnu-asm.c
// Exercise GNU assembly definition.

int printf(char const *format, ...);
typedef unsigned uint32_t;
typedef unsigned long uint64_t;
typedef unsigned long size_t;
typedef _Bool bool;

// Something like the original example I'm working with.
unsigned long long f()
{
  unsigned long long a = 0x0ab0000000234432;
  unsigned long long b = 0x0000000000000001;
  unsigned long long c = 0x0000000032123001;
  unsigned long long r1;

  __asm__ __volatile__(
    "mov    %1, %%rax   \n\t"
    "xor    %2, %%rax   \n\t"
    "xor    %3, %%rax   "
    : "=&a" (r1)
    : "g" (a), "g" (b), "g"(c)
    : "cc");   // bogus clobber just to have it

  return r1;
}

// First example in manual.
void ex1()
{
  int src = 1;
  int dst;

  asm("mov %1, %0\n\t"
      "add $1, %0"
      : "=r" (dst)
      : "r" (src));

  printf("%d\n", dst);
}

uint32_t DoCheck(uint32_t dwSomeValue)
{
   uint32_t dwRes;

   // Assumes dwSomeValue is not zero.
   asm ("bsfl %1,%0"
     : "=r" (dwRes)
     : "r" (dwSomeValue)
     : "cc");

   return dwRes;
}

void do_print(uint32_t dwSomeValue)
{
   uint32_t dwRes;

   for (uint32_t x=0; x < 5; x++)
   {
      // Assumes dwSomeValue is not zero.
      asm ("bsfl %1,%0"
        : "=r" (dwRes)
        : "r" (dwSomeValue)
        : "cc");

      printf("%u: %u %u\n", x, dwSomeValue, dwRes);
   }
}

void volatile_example()
{
  uint64_t msr;

  asm volatile ( "rdtsc\n\t"    // Returns the time in EDX:EAX.
          "shl $32, %%rdx\n\t"  // Shift the upper bits left.
          "or %%rdx, %0"        // 'Or' in the lower bits.
          : "=a" (msr)
          :
          : "rdx");

  printf("msr: %lx\n", msr);

  // Do other work...

  // Reprint the timestamp
  asm volatile ( "rdtsc\n\t"    // Returns the time in EDX:EAX.
          "shl $32, %%rdx\n\t"  // Shift the upper bits left.
          "or %%rdx, %0"        // 'Or' in the lower bits.
          : "=a" (msr)
          :
          : "rdx");

  printf("msr: %lx\n", msr);
}

uint64_t double_colon_missing_inputs()
{
  uint64_t msr;

  asm volatile ( "rdtsc\n\t"    // Returns the time in EDX:EAX.
          "shl $32, %%rdx\n\t"  // Shift the upper bits left.
          "or %%rdx, %0"        // 'Or' in the lower bits.
          : "=a" (msr)
          :: "rdx");

  return msr;
}

int mtfsf1(int x, int y, uint64_t fpenv)
{
  int sum;
  asm volatile("mtfsf 255, %0" : : "r" (fpenv));
  sum = x + y;
  return sum;
}

int mtfsf2(int x, int y, uint64_t fpenv)
{
  int sum;
  asm volatile ("mtfsf 255,%1" : "=X" (sum) : "r" (fpenv));
  sum = x + y;
  return sum;
}

bool old_ex(uint32_t *Base, uint32_t Offset)
{
  bool old;

  __asm__ ("btsl %2,%1\n\t" // Turn on zero-based bit #Offset in Base.
           "sbb %0,%0"      // Use the CF to calculate old.
     : "=r" (old), "+rm" (*Base)
     : "Ir" (Offset)
     : "cc");

  return old;
}

void output_non_lvalue()
{
  //ERROR(1): asm("something" : "=r" (3) : );
}

uint32_t missing_inputs()
{
  uint32_t result;
  asm("blah" : "=r" (result));
  return result;
}

void double_colon_missing_outputs(int x)
{
  asm("gorf" :: "r" (x));
}

void triple_colon_with_clobbers()
{
  asm("oh baby" ::: "cc");
}

void triple_colon_without_clobbers()
{
  asm("oh baby" :::);
}

uint32_t no_symname()
{
  uint32_t Mask = 1234;
  uint32_t Index;

  asm ("bsfl %1, %0"
     : "=r" (Index)
     : "r" (Mask)
     : "cc");

  return Index;
}

uint32_t with_symname()
{
  uint32_t Mask = 1234;
  uint32_t Index;

  asm ("bsfl %[aMask], %[aIndex]"
     : [aIndex] "=r" (Index)
     : [aMask] "r" (Mask)
     : "cc");

  return Index;
}

void another_output_op_example()
{
  uint32_t c = 1;
  uint32_t d;
  uint32_t *e = &c;

  asm ("mov %[e], %[d]"
     : [d] "=rm" (d)
     : [e] "rm" (*e));
}

void no_output_operands(uint32_t Offset)
{
  __asm__ ("some instructions"
     : /* No outputs. */
     : "r" (Offset / 8));
}

uint32_t combine(uint32_t foo, uint32_t bar)
{
  asm ("combine %2, %0"
     : "=r" (foo)
     : "0" (foo), "g" (bar));
  return foo;
}

uint32_t cmoveq(uint32_t test, uint32_t new, uint32_t old)
{
  uint32_t result;
  asm ("cmoveq %1, %2, %[result]"
     : [result] "=r"(result)
     : "r" (test), "r" (new), "[result]" (old));
  return result;
}

void vax_clobber(uint32_t from, uint32_t to, uint32_t count)
{
  asm volatile ("movc3 %0, %1, %2"
                     : /* No outputs. */
                     : "g" (from), "g" (to), "g" (count)
                     : "cc", "cc", /*"r0", "r1", "r2", "r3", "r4", "r5",*/ "memory");
}

uint32_t sumsq(uint32_t *x, uint32_t *y)
{
  uint32_t result = 0;
  asm ("sumsq %0, %1, %2"
       : "+r" (result)
       : "r" (x), "r" (y), "m" (*x), "m" (*y));
  return result;
}

void vecmul(uint32_t *z, uint32_t *x, uint32_t *y)
{
  asm ("vecmul %0, %1, %2"
       : "+r" (z), "+r" (x), "+r" (y), "=m" (*z)
       : "m" (*x), "m" (*y));
}

uint32_t unknown_length(void *p)
{
  uint32_t count;
  asm("repne scasb"
      : "=c" (count), "+D" (p)
      : "m" (*(const char (*)[]) p), "0" (-1), "a" (0));
  return count;
}

// "impossible constraints" ... whatever
//void
//dscal (size_t n, double *x, double alpha)
//{
//  asm ("/* lots of asm here */"
//       : "+m" (*(double (*)[n]) x), "+&r" (n), "+b" (x)
//       : "d" (alpha), "b" (32), "b" (48), "b" (64),
//         "b" (80), "b" (96), "b" (112)
//       : "cc" //"cr0",
//         //"vs32","vs33","vs34","vs35","vs36","vs37","vs38","vs39",
//         //"vs40","vs41","vs42","vs43","vs44","vs45","vs46","vs47");
//         );
//}


// EOF
