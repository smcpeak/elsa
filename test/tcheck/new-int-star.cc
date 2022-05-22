// new-int-star.cc
// Expression "new int *".

void f()
{
  // This is interesting because, naively, when the parser sees the '*',
  // it thinks it might be a multiplication operator since we are in an
  // expression context.
  new int * ;
}
