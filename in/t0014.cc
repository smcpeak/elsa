// cc.in14
// a few of the obscure expression kinds

// this would normally come from the <typeinfo> header
namespace std {
  class type_info {
  public:
    char const *name() const;
  };
}

typedef char y;

class C {};

int main()
{
  int x, *p;
  int (*ptr_to_array_of_5_ints)[5];
  C *c, *d;

  // E_constructor
  x = int(6);

  // E_new
  p = new int;

  // E_new of an array with non-const size
  p = new int[x];

  // E_new of an array of an array; this allocates
  // an array of objects, where each object has type
  // "int[5]", and 'x' objects are allocated
  ptr_to_array_of_5_ints = new int[x][5];

  // E_delete
  delete p;

  // E_keywordCast
  c = const_cast<C*>(d);
  c = dynamic_cast<C*>(d);
  c = static_cast<C*>(d);
  c = reinterpret_cast<C*>(d);

  // E_typeidExpr
  typeid(x);

  // E_typeidType
  typeid(y);
}
