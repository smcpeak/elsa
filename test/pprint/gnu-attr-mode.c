// gnu-attr-mode.c
// Testing GNU "mode" attribute.

// This works in GCC, showing that '__attribute__' can go before
// 'typedef' and be recognized properly.
__attribute__((mode(byte))) typedef int my_byte;

int main()
{
  return sizeof(my_byte) == 1? 0 : 1;
}

// EOF
