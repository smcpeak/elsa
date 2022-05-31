// p27-ex-2d-array-one-column.c
// Initialize just the first column of a 2D array.

int z[4][3] = {
  { 1 }, { 2 }, { 3 }, { 4 }
};

int main()
{
  return
    z[0][0] == 1 &&
    z[0][1] == 0 &&
    z[0][2] == 0 &&
    z[1][0] == 2 &&
    z[1][1] == 0 &&
    z[1][2] == 0 &&
    z[2][0] == 3 &&
    z[2][1] == 0 &&
    z[2][2] == 0 &&
    z[3][0] == 4 &&
    z[3][1] == 0 &&
    z[3][2] == 0 &&

    1? 0 : 1;
}

// EOF
