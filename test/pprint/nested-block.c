// nested-block.c
// Compound statement in compound statement.

void f(int x)
{
  x++;
  {
    x++;
  }
}

// EOF
