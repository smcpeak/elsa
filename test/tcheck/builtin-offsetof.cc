// builtin-offsetof.cc
// __builtin_offsetof

struct S {
  int x;
  int y;
};

int f()
{
  // Non-existent field.
  //ERROR(1): __builtin_offsetof(struct S, z);

  // Not a struct.
  //ERROR(2): __builtin_offsetof(int, x);

  return __builtin_offsetof(struct S, x);
}

int g()
{
  return __builtin_offsetof(struct S, y);
}

class Base {
public:
  int x;
  int y;
};

class Derived : public Base {
public:
  int y;
  int z;
};

int some_global;

void testDerived()
{
  __builtin_offsetof(Base, x);
  __builtin_offsetof(Derived, x);
  __builtin_offsetof(Derived, y);
  __builtin_offsetof(Derived, Base::y);
  __builtin_offsetof(Derived, z);

  // Field does not exist.
  //ERROR(3): __builtin_offsetof(Derived, Base::z);
  //ERROR(4): __builtin_offsetof(Base, z);
  //ERROR(5): __builtin_offsetof(Derived, w);

  // This *is* accepted, even though it should not be.
  __builtin_offsetof(Derived, ::some_global);
}

// EOF
