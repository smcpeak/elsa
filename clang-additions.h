// clang-additions.h
// Functions that libclang should have but doesn't.

#ifndef ELSA_CLANG_ADDITIONS_H
#define ELSA_CLANG_ADDITIONS_H

#include "clang-c/Index.h"             // CXCursor
#include <stddef.h>                    // size_t

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// --------------------------- UnaryOperator ---------------------------
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
//
// Rationale for addition: This information is not exposed in the
// existing API.  One can sometimes get the operator by scanning the
// tokens, but that fails for an operator that is the result of the
// expansion of a macro, for example:
//
//   #define PLUS +
//   int x = PLUS 2;         // First token is "PLUS".
//
enum CXUnaryOperator clang_unaryOperator_operator(CXCursor cursor);


// -------------------------- BinaryOperator ---------------------------
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
//
// Rationale for addition: This information is not exposed in the
// existing API.
enum CXBinaryOperator clang_binaryOperator_operator(CXCursor cursor);


// ------------------------------ ForStmt ------------------------------
// Selector for 'clang_forStmtElement'.
enum CXForStmtElement {
  CXForStmtElement_init = 0,
  CXForStmtElement_cond = 1,
  CXForStmtElement_inc  = 2,
  CXForStmtElement_body = 3,
};


// Get the specified element of a 'for' loop.  Returns
// 'clang_getNullCursor()' for a missing element.
//
// Rationale for addition: The existing 'clang_visitChildren' API is
// insufficient because you can't tell which is which if one or more are
// missing.
CXCursor clang_forStmtElement(CXCursor forStmt, CXForStmtElement element);


// -------------------------- CharacterLiteral -------------------------
// Selector for 'clang_characterLiteralElement'.
enum CXCharacterLiteralElement {
  // The "kind" of string literal, as a CXCharacterLiteralKind.
  CXCharacterLiteralElement_kind = 0,

  // The numeric value of the literal.
  CXCharacterLiteralElement_value = 1,
};


// Possible values for 'CXCharacterLiteralElement_kind' and
// 'CXStringLiteralElement_kind'.  I do not know what they all mean, as
// they are not documented in the clang sources.
enum CXCharacterLiteralKind {
  // This might be used if the C++ API gains a new king of string
  // literal and this interface has not been updated.
  CXCharacterLiteralKind_Unknown = 0,

  CXCharacterLiteralKind_Ascii   = 1,
  CXCharacterLiteralKind_Wide    = 2,
  CXCharacterLiteralKind_UTF8    = 3,
  CXCharacterLiteralKind_UTF16   = 4,
  CXCharacterLiteralKind_UTF32   = 5,
};


// Get the specified element of a CXCursor_CharacterLiteral.
//
// Rationale for addition: The existing API does not allow
// distinguishing the kind of literal in C (since they all have type
// 'int'), and using the evaluator to get the numeric value is
// inelegant.
unsigned clang_characterLiteralElement(CXCursor characterLiteralCursor,
  CXCharacterLiteralElement element);


// --------------------------- StringLiteral ---------------------------
// Selector for 'clang_stringLiteralElement'.
enum CXStringLiteralElement {
  // Length in characters.
  CXStringLiteralElement_length = 0,

  // Bytes per character.
  CXStringLiteralElement_charByteWidth = 1,

  // The "kind" of string literal, as a CXCharacterLiteralKind.
  CXStringLiteralElement_kind = 2,

  // True if this "isPascal()", which isn't document in clang.
  CXStringLiteralElement_isPascal = 3,

  // Number of adjacent string literal tokens that were concatenated to
  // form this string literal.
  CXStringLiteralElement_numConcatenated = 4,
};


// Get the specified element of a CXCursor_StringLiteral.
//
// Rationale for addition: The existing API exposes some of this through
// 'clang_getCursorSpelling', but that requires additional parsing, and
// is incomplete.
unsigned clang_stringLiteralElement(CXCursor stringLiteralCursor,
  CXStringLiteralElement element);


// Get the contents of a string literal.  'contents' must point to an
// array of 'contentsSize' bytes, and 'contentsSize' must be at least
// 'length * charByteWidth'.
//
// Rationale for addition: The existing API exposes this through
// 'clang_getCursorSpelling', but that requires additional parsing.
void clang_getStringLiteralBytes(CXCursor stringLiteralCursor,
  unsigned char *contents, size_t contentsSize);


// ----------------------------- Attribute -----------------------------
// The flavor of attribute syntax used by a given attribute instance.
enum CXAttributeSyntaxKind {
  // Used for an invalid call, or an unknown syntax.
  CXAttributeSyntaxKind_Unknown = 0,

  // __attribute__((...))
  CXAttributeSyntaxKind_GNU = 1,

  // [[...]]
  CXAttributeSyntaxKind_CXX11 = 2,

  // [[...]]
  CXAttributeSyntaxKind_C2x = 3,

  // __declspec(...)
  CXAttributeSyntaxKind_Declspec = 4,

  // [uuid("...")] class Foo
  CXAttributeSyntaxKind_Microsoft = 5,

  // __ptr16, alignas(...), etc.
  CXAttributeSyntaxKind_Keyword = 6,

  // #pragma ...
  CXAttributeSyntaxKind_Pragma = 7,

  // Context-sensitive version of a keyword attribute.
  CXAttributeSyntaxKind_ContextSensitiveKeyword = 8,
};


// Get the syntax kind of the given attribute.
//
// Returns CXAttributeSyntaxKind_Unknown if this is not an attribute.
CXAttributeSyntaxKind clang_getAttributeSyntaxKind(CXCursor attributeCursor);


// Get the name of the given attribute.
//
// Returns an empty string if this is not an attribute.
CXString clang_getAttributeName(CXCursor attributeCursor);


// Return the "printPretty" string for this attribute, which contains
// the leading syntax element along with its name and arguments, for
// example: ' __attribute__((alias("var_target")))'.
//
// This typically (always?) has a leading space for all syntaxes except
// Microsoft.
//
// Returns an empty string if this is not an attribute.
CXString clang_getAttributePrettyString(CXCursor attributeCursor);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ELSA_CLANG_ADDITIONS_H
