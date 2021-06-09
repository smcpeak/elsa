// expr-type.c
// Types of some expressions in C.

void f()
{
  __elsa_checkType(1, 1);

  // Fails because "!1" is incorrectly called 'bool' in C.
  //__elsa_checkType(1, !1);
}

// EOF
