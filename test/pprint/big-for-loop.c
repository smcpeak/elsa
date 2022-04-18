// big-for-loop.c
// A 'for' loop that has a lot of code in its condition and increment,
// which was printing unsatisfactorily.

// This example is baed on the core of ea-libc/src/string/memcmp.c
// after it has gone through my AV instrumentor.

int dummy12;
int AV_ORD_PTR_CHECK_DEREF(void *p, int dummy);
void *av_ord_ptr_add(void *p, unsigned size);

void f(int i, int n, void *p1, void *p2)
{
  for (i = 0;
         i < n &&
           AV_ORD_PTR_CHECK_DEREF(p1, dummy12) ==
             AV_ORD_PTR_CHECK_DEREF(p2, dummy12);
         i++, p1 = av_ord_ptr_add(p1, 1 * sizeof(dummy12)),
           p2 = av_ord_ptr_add(p2, 1 * sizeof(dummy12))) {
  }
}

// EOF
