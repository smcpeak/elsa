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
    int m_host;
    int m_linux64;
  };
}


#define NAME(name) TypeSizes::STS_##name, #name

static SizeData const g_sizeData[] = {
  // Name                      Host                          Linux64
  { NAME(EMPTY),               0,                                  0, },
  { NAME(BOOL),                sizeof(bool),                       1, },
  { NAME(CHAR),                sizeof(char),                       1, },
  { NAME(SHORT),               sizeof(short),                      2, },
  { NAME(INT),                 sizeof(int),                        4, },
  { NAME(LONG),                sizeof(long),                       8, },
  { NAME(LONG_LONG),           sizeof(long long),                  8, },
  { NAME(FLOAT),               sizeof(float),                      4, },
  { NAME(DOUBLE),              sizeof(double),                     8, },
  { NAME(LONG_DOUBLE),         sizeof(long double),               16, },
  { NAME(FLOAT_COMPLEX),       sizeof(float _Complex),             8, },
  { NAME(DOUBLE_COMPLEX),      sizeof(double _Complex),           16, },
  { NAME(LONG_DOUBLE_COMPLEX), sizeof(long double _Complex),      32, },
  { NAME(POINTER),             sizeof(void*),                      8, },
  { NAME(POINTER_TO_MEMBER),   sizeof(int SizeData::*),            8, },
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


void TypeSizes::set_host_compiler()
{
  ASSERT_TABLESIZE(g_sizeData, NUM_SCALAR_TYPE_SETS);
  for (int i=0; i < NUM_SCALAR_TYPE_SETS; i++) {
    xassert(g_sizeData[i].m_sts == i);
    m_stsSize[i] = g_sizeData[i].m_host;
  }
}


void TypeSizes::set_linux_x86_64()
{
  for (int i=0; i < NUM_SCALAR_TYPE_SETS; i++) {
    xassert(g_sizeData[i].m_sts == i);
    m_stsSize[i] = g_sizeData[i].m_linux64;
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
