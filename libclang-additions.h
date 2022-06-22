// libclang-additions.h
// Functions that libclang should have but doesn't.

#ifndef ELSA_LIBCLANG_ADDITIONS_H
#define ELSA_LIBCLANG_ADDITIONS_H

#include "clang-c/Index.h"             // CXCursor

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// Possible operators for CXCursor_UnaryOperator.
enum CXUnaryOperator {
  // Columns: \S+ = @40://
  CXUnaryOperator_Invalid   =  0,      // Not a unary expression, or unrecognized operator.
  CXUnaryOperator_PostInc   =  1,      // postfix ++
  CXUnaryOperator_PostDec   =  2,      // postfix --
  CXUnaryOperator_PreInc    =  3,      // prefix ++
  CXUnaryOperator_PreDec    =  4,      // prefix --
  CXUnaryOperator_AddrOf    =  5,      // &
  CXUnaryOperator_Deref     =  6,      // *
  CXUnaryOperator_Plus      =  7,      // +
  CXUnaryOperator_Minus     =  8,      // -
  CXUnaryOperator_Not       =  9,      // ~
  CXUnaryOperator_LNot      = 10,      // !
  CXUnaryOperator_Real      = 11,      // __real
  CXUnaryOperator_Imag      = 12,      // __imag
  CXUnaryOperator_Extension = 13,      // __extension__
  CXUnaryOperator_Coawait   = 14,      // co_await
};


// For a CXCursor_UnaryOperator, return which operator it uses.
enum CXUnaryOperator clang_unaryOperator_operator(CXCursor cursor);


// Possible operators for CXCursor_BinaryOperator.
enum CXBinaryOperator {
  // Columns: \S+ = @40://
  CXBinaryOperator_Invalid   =  0,     // Not a binary expression, or unrecognized operator.
  CXBinaryOperator_PtrMemD   =  1,     // .*
  CXBinaryOperator_PtrMemI   =  2,     // ->*
  CXBinaryOperator_Mul       =  3,     // *
  CXBinaryOperator_Div       =  4,     // /
  CXBinaryOperator_Rem       =  5,     // %
  CXBinaryOperator_Add       =  6,     // +
  CXBinaryOperator_Sub       =  7,     // -
  CXBinaryOperator_Shl       =  8,     // <<
  CXBinaryOperator_Shr       =  9,     // >>
  CXBinaryOperator_Cmp       = 10,     // <=>
  CXBinaryOperator_LT        = 11,     // <
  CXBinaryOperator_GT        = 12,     // >
  CXBinaryOperator_LE        = 13,     // <=
  CXBinaryOperator_GE        = 14,     // >=
  CXBinaryOperator_EQ        = 15,     // ==
  CXBinaryOperator_NE        = 16,     // !=
  CXBinaryOperator_And       = 17,     // &
  CXBinaryOperator_Xor       = 18,     // ^
  CXBinaryOperator_Or        = 19,     // |
  CXBinaryOperator_LAnd      = 20,     // &&
  CXBinaryOperator_LOr       = 21,     // ||
  CXBinaryOperator_Assign    = 22,     // =
  CXBinaryOperator_MulAssign = 23,     // *=
  CXBinaryOperator_DivAssign = 24,     // /=
  CXBinaryOperator_RemAssign = 25,     // %=
  CXBinaryOperator_AddAssign = 26,     // +=
  CXBinaryOperator_SubAssign = 27,     // -=
  CXBinaryOperator_ShlAssign = 28,     // <<=
  CXBinaryOperator_ShrAssign = 29,     // >>=
  CXBinaryOperator_AndAssign = 30,     // &=
  CXBinaryOperator_XorAssign = 31,     // ^=
  CXBinaryOperator_OrAssign  = 32,     // |=
  CXBinaryOperator_Comma     = 33,     // ,
};


// For a CXCursor_BinaryOperator, return the operator.
enum CXBinaryOperator clang_binaryOperator_operator(CXCursor cursor);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ELSA_LIBCLANG_ADDITIONS_H
