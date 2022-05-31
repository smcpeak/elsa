// p38-ex-any-union-member.c
// Any member of a union can be initialized.

union {
  char some_member;
  int any_member;
} u = {
  .any_member = 42
};

int main()
{
  return
    u.any_member == 42 &&
    1? 0 : 1;
}

// EOF
