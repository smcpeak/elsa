// gnu-attr-several.c
// Several combinations of GNU attribute placements.

int var_target = 7;

extern int __attribute__((alias("var_target"))) declaration_attr;
extern int __attribute__((alias("var_target"))) da1, da2;
extern int declarator_attr __attribute__((alias("var_target"))), y;
extern int x, declarator_attr2 __attribute__((alias("var_target")));

extern int __attribute__(()) empty_attr;

extern __attribute__((format(printf, 1, 2))) int myprintf(char const *format, ...);

extern  __attribute__((alias("var_target"),weak)) int alias_and_weak;

// EOF
