// t0581.cc
// a simpler (than t0435.cc) template array bounds example

template <int ArraySize>
int *end(int(&array)[ArraySize])
{
  int dummy[2];
  end(dummy);
}

// EOF
