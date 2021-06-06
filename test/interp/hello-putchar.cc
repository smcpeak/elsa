// hello-putchar.cc
// Hello world using 'putchar'.

// Normally declared in stdio.h, but I do not have any headers yet.
int putchar(int ch);

int main()
{
  putchar(0x48);  // 'H'
  putchar(0x0A);  // '\n'
  return 0;
}

// EOF
