// forward.cc
// class forward decls
// Based on in/t0018.cc.

class Foo;

int main()
{
  Foo *x;
}

void f(Foo *) {}

class Foo {
public:
  int x;
};

void f(Foo *, int) {}

// EOF
