// overloaded function resolution in an array

// originally found in package fltk

// error: failed to resolve address-of of overloaded function `foo' assigned
// to type `struct Anon_struct_1 []'

// error: failed to resolve address-of of overloaded function `foo' assigned
// to type `void (*[])(int /*anon*/, long int /*anon*/)'

// ERR-MATCH: resolve address-of of overloaded function

typedef void (*funcType1)(int);
typedef void (*funcType2)(int, long);

void foo(int x);

void foo(int x, long y);

funcType2 array1[] = {foo};

struct S {
    funcType1 f;
    //ERROR(1): int x;     // would assign 'foo' to 'int', not ok
    funcType2 g;
} array2[] = { {foo, foo} };

// these provoke errors because of the failure to resolve 'foo',
// rather than because of having too many initializers, because
// the latter is just a weak error right now
//ERROR(2): funcType array3[2] = {foo, foo, foo};
//ERROR(3): S s = {foo, foo, foo};

S array4[2] = {
  foo,foo,
  foo,foo,
};


