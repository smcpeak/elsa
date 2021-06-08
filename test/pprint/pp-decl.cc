// pp-decl.cc
// Pretty printing of declarations.

int global1;
int global2 = 2;
int globalArr[] = {1, 2, 3};
int g3=3, g4=4;

void f()
{
  int x=1, y=2;
  int (*fnptr)(int, char) = 0;
}

class C {
public:
  int m_x;
  void g(int)
  {
    void (C::*ptmFunc)(int) = &C::g;
    int C::*ptmData = &C::m_x;
  }
};


// EOF
