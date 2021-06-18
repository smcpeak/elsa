// pqname1.c
// Testing makePQName in C.

int x;
int f(int);

typedef struct GlobalStruct_struct {
  int x;
  int y;
} GlobalStruct;

GlobalStruct gs1;
struct GlobalStruct_struct gs2;

int g(int a, int b, int c)
{
  int d, e;

  typedef struct LocalStruct_struct {
    int h;
    int i;
  } LocalStruct;

  LocalStruct ls1;
  struct LocalStruct_struct ls2;

  return 2;
}

// EOF
