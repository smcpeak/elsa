// long-decl.c
// Declaration statement that is long.
// I want to break the line after the '='.

int AV_ORD_PTR_CHECK_DEREF(int, int);
int av_ord_ptr_add(int, int);
int AV_ORD_PTR_REGISTER_LOCAL(int);

int f(int arr, int dummy)
{
  {
    int __retval_longer_name = AV_ORD_PTR_CHECK_DEREF(
                                 av_ord_ptr_add(
                                   AV_ORD_PTR_REGISTER_LOCAL(arr),
                                   0 * sizeof(int)), dummy) +
                                 AV_ORD_PTR_CHECK_DEREF(
                                   av_ord_ptr_add(
                                     AV_ORD_PTR_REGISTER_LOCAL(arr),
                                     1 * sizeof(int)), dummy);

    // Compound declaration.
    int __retval_longer_namf = AV_ORD_PTR_CHECK_DEREF(
                                 av_ord_ptr_add(
                                   AV_ORD_PTR_REGISTER_LOCAL(arr),
                                   0 * sizeof(int)), dummy) +
                                 AV_ORD_PTR_CHECK_DEREF(
                                   av_ord_ptr_add(
                                     AV_ORD_PTR_REGISTER_LOCAL(arr),
                                     1 * sizeof(int)), dummy),
        __retval_longer_namg = AV_ORD_PTR_CHECK_DEREF(
                                 av_ord_ptr_add(
                                   AV_ORD_PTR_REGISTER_LOCAL(arr),
                                   0 * sizeof(int)), dummy) +
                                 AV_ORD_PTR_CHECK_DEREF(
                                   av_ord_ptr_add(
                                     AV_ORD_PTR_REGISTER_LOCAL(arr),
                                     1 * sizeof(int)), dummy);

    return __retval_longer_name;
  }
}

// EOF
