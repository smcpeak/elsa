// p33-ex-enum-array-desig.c
// Using enumerators as array index designators.

enum { member_one, member_two };
const char *nm[] = {
  [member_two] = "member two",
  [member_one] = "member one",
};

int main()
{
  return
    nm[0][0] == 'm' &&
    nm[0][7] == 'o' &&
    nm[1][0] == 'm' &&
    nm[1][7] == 't' &&
    1? 0 : 1;
}

// EOF
