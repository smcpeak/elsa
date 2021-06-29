// longlines2.c
// printf with big arguments

typedef unsigned long long size_t;

typedef unsigned long long uint64_t;

typedef long long intptr_t;

struct av_oo_pointer_struct {
  uint64_t m_ordinal;
  intptr_t m_offset;
};

typedef struct av_oo_pointer_struct av_oo_pointer_t;

void *av_oo_ptr_check(av_oo_pointer_t p, size_t size);

_Bool av_oo_ptr_cmp_eq(av_oo_pointer_t p1, av_oo_pointer_t p2);

int printf(char const *fmt, ...);

void f()
{
  av_oo_pointer_t __ptr_to_p;
  av_oo_pointer_t __ptr_to_q;
  av_oo_pointer_t pp;

  printf("**pp=%d pp=%s\n",
    *((int *)av_oo_ptr_check(*((av_oo_pointer_t *)av_oo_ptr_check(pp, 16)), 4)),
    av_oo_ptr_cmp_eq(pp, __ptr_to_p)?
      "&p" :
      (av_oo_ptr_cmp_eq(pp, __ptr_to_q)? "&q" : "?"));
}

// EOF
