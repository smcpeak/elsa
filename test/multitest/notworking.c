// notworking.c
// Test the NOTWORKING mechanism.

int main()
{
  //ERROR(1): return 1;

  //ERROR(2): return 0;
  //NOTWORKING(): Wrong return code.
  //NOTWORKING(sut1): Wrong return code.

  //ERROR(3): return 3;
  //NOTWORKING(sut1): Erroneous NOTWORKING declaration.

  return 0;
}

// EOF
