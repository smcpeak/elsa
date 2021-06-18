// make-typedef.c
// Exercise makeASTTypeId with a typedef type.

typedef int myint;
typedef int myint2;

void f()
{
  __elsa_checkMakeASTTypeId((myint)0, "myint");
  __elsa_checkMakeASTTypeId((myint2)0, "myint2");
}

// EOF
