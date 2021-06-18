// pqname2.cc
// Testing makePQName in C++.

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

namespace NS1 {
  int ns1_memb;
}

namespace {
  int anon_ns_memb;
}

template <class T>
T tf(T t)
{
  return t;
}

int call_tf()
{
  return tf<int>(3);
}

template <>
char tf(char)
{
  return 'x';
}

template <class T>
class C {
public:
  T t;

  template <class U>
  class D {
  public:
    U u;
  };
};

C<int> c;
C<int>::D<int> d;

// EOF
