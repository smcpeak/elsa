// constant-exprs.c
// C11 6.7.9/4: "All the expressions in an initializer for an object
// that has static or thread storage duration shall be constant
// expressions or string literals."

int one()
{
  return 1;
}


int arr2a[2] = { 1+2, 3+4 };

//ERROR(non-const-global-init): int arr2b[2] = { one()+2, 3+4 };
//NOTWORKING(elsa): Rule not enforced.

static int test_arr2a()
{
  return
    arr2a[0] == 3 &&
    arr2a[1] == 7 &&
    1;
}


static int test_local_arr2a()
{
  int arr2a[2] = { 1+2, 3+4 };

  // Non-constant init allowed here.
  int arr2b[2] = { one()+2, 3+4 };

  return
    arr2a[0] == 3 &&
    arr2a[1] == 7 &&
    arr2b[0] == 3 &&
    arr2b[1] == 7 &&
    1;
}


static int test_static_local_arr2a()
{
  static int arr2a[2] = { 1+2, 3+4 };

  //ERROR(non-const-static-init): static int arr2b[2] = { one()+2, 3+4 };
  //NOTWORKING(elsa): Rule not enforced.

  return
    arr2a[0] == 3 &&
    arr2a[1] == 7 &&
    1;
}


int main()
{
  if (1 &&
    test_arr2a() &&
    test_local_arr2a() &&
    test_static_local_arr2a() &&
  1) {
    return 0;
  }
  else {
    return 1;
  }
}


// EOF
