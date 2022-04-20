// for-loop-no-init.c
// Test pprint where a 'for' loop has no initializer but has large
// test and increment expressions.

int av_ord_ptr_cmp_lt(int,int);
int av_ord_ptr_add(int,int);
int av_ord_ptr_preincrement(int*, int);
int av_ord_ptr_postincrement(int*, int);
int AV_ORD_PTR_REGISTER_LOCAL(int);

void f(int p, int q, int r, int buf)
{
  int __ptr_to_buf;

  // Current printing has an unneeded newline after initial ';'.
  for (;
         av_ord_ptr_cmp_lt(p,
           av_ord_ptr_add(__ptr_to_buf = AV_ORD_PTR_REGISTER_LOCAL(buf),
             5 * sizeof(char)));
         av_ord_ptr_preincrement(&p, sizeof(char)),
           av_ord_ptr_preincrement(&q, sizeof(char))) {
    av_ord_ptr_postincrement(&r, sizeof(char));
  }
}

// EOF
