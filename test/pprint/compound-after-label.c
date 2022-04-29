// compound-after-label.c
// Test having a label followed by a compound statement.

static int f()
{
label:
  {
    return 3;
  }
}

static int f2()
{
  {
  label:
    {
      return 3;
    }
  label4:
  label44:
    {
      return 4;
    }
  }
}

static int f3(int x)
{
  switch (x) {
    case 1:
      {
        return 3;
      }
    case 2:
    case 3:
      {
        return 23;
      }
    default:
      {
        return 4;
      }
  }
}

// EOF
