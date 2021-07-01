void g(char const *p);

void f()
{
  // In C, string literals have type 'char[]' even though modifying
  // them has undefined behavior.

  char const *p = "init";
  char *q = "init2";
  p = "assign";
  g("arg");
}
