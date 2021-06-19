// conv-void-ptr.c
// C allows converting void* to any*.

int *x;

void f(void *p)
{
  x = p;
}

// EOF
