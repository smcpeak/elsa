typedef void F();
struct S {
  const F f;   // OK: equivalent to: void f();
};
