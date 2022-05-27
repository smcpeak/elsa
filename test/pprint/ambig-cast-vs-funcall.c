// ambig-cast-vs-funcall.c
// Illustrate an ambiguity with "(a)(b).c".

// The interesting bit is how the parsing precedence is affected by
// whether 'a' is the name of a type or the name of a variable.

// I probably already have this tested elsewhere, but having recently
// encountered it again, I made another test, so choose to keep it.

typedef struct S {
  int c;
} S;

int f1(S b, int choice)
{
  typedef int a;

  if (choice) {
    // Cast on top of field access.
    return (a)(b).c;
  }
  else {
    // Like this.
    return (a)((b).c);
  }
}

int f2(S (*a)(int), int b, int choice)
{
  if (choice) {
    // Field access on top of a function call.
    return (a)(b).c;
  }
  else {
    // Like this.
    return ((a)(b)).c;
  }
}

// EOF
