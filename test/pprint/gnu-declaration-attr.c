// gnu-declaration-attr.c
// GNU attribute on a declaration (not declarator).

int var_target = 7;

// Attribute after the type specifier, before the declarator.
extern int __attribute__((alias("var_target"))) var_alias;


int target2 = 9;

// Attribute after storage class specifier, before the type specifier.
extern __attribute__((alias("target2"))) int alias2;


int target3 = 29;

// Attribute before storage class specifier.
__attribute__((alias("target3"))) extern int alias3;


int main()
{
  int x = var_alias;

  return
    x == 7 &&
    alias2 == 9 &&
    alias3 == 29 &&
    1? 0 : 1;
}

// EOF
