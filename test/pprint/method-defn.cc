// method-defn.cc
// Some cases of method definitions.

struct Other {};
struct Other2 {};

struct S {
  int x;
  int y;

  void method1();
  Other method2(Other2);
  static Other method3(Other2);
};

// Non-method that returns class type.
struct S func()
{
  struct S s;
  return s;
}

// Method that returns nothing.
void S::method1()
{}

// Method that both accepts and returns class types.
Other S::method2(Other2)
{
  struct Other o;
  return o;
}

// Static method that accepts and returns class types.
/*static*/ Other S::method3(Other2)
{
  struct Other o;
  return o;
}

// EOF
