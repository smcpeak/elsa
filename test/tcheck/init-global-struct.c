struct S {
  int x;
  char arr[10];
  char *p;
} s = {
  //ERROR(2): "some string"+
  4,
  "0123456789",
  "any length works here"
  //ERROR(1): ,6
};
