// p34-ex-field-desig.c
// Initializing using field designators to not depend on field order.

typedef struct div_t {
  int rem;
  int quot;
} div_t;

div_t answer = { .quot = 2, .rem = -1 };

int main()
{
  return
    answer.rem == -1 &&
    answer.quot == 2 &&
    1? 0 : 1;
}

// EOF
