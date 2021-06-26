// type-sizes.cc
// Code for type-sizes.h.

#include "type-sizes.h"                // this module

// smbase
#include "sm-macros.h"                 // ASSERT_TABLESIZE
#include "xassert.h"                   // xassert

#ifdef __STDC_IEC_559_COMPLEX__
#  include <complex.h>                 // _Complex
#else
   // This would not be difficult to deal with, but I will wait until
   // I run into it.
#  error "No support for _Complex."
#endif

namespace {
  // An entry for one STS in 'g_sizeData'.
  struct SizeData {
    TypeSizes::ScalarTypeSet m_sts;
    char const *m_name;

    // This is the size according to the compiler used to compile Elsa.
    int m_build;

    // This is the size I experimentally found for linux on x86_64 by
    // asking GCC-9.3.0.
    int m_linux64;

    // Windows/x86_64 as reported by MinGW-W64 GCC-5.4.0.  It does not
    // support _Complex, so I just doubled the values for the
    // non-complex floats on the assumption that is what the sizes would
    // be if implemented.
    int m_win64;

    // Windows/x86.  The values below are guesses because I don't have
    // a compiler targeting this platform at the moment.
    int m_win32;
  };
}


#define NAME(name) TypeSizes::STS_##name, #name

static SizeData const g_sizeData[] = {
  // Name                      Host                         L64 W64 W32
  { NAME(EMPTY),               0,                             0,  0,  0, },
  { NAME(BOOL),                sizeof(bool),                  1,  1,  1, },
  { NAME(CHAR),                sizeof(char),                  1,  1,  1, },
  { NAME(WCHAR),               sizeof(wchar_t),               4,  2,  2, },
  { NAME(SHORT),               sizeof(short),                 2,  2,  2, },
  { NAME(INT),                 sizeof(int),                   4,  4,  4, },
  { NAME(LONG),                sizeof(long),                  8,  4,  4, },
  { NAME(LONG_LONG),           sizeof(long long),             8,  8,  8, },
  { NAME(FLOAT),               sizeof(float),                 4,  4,  4, },
  { NAME(DOUBLE),              sizeof(double),                8,  8,  8, },
  { NAME(LONG_DOUBLE),         sizeof(long double),          16, 16, 16, },
  { NAME(FLOAT_COMPLEX),       sizeof(float _Complex),        8,  8,  8, },
  { NAME(DOUBLE_COMPLEX),      sizeof(double _Complex),      16, 16, 16, },
  { NAME(LONG_DOUBLE_COMPLEX), sizeof(long double _Complex), 32, 32, 32, },
  { NAME(POINTER),             sizeof(void*),                 8,  8,  4, },
  { NAME(POINTER_TO_MEMBER),   sizeof(int SizeData::*),       8,  8,  4, },
};

#undef NAME


/*static*/ char const *TypeSizes::stsName(ScalarTypeSet sts)
{
  if ((unsigned)sts < NUM_SCALAR_TYPE_SETS) {
    return g_sizeData[sts].m_name;
  }
  else {
    return "bad ScalarTypeSet";
  }
}


int TypeSizes::getSize(ScalarTypeSet sts) const
{
  xassert((unsigned)sts < NUM_SCALAR_TYPE_SETS);
  return m_stsSize[sts];
}


void TypeSizes::set_build_compiler()
{
  ASSERT_TABLESIZE(g_sizeData, NUM_SCALAR_TYPE_SETS);
  for (int i=0; i < NUM_SCALAR_TYPE_SETS; i++) {
    xassert(g_sizeData[i].m_sts == i);
    m_stsSize[i] = g_sizeData[i].m_build;
  }
}


void TypeSizes::set_linux_x86_64()
{
  for (int i=0; i < NUM_SCALAR_TYPE_SETS; i++) {
    xassert(g_sizeData[i].m_sts == i);
    m_stsSize[i] = g_sizeData[i].m_linux64;
  }
}


void TypeSizes::set_windows_x86_64()
{
  for (int i=0; i < NUM_SCALAR_TYPE_SETS; i++) {
    xassert(g_sizeData[i].m_sts == i);
    m_stsSize[i] = g_sizeData[i].m_win64;
  }
}


void TypeSizes::set_windows_x86()
{
  for (int i=0; i < NUM_SCALAR_TYPE_SETS; i++) {
    xassert(g_sizeData[i].m_sts == i);
    m_stsSize[i] = g_sizeData[i].m_win32;
  }
}


string TypeSizes::toString() const
{
  stringBuilder sb;
  for (int i=0; i < NUM_SCALAR_TYPE_SETS; i++) {
    sb << g_sizeData[i].m_name << ": " << m_stsSize[i] << '\n';
  }
  return sb.str();
}


// EOF
