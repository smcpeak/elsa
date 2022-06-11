// cc.in32
// Anonymous unions: C++14 9.5p5.

int main()
{
  union {
    unsigned ret;
    char c[4];
  };

  c[3] = 4;
  return ret;
}


int foo(int x)
{
  if (x) {
    // better not still be able to see 'ret' ..
    //ERROR(1): return ret;     // undeclared
  }
  return 0;
}


struct nsStr {
  union {
    char*         mStr;
    short*        mUStr;
  };
};

short *someFunc(struct nsStr s)
{
  return s.mUStr;
}


char *anotherFunc(int x)
{
  if (x) {
    //ERROR(2): return mStr;
  }
  return 0;
}


