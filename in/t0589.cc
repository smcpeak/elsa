template<typename T, typename S, S n>
struct foo{
  foo<T,S,n>& operator= (const foo<T,S,n>* in) {
    return *this;
  }
  foo<T,S,n>() {
  }
};

struct bar{
  foo<int,int,64 > arr;
  bar() {
    arr = new foo<int,int,64 >();
  }
};
