// offsetof-array-index.c
// __builtin_offsetof applied to an array index expression.

struct S {
  int arr[4];
};

int f(struct S *s)
{
  char *p = (char*)s;

  // Calculate the offset of a particular array element.
  p += __builtin_offsetof(struct S, arr[2]);

  int *q = (int*)p;

  // Return arr[3] + arr[4];
  return q[0] + q[1];
}

int main()
{
  struct S s = { { 1,2,3,4 } };

  return
    f(&s) == 7 &&
    1? 0 : 1;
}

// EOF
