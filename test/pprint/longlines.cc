// longlines.cc
// Test with long lines.

int something(char const *fmt, ...);

void f(int a)
{
  something("long format string with %d some format specifiers %d",
            a, a, a, a, a, a, a, a, a, a, a, a);
}

void g(int a)
{
  if (a) {
    a++;
  }
  else {
    a--;
  }
}

int arr[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

int next;

int arr2[][3] = {
  { 1, 2, 3 },
  { 1, 2, 3 },
  { 1, 2, 3 },
  { 1, 2, 3 },
  { 1, 2, 3 },
  { 1, 2, 3 },
};

// EOF