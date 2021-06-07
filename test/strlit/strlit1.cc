// strlit1.cc
// Exercise string literal decoding.

void f()
{
  "hello";
  "hello, " "world";
  "escapes1: \n\t\v\b\r\f\a\\\?\'\"\123\x45";
  "impldef: \q";
  L"wide string";
}

// EOF
