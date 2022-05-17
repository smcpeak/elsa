// gnu-alias.c
// Test type-checking for GNU __alias__ attribute.

int __f()
{
  return 6;
}

int f() __attribute__ ((weak, alias ("__f")));

// Use string juxtaposition to name the target.
int f_concat() __attribute__ ((weak, alias ("__" "f")));

// Alias names something that does not exist.
//ERROR(1): int fx() __attribute__ ((weak, alias ("__fx")));

int another()
{
  return 7;
}

int another_alias() __attribute__((__alias__("another")));

void __elsa_checkIsGNUAlias(int dummy, ...);

void checks()
{
  __elsa_checkIsGNUAlias(0, another_alias, another);
  __elsa_checkIsGNUAlias(0, f, __f);

  // Attempt to assert mismatched alias.
  //ERROR(2): __elsa_checkIsGNUAlias(0, another_alias, __f);
  //ERROR(3): __elsa_checkIsGNUAlias(0, f, another);

  // Not a GNU alias at all.
  //ERROR(4): __elsa_checkIsGNUAlias(0, another, another);
}

// EOF
