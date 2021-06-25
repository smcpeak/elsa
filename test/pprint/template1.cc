// template1.cc
// Based on t0026.cc.

template <class T>
int f(T t)
{
  T(z);    // ambiguous, but I can disambiguate!

  int q = T(z);    // ctor call
  return q;
}

template <class T>
void g(T t, int);

int main()
{
  f(9);

  f<int>(8);

  return 0;
}

// EOF
