// multitest/simple.c
// A simple demonstration of multitest.pl.

int main()
{
  //ERROR(1): return 1;

  //ERROR(2): return
  //ERROR(2):   2;

  //ERROR(named-error): return 4;

  int ret = 3;
  ret = 0;     //ERRORIFMISSING(3): set to 0

  return ret;
}

// EOF
