// character-literals.c
// Some character literals.

typedef int wchar_t;

void f()
{
  'a';
  L'a';

  '\'';
  '\\';

  '\0';
  '\1';

  '\x7E';
  '\x7F';
  '\x80';
  '\x81';

  '\xFE';
  '\xFF';

  L'\xFF';
  L'\xFFFF';
  L'\xFFFFFFFF';

  'ab';
  'abc';
  'abcd';
  'abcde';

  '\xFF_';
  '\xFF\x5F';
}


// EOF
