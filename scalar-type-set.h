// scalar-type-set.h
// ScalarTypeSet enumeration.

// This enumeration is logically a part of the TypeSizes module.
// However, I split it out into its own file to resolve a definition
// circularity: SimpleTypeId wants to map to ScalarTypeSet, while
// TypeSizes wants to map size_t and friends to SimpleTypeId.

#ifndef ELSA_SCALAR_TYPE_SET_H
#define ELSA_SCALAR_TYPE_SET_H

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
  STS_WCHAR,               // C++ wchar_t
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

// Returns "BOOL", etc.
//
// Defined in type-sizes.cc.
char const *toString(ScalarTypeSet sts);


#endif // ELSA_SCALAR_TYPE_SET_H
