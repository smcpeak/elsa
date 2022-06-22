// try-catch.cc
// Simple example of a try-catch block.

void g(int a)
{
  try {
    a++;
  }
  catch (long b) {
    a--;
  }
  catch (float b) {
    a--;
    a--;
  }
  catch (...) {
    a = 3;
  }

  a++;
}

// EOF
