// libclang-additions.cc
// Code for libclang-additions.h.

#include "libclang-additions.h"                  // this module

#include "clang/AST/Expr.h"                      // UnaryOperator, etc.


using clang::BinaryOperator;
using clang::Expr;
using clang::Stmt;
using clang::UnaryOperator;
using llvm::dyn_cast;
using llvm::dyn_cast_or_null;


// Copied from clang/tools/libclang/CXCursor.cpp.
static const Stmt *getCursorStmt(CXCursor Cursor) {
  if (Cursor.kind == CXCursor_ObjCSuperClassRef ||
      Cursor.kind == CXCursor_ObjCProtocolRef ||
      Cursor.kind == CXCursor_ObjCClassRef)
    return nullptr;

  return static_cast<const Stmt *>(Cursor.data[1]);
}


extern "C"
enum CXUnaryOperator clang_unaryOperator_operator(CXCursor cursor)
{
  Stmt const *stmt = getCursorStmt(cursor);

  if (UnaryOperator const *unExpr = dyn_cast_or_null<UnaryOperator>(stmt)) {
    switch (unExpr->getOpcode()) {
      #define UNY_CASE(Name) \
        case clang::UO_##Name: return CXUnaryOperator_##Name;
      UNY_CASE(PostInc)
      UNY_CASE(PostDec)
      UNY_CASE(PreInc)
      UNY_CASE(PreDec)
      UNY_CASE(AddrOf)
      UNY_CASE(Deref)
      UNY_CASE(Plus)
      UNY_CASE(Minus)
      UNY_CASE(Not)
      UNY_CASE(LNot)
      UNY_CASE(Real)
      UNY_CASE(Imag)
      UNY_CASE(Extension)
      UNY_CASE(Coawait)
      #undef UNY_CASE

      default:
        break;
    }
  }

  return CXUnaryOperator_Invalid;
}


extern "C"
enum CXBinaryOperator clang_binaryOperator_operator(CXCursor cursor)
{
  Stmt const *stmt = getCursorStmt(cursor);

  if (BinaryOperator const *binExpr = dyn_cast_or_null<BinaryOperator>(stmt)) {
    switch (binExpr->getOpcode()) {
      #define BIN_CASE(Name) \
        case clang::BO_##Name: return CXBinaryOperator_##Name;
      BIN_CASE(PtrMemD)
      BIN_CASE(PtrMemI)
      BIN_CASE(Mul)
      BIN_CASE(Div)
      BIN_CASE(Rem)
      BIN_CASE(Add)
      BIN_CASE(Sub)
      BIN_CASE(Shl)
      BIN_CASE(Shr)
      BIN_CASE(Cmp)
      BIN_CASE(LT)
      BIN_CASE(GT)
      BIN_CASE(LE)
      BIN_CASE(GE)
      BIN_CASE(EQ)
      BIN_CASE(NE)
      BIN_CASE(And)
      BIN_CASE(Xor)
      BIN_CASE(Or)
      BIN_CASE(LAnd)
      BIN_CASE(LOr)
      BIN_CASE(Assign)
      BIN_CASE(MulAssign)
      BIN_CASE(DivAssign)
      BIN_CASE(RemAssign)
      BIN_CASE(AddAssign)
      BIN_CASE(SubAssign)
      BIN_CASE(ShlAssign)
      BIN_CASE(ShrAssign)
      BIN_CASE(AndAssign)
      BIN_CASE(XorAssign)
      BIN_CASE(OrAssign)
      BIN_CASE(Comma)
      #undef BIN_CASE

      default:
        break;
    }
  }

  return CXBinaryOperator_Invalid;
}


// EOF
