// range-designators.c
// GNU range designator extension.
// https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html

// NOTE: Elsa currently produces the wrong output for these because it
// drops the lower bound of every range, treating it instead as if only
// the upper bound was present.


// From the manual.
int widths[] = { [0 ... 9] = 1, [10 ... 99] = 2, [100] = 3 };

static int test_widths()
{
  _Static_assert(sizeof(widths) / sizeof(widths[0]) == 101, "");

  for (int i=0; i < 101; i++) {
    int exp = (i <= 9)? 1 : (i <= 99)? 2 : 3;
    if (!( widths[i] == exp )) {
      return 0;
    }
  }

  return 1;
}


int overlap[] = { [3 ... 9] = 2, [0 ... 6] = 1 };

static int test_overlap()
{
  _Static_assert(sizeof(overlap) / sizeof(overlap[0]) == 10, "");

  for (int i=0; i < 10; i++) {
    int exp = (i <= 6)? 1 : 2;
    if (!( overlap[i] == exp )) {
      return 0;
    }
  }

  return 1;
}


int gap[] = { [6 ... 9] = 2, [0 ... 3] = 1 };

static int test_gap()
{
  _Static_assert(sizeof(gap) / sizeof(gap[0]) == 10, "");

  for (int i=0; i < 10; i++) {
    int exp = (i <= 3)? 1 : (i < 6)? 0 : 2;
    if (!( gap[i] == exp )) {
      return 0;
    }
  }

  return 1;
}


// The semantics of nested range designators is not specified in the
// manual, although it appears reasonably straightforward.
int nest[2][3] = { [0 ... 1][0 ... 2] = 9 };

static int test_nest()
{
  for (int i=0; i < 2; i++) {
    for (int j=0; j < 3; j++) {
      int exp = 9;
      if (!( nest[i][j] == exp )) {
        return 0;
      }
    }
  }

  return 1;
}


// The next thing to initialize after the use of any nested arrangement
// of range designators is the same as if all range designators were
// single designators of their larger value.
int nest_next[3][4] = { [0 ... 1][0 ... 2] = 9, 10, 11 };

static int test_nest_next()
{
  for (int i=0; i < 3; i++) {
    for (int j=0; j < 4; j++) {
      int exp = (i <= 1 && j <= 2)?  9 :
                (i == 1 && j == 3)? 10 :
                (i == 2 && j == 0)? 11 :
                                     0 ;
      if (!( nest_next[i][j] == exp )) {
        return 0;
      }
    }
  }

  return 1;
}


struct S {
  int a[4];
};

struct S s_nest[3] = { [0 ... 1].a[1 ... 2] = 9, 10, 11 };

static int test_s_next()
{
  for (int i=0; i < 3; i++) {
    for (int j=0; j < 4; j++) {
      int exp = (i <= 1 && 1 <= j && j <= 2)?  9 :
                (i == 1 && j == 3)          ? 10 :
                (i == 2 && j == 0)          ? 11 :
                                               0 ;
      if (!( s_nest[i].a[j] == exp )) {
        return 0;
      }
    }
  }

  return 1;
}


int main()
{
  return
    test_widths() &&
    test_overlap() &&
    test_gap() &&
    test_nest() &&
    test_nest_next() &&
    test_s_next() &&
    1? 0 : 1;
}


// EOF
