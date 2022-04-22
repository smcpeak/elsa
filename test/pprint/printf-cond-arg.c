// printf-cond-arg.c
// printf with a condition in an argument expression.

int printf(char const *format, ...);

int main(int argc, char **argv)
{
  printf("argc==2: %s\n", (argc==2 ? "yes" : "no"));
  return 0;
}
