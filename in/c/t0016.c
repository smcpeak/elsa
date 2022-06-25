// t0016.c
// function prototype and definition with different parameter lists

int fallowc(int);

// When this is pretty-printed in --clang mode, it prints as
// "int fallowc(char c) { ... }", which is then not accepted as input.
// For now I'm just going to accept that and hope this does not arise
// in practice.
fallowc(c)
char c;        // "default argument promotions" make this int
{
  return c;
}
