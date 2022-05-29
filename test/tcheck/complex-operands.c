// complex-operands.c
// Testing various combinations of operands involving complex types.

void f()
{
  double dr;
  double _Complex dc;

  __elsa_checkType(dr + dr, (double)0);
  __elsa_checkType(dr + dc, (double _Complex)0);

  __elsa_checkType(dc + dr, (double _Complex)0);
  __elsa_checkType(dc + dc, (double _Complex)0);

  __elsa_checkType(dr * dr, (double)0);
  __elsa_checkType(dr * dc, (double _Complex)0);

  __elsa_checkType(dc * dr, (double _Complex)0);
  __elsa_checkType(dc * dc, (double _Complex)0);

  int i;

  __elsa_checkType(dr == i, (int)0);
  __elsa_checkType(dc == i, (int)0);
  __elsa_checkType(i == dr, (int)0);
  __elsa_checkType(i == dc, (int)0);

  __elsa_checkType(dc + i, (double _Complex)0);
  __elsa_checkType(i + dc, (double _Complex)0);
}

// EOF
