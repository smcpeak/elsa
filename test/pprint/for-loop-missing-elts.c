// for-loop-missing-elts.c
// 'for' loop with various missing elements.

void f(int a)
{
  for (a=0; a < 10; a++) {
  }
  for (; a < 10; a++) {
  }
  for (a=0; ; a++) {
  }
  for (a=0; a < 10; ) {
  }
  for (; ; a++) {
  }
  for (; a < 10; ) {
  }
  for (; ; a++) {
  }
  for (;;) {
  }
}

// EOF
