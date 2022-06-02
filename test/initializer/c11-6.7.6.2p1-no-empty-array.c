// c11-6.7.6.2p1-no-empty-array.c
// C11 6.7.6.2p1: Zero-length arrays are not allowed.

//ERROR(not-allowed): int arr0[0];

// Completely empty TU is not allowed.  (C11 6.9p1 grammar.)  And anyway
// I need a 'main'.
int main()
{
  return 0;
}

// EOF
