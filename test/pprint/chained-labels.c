// chained-labels.c
// This file is generated by the gen-chained-labels.py script.

// Its purpose is to exercise all combinations of two successive
// label-like elements, both with and without a trailing semicolon.
// Label-like elements are default, case, GNU range case, and actual
// goto labels.

// Repeated 'default' is not valid C/C++, but Elsa currently accepts
// it, so this script generates it.


void f_0_0(int x)
{
  switch (x) {
    default:
    default:
      ;
  }
}

void f_0_1(int x)
{
  switch (x) {
    default:
    case 3:
      ;
  }
}

void f_0_2(int x)
{
  switch (x) {
    default:
    case 3 ... 4:
      ;
  }
}

void f_0_3(int x)
{
  switch (x) {
    default:
    label3:
      ;
  }
}

void f_1_0(int x)
{
  switch (x) {
    case 1:
    default:
      ;
  }
}

void f_1_1(int x)
{
  switch (x) {
    case 1:
    case 3:
      ;
  }
}

void f_1_2(int x)
{
  switch (x) {
    case 1:
    case 3 ... 4:
      ;
  }
}

void f_1_3(int x)
{
  switch (x) {
    case 1:
    label3:
      ;
  }
}

void f_2_0(int x)
{
  switch (x) {
    case 1 ... 2:
    default:
      ;
  }
}

void f_2_1(int x)
{
  switch (x) {
    case 1 ... 2:
    case 3:
      ;
  }
}

void f_2_2(int x)
{
  switch (x) {
    case 1 ... 2:
    case 3 ... 4:
      ;
  }
}

void f_2_3(int x)
{
  switch (x) {
    case 1 ... 2:
    label3:
      ;
  }
}

void f_3_0(int x)
{
  switch (x) {
    label1:
    default:
      ;
  }
}

void f_3_1(int x)
{
  switch (x) {
    label1:
    case 3:
      ;
  }
}

void f_3_2(int x)
{
  switch (x) {
    label1:
    case 3 ... 4:
      ;
  }
}

void f_3_3(int x)
{
  switch (x) {
    label1:
    label3:
      ;
  }
}
