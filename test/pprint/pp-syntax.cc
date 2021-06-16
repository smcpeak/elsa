// pp-syntax.cc
// Exercise how various kinds of syntax are pretty-printed.

int f()
{
  return 1;
}

void g()
{}

int hasTry(int x)
try {
  return 2+x;
}
catch (int z) {
  return ++z;
}

class C {
public:      // data
  int m_x;
  int m_y;

public:      // methods
  C(int x);
};

C::C(int x)
try
  : m_x(x),
    m_y(0)
{
  x++;
}
catch (int y) {
  y++;
  // Implicitly re-throws.
}

int h(C &c, int x)
{
  return c.m_x + x;
}

// EOF
