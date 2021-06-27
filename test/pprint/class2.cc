// class2.cc
// Testing a few more things.

// 2021-06-27: Every class has to have an explicit 'public:' because
// there is a pprint bug that fails to print implicit members with the
// proper access control.

class Base1 {
public:
};

class Base2 {
public:
};

class Derived1 : public Base1 {
public:
  Derived1(Derived1 const &other)
    : Base1(other)
  {}
};

class Derived2 : public Base1, public Base2 {
public:
};

// EOF
