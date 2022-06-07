// cast-btw-float-and-ptr.c
// Test C11 6.5.4p4 prohibition on casts between pointers and floats.

void *vp;
int *ip;

float f;
double d;
long double ld;
_Complex float cf;
_Complex double cd;
_Complex long double cld;

typedef unsigned long uintptr_t;

uintptr_t i;

void func()
{
  // Can cast either to/from int.
  (uintptr_t)vp;
  (uintptr_t)ip;
  (uintptr_t)f;
  (uintptr_t)d;
  (uintptr_t)ld;
  (uintptr_t)cf;
  (uintptr_t)cd;
  (uintptr_t)cld;

  (void*)i;
  (int*)i;
  (float)i;
  (double)i;
  (long double)i;
  (_Complex float)i;
  (_Complex double)i;
  (_Complex long double)i;

  // Cannot cast pointer to float.
  //ERROR(p2f1): (float)vp;
  //ERROR(p2f2): (float)ip;
  //ERROR(p2f3): (double)vp;
  //ERROR(p2f4): (long double)vp;
  //ERROR(p2f5): (_Complex float)ip;
  //ERROR(p2f6): (_Complex double)vp;
  //ERROR(p2f7): (_Complex long double)vp;

  // Cannot cast float to pointer.
  //ERROR(f2p1): (void*)f;
  //ERROR(f2p2): (int*)f;
  //ERROR(f2p3): (void*)d;
  //ERROR(f2p4): (void*)ld;
  //ERROR(f2p5): (int*)cf;
  //ERROR(f2p6): (void*)cd;
  //ERROR(f2p7): (void*)cld;
}

// EOF
