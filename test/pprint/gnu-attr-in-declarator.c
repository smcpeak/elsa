// gnu-attr-in-declarator.c
// Uses of GNU '__attribute__' associated with a specific declarator
// rather than the declaration as a whole.

int x __attribute__((unused)),
    y __attribute__((mode(byte)));

int * __attribute__((may_alias)) * __attribute__((may_alias)) ma2;

// These grouping parens are redundant per the standard C++ grammar, but
// play a role in the GNU grammar due to binding the attribute to the
// declarator rather than declaration.  When there is only one
// declarator, that has no semantic consequence, but Elsa will print the
// parens in order to retain the association anyway.
int (__attribute__((unused)) in_paren);

// Here, the attribute is only associated with the first declarator, but
// if we omit the grouping parens then it will be associated with both.
int (__attribute__((unused)) in_paren1), (in_paren2);

// For a second or later declarator, parens are not needed for
// disambiguation.
int (in_paren1b), (__attribute__((unused)) in_paren2b);

int (in_paren1c), (__attribute__((unused)) in_paren2c), in_paren3c;


int f()
{
  int x = 0;
  x += (int)3;
  x += (__attribute__((mode(byte))) int)4;
  x += (int __attribute__((mode(byte))))5;
  return x;
}

int *g(int *p)
{
  p = ( int *                            )3;
  p = ( int * __attribute__((may_alias)) )4;

  p = ( int (*                           ) )5;
  p = ( int (*__attribute__((may_alias)) ) )6;

  return p;
}

int (*fp1)(int);
int (__attribute__((unused)) *fp2)(int);

// Attributes on bitfields.
struct S1 {
  int a : 1;
  int b : 1 __attribute__((packed));
  int c : 1 __attribute__((packed)) __attribute__((deprecated));
};

// EOF
