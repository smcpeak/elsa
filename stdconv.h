// stdconv.h                       see license.txt for copyright and terms of use
// Standard Conversions, i.e. Section ("clause") 4 of C++98

// A Standard Conversion is one of a dozen or so type coercions
// described in section 4 of C++98.  A Standard Conversion Sequence
// is up to three Standard Conversions, drawn from particular
// subsets of such conversions, to be applied in sequence.

// Standard Conversions are the kinds of conversions that can be
// done implicitly (no explicit conversion or cast syntax), short
// of user-defined constructors and conversions.

// Note that in my system, there is nothing directly called an
// "lvalue", rather there are simply references and non-references.

#ifndef STDCONV_H
#define STDCONV_H

// elsa
#include "cc-env-fwd.h"                // Env
#include "cc-lang-fwd.h"               // CCLang
#include "cc-type-fwd.h"               // AtomicType, Type, TypeFactory
#include "cc-flags.h"                  // SpecialExpr

// smbase
#include "sm-macros.h"                 // ENUM_BITWISE_AND,OR
#include "str.h"                       // string


// The kinds of Standard Conversions.  Any given pair of convertible
// types will be related by the conversions permitted as one or more
// of the following kinds.  ORing together zero or one conversion from
// each group yields a Standard Conversion Sequence.
//
// 2022-06-25: In the past I had worked to ensure this fit into 8 bits,
// but that was not necessary, and it is convenient to express a wider
// range of conversions, so the size restriction is now gone.
//
enum StandardConversion {
  SC_IDENTITY        = 0x000,  // types are identical

  // conversion group 1 (comes first)
  SC_LVAL_TO_RVAL    = 0x001,  // 4.1: int& -> int
  SC_ARRAY_TO_PTR    = 0x002,  // 4.2: char[] -> char*
  SC_FUNC_TO_PTR     = 0x003,  // 4.3: int ()(int) -> int (*)(int)
  SC_GROUP_1_MASK    = 0x003,

  // conversion group 3 (comes last conceptually)
  SC_QUAL_CONV       = 0x004,  // 4.4: int* -> int const*
  SC_GROUP_3_MASK    = 0x004,

  // conversion group 2 (goes in the middle)
  SC_INT_PROM        = 0x010,  // 4.5: int... -> int..., no info loss possible
  SC_FLOAT_PROM      = 0x020,  // 4.6: float -> double, no info loss possible
  SC_INT_CONV        = 0x030,  // 4.7: int... -> int..., info loss possible
  SC_FLOAT_CONV      = 0x040,  // 4.8: float... -> float..., info loss possible
  SC_FLOAT_INT_CONV  = 0x050,  // 4.9: int... <-> float..., info loss possible
  SC_PTR_CONV        = 0x060,  // 4.10: 0 -> Foo*, Child* -> Parent*
  SC_PTR_MEMB_CONV   = 0x070,  // 4.11: int Child::* -> int Parent::*
  SC_BOOL_CONV       = 0x080,  // 4.12: various types <-> bool
  SC_DERIVED_TO_BASE = 0x090,  // 13.3.3.1p6: Child -> Parent
  SC_CHAR_PTR_CONV   = 0x0A0,  // C: char* -> unsigned char*, etc.
  SC_GROUP_2_MASK    = 0x0F0,

  // The rest are ad-hoc conversions that do not fit into the above
  // scheme, and are not usually combined with them.

  // This is for a conversion to void, which is allowed as an extension
  // by GCC in ?: expressions, and might be needed in other cases too.
  SC_ADH_TO_VOID     = 0x100,

  // According to Clang, it is sometimes possible to implicitly convert
  // a pointer to an integer and vice-versa!
  SC_ADH_PTR_TO_INT  = 0x200,
  SC_ADH_INT_TO_PTR  = 0x300,

  SC_ADH_MASK        = 0xF00,

  SC_ERROR          = 0x1000,  // cannot convert
};

// for '&', one of the arguments should always be a mask
ENUM_BITWISE_AND(StandardConversion)

// for '|', some care should be taken to ensure you're not
// ORing together nonzero elements from the same group
ENUM_BITWISE_OR(StandardConversion)

// allow creation of masks for turning off bits
ENUM_BITWISE_NOT(StandardConversion, SC_ERROR);

// render in C++ syntax as bitwise OR of the constants above
string toString(StandardConversion c);


// remove SC_LVAL_TO_RVAL from a conversion sequence
StandardConversion removeLval(StandardConversion scs);

// true if 'left' is a subsequence of 'right'
bool isSubsequenceOf(StandardConversion left, StandardConversion right);


// standard conversion rank, as classified by C++98 Table 9
// (section 13.3.3.1.1 para 3)
enum SCRank {
  SCR_EXACT,
  SCR_PROMOTION,
  SCR_CONVERSION,
  NUM_SCRANKS
};

SCRank getRank(StandardConversion scs);


// given two types, determine the Standard Conversion Sequence,
// if any, that will convert 'src' into 'dest'
StandardConversion getStandardConversion(
  string *errorMsg,    // if non-null, failed conversion sets error message
  CCLang const &lang,  // language rules
  SpecialExpr special, // properties of the source expression
  Type const *src,     // source type
  Type const *dest,    // destination type

  // when the dest type is a method receiver ('this') parameter,
  // it's allowable to bind an rvalue to a non-const reference
  // (13.3.1 para 5 bullet 3)
  bool destIsReceiver = false
);


// reverse-engineer a previous conversion that involved a
// polymorphic destination type
Type *getConcreteDestType(TypeFactory &tfac, Type *srcType,
                          StandardConversion sconv,
                          SimpleTypeId destPolyType);

// C++98 section 5, para 9
Type *usualArithmeticConversions(TypeFactory &tfac, Type *left, Type *right);
SimpleTypeId usualArithmeticConversions(SimpleTypeId leftId, SimpleTypeId rightId);

// C++98 4.5
bool isIntegerPromotion(AtomicType const *src, AtomicType const *dest);
SimpleTypeId applyIntegralPromotions(Type *t);
SimpleTypeId applyIntegralPromotions(SimpleTypeId id);


// testing interface, for use by the type checker
void test_getStandardConversion(
  Env &env, SpecialExpr special, Type const *src, Type const *dest,
  int expected     // expected return value
);


// reference-relatedness (8.5.3 para 4)

// "is t1 reference-related to t2?"  (both types have already had their
// references stripped)  NOTE: this relation is *not* symmetric!
bool isReferenceRelatedTo(Type *t1, Type *t2);

// determine which of three reference-compatilibity conditions exist:
//   0: not compatible
//   1: compatible with added qualification
//   2: compatible exactly
int referenceCompatibility(Type *t1, Type *t2);

// "is t1 reference-compatible (possibly with added qual) with t2?"
inline bool isReferenceCompatibleWith(Type *t1, Type *t2)
  { return referenceCompatibility(t1, t2) != 0; }


#endif // STDCONV_H
