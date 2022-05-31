// c11-6.7.2.1p1-no-empty-struct.c
// C11 6.7.2.1p1: Empty structs are not allowed.

//ERROR(not-allowed): struct Empty {};
//NOTWORKING(elsa): Rule not enforced.

int main()
{
  return 0;
}

// EOF
