// sizeof.c
// Simple tests of 'sizeof'.

int i1;

typedef int myint;

myint i2;

typedef struct S_tag {
  int x;
} S_td;

S_td s;

unsigned f()
{
  unsigned exprs =
    sizeof(i1) +
    sizeof(i2) +
    sizeof(s) +
    0;

  unsigned types =

  // These do not work because of libclang deficiencies.
  #if 0
    sizeof(int) +
    sizeof(myint) +
    sizeof(struct S_tag) +
    sizeof(S_td) +
  #endif

    0;

  return exprs + types;
}

unsigned g()
{
  // In the libclang API, it is not possible to distinguish the
  // following two cases!  This is the straw that broke the libclang
  // camel's back, leading me to abandon it in favor of the C++ API.
  return sizeof 3 +
         sizeof(int [3]);
}

// EOF
