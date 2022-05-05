// dsw: from cqual/tests/linux/rtc.i; I think these are from the real
// linux kernel

typedef struct { volatile int counter; } atomic_t;
static __inline__ void atomic_add(int i, volatile atomic_t *v)
{
  // 2022-05-04: Previously, this type just appeared inside the cast
  // operator, but both GCC and Elsa reject that.
  typedef volatile struct { int a[100]; } SomeStruct;

  __asm__ __volatile__("lock ; "  "addl %1,%0"
                       // two colons here
                       :"=m" ((*(SomeStruct *) v ) )
                       :"ir" (i), "m" ((*(SomeStruct *) v ) ));
}

typedef struct __dummy_lock_t {} __dummy_lock_t;

typedef struct {
  volatile unsigned int lock;
} rwlock_t;
extern inline void read_lock(rwlock_t *rw)
{
  do {
    if (true) // don't have this: (__builtin_constant_p( rw ))
      asm volatile
        ("lock ; "
         "subl $1,%0\n\t"
         "js 2f\n"
         "1:\n"
         ".section .text.lock,\"ax\"\n"
         "2:\tpushl %%eax\n\t"
         "leal %0,%%eax\n\t"
         "call "
         "__read_lock_failed"
         "\n\t"
         "popl %%eax\n\t"
         "jmp 1b\n"
         ".previous"
         :"=m" ((*(__dummy_lock_t *)(   rw   )) ) // only one colon here
         ) ;
    else
      asm volatile
        ("lock ; "
         "subl $1,(%0)\n\t"
         "js 2f\n"
         "1:\n"
         ".section .text.lock,\"ax\"\n"
         "2:\tcall "
         "__read_lock_failed"
         "\n\t"
         "jmp 1b\n"
         ".previous"
         :
         :"a" (  rw  )
         : "memory"             // three colons here
         ) ;
  } while (0) ;
}

// this is another copy but retaining the double colon, which is a
// single token in C++
extern inline void read_lock2(rwlock_t *rw)
{
	do {
          if (true)//__builtin_constant_p( rw ))
            asm volatile("lock ; "  "subl $1,%0\n\t" "js 2f\n" "1:\n" ".section .text.lock,\"ax\"\n" "2:\tpushl %%eax\n\t" "leal %0,%%eax\n\t" "call "     "__read_lock_failed"   "\n\t" "popl %%eax\n\t" "jmp 1b\n" ".previous" :"=m" ((*(__dummy_lock_t *)(   rw   )) )) ; else asm volatile("lock ; "  "subl $1,(%0)\n\t" "js 2f\n" "1:\n" ".section .text.lock,\"ax\"\n" "2:\tcall "     "__read_lock_failed"   "\n\t" "jmp 1b\n" ".previous"
  ::"a" (  rw  )                // NOTE double colon!
  : "memory") ;
        } while (0) ;
}

void triple() {
  // three-colons now works also!
  asm ("asdfasd" ::: "cc" );

  // 2022-05-04: The following wouldn't be accepted by GCC because it
  // would require the 'goto' keyword.  I haven't added that detail to
  // the Elsa grammar yet so I'm just commenting this.
  // and four
  //asm ("asdfasd" :::: "a"(rw) );
}

//  /home/dsw/oink_extra/ballAruns/tmpfiles/./arts-1.1-7/gsldatahandle-mad-04hG.i:2145:107: Parse error (state 222) at <string literal>: "fpatan"

typedef unsigned int guint32;
typedef guint32 CORBA_unsigned_long;
typedef unsigned char guchar;
void f(guint32 __v, guchar *_ORBIT_curptr) {
        __asm__
        // 2022-05-04: GCC-9.3.0 rejects "const" here.
        //__const__       // "const" is also legal after an asm
          ("rorw $8, %w0\n\t" "rorl $16, %0\n\t" "rorw $8, %w0": "=r" (__v):"0"
           ((guint32)
            (*((guint32 *) _ORBIT_curptr))));
}
