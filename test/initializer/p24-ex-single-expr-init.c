// p24-ex-single-expr-init.c
// Demonstrates initializing a scalar and a struct with an expression.

// The original relied on <complex.h>, but I want this to be self-
// contained, so I'm using stand-ins.

typedef struct Complex {
  double re;
  double im;
} Complex;

Complex make_complex(double r, double i)
{
  Complex ret = { r, i };
  return ret;
}

int main()
{
  int i = 3.5;
  Complex c = make_complex(5, 3);

  return
    i == 3 &&
    c.re == 5 &&
    c.im == 3 &&
    1? 0 : 1;
}

// EOF
