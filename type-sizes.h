// type-sizes.h
// TypeSizes class.

#ifndef ELSA_TYPE_SIZES_H
#define ELSA_TYPE_SIZES_H

#include "type-sizes-fwd.h"            // forward for this module

// smbase
#include "str.h"                       // string


// Configurable map from scalar type to size and alignment.
class TypeSizes {
public:      // types
  // Each element of this enumeration represents a set of scalar
  // types that all have the same size and alignment.
  //
  // Per C11 6.2.5, paragraph number:
  //
  // * 21: Scalar types are arithmetic types and pointer types.
  // * 18: Arithmetic types are integer and floating types.
  // * 17: Integer types are char, un/signed integer, and enumerated.
  // * 11: Floating types are real floating and complex types.
  // * 10: Real floating types are float, double, and long double.
  //
  // Per paragraph 28, pointer types need not all have the same size
  // and alignment, but I am assuming here that they do.
  //
  // C++14 3.9/9 adds pointer to member types to the "scalar types".
  //
  enum ScalarTypeSet {
    STS_EMPTY,               // Empty set.

    STS_BOOL,                // C++ bool, C _Bool
    STS_CHAR,                // char, signed char, unsigned char
    STS_SHORT,               // short, unsigned short
    STS_INT,                 // int, unsigned int
    STS_LONG,                // long, unsigned long
    STS_LONG_LONG,           // long long, unsigned long long
    STS_FLOAT,               // float
    STS_DOUBLE,              // double
    STS_LONG_DOUBLE,         // long double
    STS_FLOAT_COMPLEX,       // C float _Complex
    STS_DOUBLE_COMPLEX,      // C double _Complex
    STS_LONG_DOUBLE_COMPLEX, // C long double _Complex
    STS_POINTER,             // T*
    STS_POINTER_TO_MEMBER,   // T C::*

    NUM_SCALAR_TYPE_SETS
  };

public:      // data
  // Size for each set.
  int m_stsSize[NUM_SCALAR_TYPE_SETS];

public:      // class methods
  // Returns "BOOL", etc.
  static char const *stsName(ScalarTypeSet sts);

public:      // methods
  TypeSizes() { set_host_compiler(); }

  // Use the sizes that the compiler used to compie Elsa uses.
  void set_host_compiler();

  // Configure to emulate Linux/x86_64.
  void set_linux_x86_64();

  // Return a string where each line specifies one of the sizes.  This
  // is meant for debugging purposes.
  string toString() const;
};


#endif // ELSA_TYPE_SIZES_H
