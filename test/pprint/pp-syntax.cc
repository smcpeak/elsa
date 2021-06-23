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

// This exercises all the combinations of the presence of 'else' and
// whether each side is brace-enclosed, although it seems that the
// latter is always normalized to use braces.
void test_if(int a, int b, int c)
{
  if (a)
    b=1;

  if (a)
    b=1;
  else
    c=1;

  if (a) {
    b=1;
  }

  if (a) {
    b=1;
  }
  else {
    c=1;
  }

  if (a)
    b=1;
  else {
    c=1;
  }

  if (a) {
    b=1;
  }
  else
    c=1;

  if (int x = a) {
    b=1;
  }
}

void test_while(int a)
{
  while (a) {
    a--;
  }

  while (int i = a++) {
    i--;
  }
}

void test_do_while(int a)
{
  do {
    a--;
  } while (a);
}

void test_for(int a, int b)
{
  for (a=0; a<10; a++) {
    b++;
  }

  for (int i=0; i<10; i++) {
    b++;
  }
}

// EOF
