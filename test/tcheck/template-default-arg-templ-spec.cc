// template-default-arg-templ-spec.cc
// A template that has a default template argument that is, itself,
// a template specialization.

template < typename _Alloc >
class allocator;

typedef int Integer;

// Elsa will crash on the second '>' if I prevent multi-yielding
// TypeSpecifier semantic values.  This is due in part to a conflict
// with this rule in kandr.gr:
//
//   TFDeclaration -> m1:UberModifierSeqOpt /*implicit-int*/ list:InitDeclaratorList ";"
//
// That rule is what causes the GLR algorithm to first engage on the
// 'template' keyword, but there may be other rules interacting to
// causes the sval to be reused.
//
template < typename _Alloc = allocator < Integer > >
class basic_string;

// EOF
