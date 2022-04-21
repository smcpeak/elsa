// cc.in74
// need global operator new defined

void *foo()
{
  return operator new(3);
}

void *foo2()
{
  return ::operator new(4);
}
