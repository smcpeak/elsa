// clang-additions.cc
// Code for clang-additions.h.

#include "clang-additions.h"                     // this module

#include "clang/AST/Expr.h"                      // UnaryOperator, etc.

#include <assert.h>                              // assert
#include <string.h>                              // memcpy


// I have chosen to disable the Objective-C stuff in order to limit how
// much I have to copy from CXCursor.cpp.  Disabled ObjC code is guarded
// by "#if OBJC_SUPPORT".
#define OBJC_SUPPORT 0


using namespace clang;
using namespace llvm;


// ------------------ BEGIN: Copied from CXCursor.cpp ------------------
// The routines in this section were copied from
// clang/tools/libclang/CXCursor.cpp.  If this code were submitted to
// Clang, this section would be omitted.
static const Stmt *getCursorStmt(CXCursor Cursor) {
  if (Cursor.kind == CXCursor_ObjCSuperClassRef ||
      Cursor.kind == CXCursor_ObjCProtocolRef ||
      Cursor.kind == CXCursor_ObjCClassRef)
    return nullptr;

  return static_cast<const Stmt *>(Cursor.data[1]);
}


static CXTranslationUnit getCursorTU(CXCursor Cursor) {
  return static_cast<CXTranslationUnit>(const_cast<void*>(Cursor.data[2]));
}


static const Decl *getCursorDecl(CXCursor Cursor) {
  return static_cast<const Decl *>(Cursor.data[0]);
}


static CXCursor MakeCXCursor(const Stmt *S, const Decl *Parent,
                             CXTranslationUnit TU,
                             SourceRange RegionOfInterest = SourceRange()) {
  assert(S && TU && "Invalid arguments!");
  CXCursorKind K = CXCursor_NotImplemented;

  switch (S->getStmtClass()) {
  default:
    // This 'default' was not in the original code, but was implied.
    break;

  case Stmt::NoStmtClass:
    break;

  case Stmt::CaseStmtClass:
    K = CXCursor_CaseStmt;
    break;

  case Stmt::DefaultStmtClass:
    K = CXCursor_DefaultStmt;
    break;

  case Stmt::IfStmtClass:
    K = CXCursor_IfStmt;
    break;

  case Stmt::SwitchStmtClass:
    K = CXCursor_SwitchStmt;
    break;

  case Stmt::WhileStmtClass:
    K = CXCursor_WhileStmt;
    break;

  case Stmt::DoStmtClass:
    K = CXCursor_DoStmt;
    break;

  case Stmt::ForStmtClass:
    K = CXCursor_ForStmt;
    break;

  case Stmt::GotoStmtClass:
    K = CXCursor_GotoStmt;
    break;

  case Stmt::IndirectGotoStmtClass:
    K = CXCursor_IndirectGotoStmt;
    break;

  case Stmt::ContinueStmtClass:
    K = CXCursor_ContinueStmt;
    break;

  case Stmt::BreakStmtClass:
    K = CXCursor_BreakStmt;
    break;

  case Stmt::ReturnStmtClass:
    K = CXCursor_ReturnStmt;
    break;

  case Stmt::GCCAsmStmtClass:
    K = CXCursor_GCCAsmStmt;
    break;

  case Stmt::MSAsmStmtClass:
    K = CXCursor_MSAsmStmt;
    break;

  case Stmt::ObjCAtTryStmtClass:
    K = CXCursor_ObjCAtTryStmt;
    break;

  case Stmt::ObjCAtCatchStmtClass:
    K = CXCursor_ObjCAtCatchStmt;
    break;

  case Stmt::ObjCAtFinallyStmtClass:
    K = CXCursor_ObjCAtFinallyStmt;
    break;

  case Stmt::ObjCAtThrowStmtClass:
    K = CXCursor_ObjCAtThrowStmt;
    break;

  case Stmt::ObjCAtSynchronizedStmtClass:
    K = CXCursor_ObjCAtSynchronizedStmt;
    break;

  case Stmt::ObjCAutoreleasePoolStmtClass:
    K = CXCursor_ObjCAutoreleasePoolStmt;
    break;

  case Stmt::ObjCForCollectionStmtClass:
    K = CXCursor_ObjCForCollectionStmt;
    break;

  case Stmt::CXXCatchStmtClass:
    K = CXCursor_CXXCatchStmt;
    break;

  case Stmt::CXXTryStmtClass:
    K = CXCursor_CXXTryStmt;
    break;

  case Stmt::CXXForRangeStmtClass:
    K = CXCursor_CXXForRangeStmt;
    break;

  case Stmt::SEHTryStmtClass:
    K = CXCursor_SEHTryStmt;
    break;

  case Stmt::SEHExceptStmtClass:
    K = CXCursor_SEHExceptStmt;
    break;

  case Stmt::SEHFinallyStmtClass:
    K = CXCursor_SEHFinallyStmt;
    break;

  case Stmt::SEHLeaveStmtClass:
    K = CXCursor_SEHLeaveStmt;
    break;

  case Stmt::CoroutineBodyStmtClass:
  case Stmt::CoreturnStmtClass:
    K = CXCursor_UnexposedStmt;
    break;

  case Stmt::ArrayTypeTraitExprClass:
  case Stmt::AsTypeExprClass:
  case Stmt::AtomicExprClass:
  case Stmt::BinaryConditionalOperatorClass:
  case Stmt::TypeTraitExprClass:
  case Stmt::CoawaitExprClass:
  case Stmt::DependentCoawaitExprClass:
  case Stmt::CoyieldExprClass:
  case Stmt::CXXBindTemporaryExprClass:
  case Stmt::CXXDefaultArgExprClass:
  case Stmt::CXXDefaultInitExprClass:
  case Stmt::CXXFoldExprClass:
  case Stmt::CXXStdInitializerListExprClass:
  case Stmt::CXXScalarValueInitExprClass:
  case Stmt::CXXUuidofExprClass:
  case Stmt::ChooseExprClass:
  case Stmt::DesignatedInitExprClass:
  case Stmt::DesignatedInitUpdateExprClass:
  case Stmt::ArrayInitLoopExprClass:
  case Stmt::ArrayInitIndexExprClass:
  case Stmt::ExprWithCleanupsClass:
  case Stmt::ExpressionTraitExprClass:
  case Stmt::ExtVectorElementExprClass:
  case Stmt::ImplicitCastExprClass:
  case Stmt::ImplicitValueInitExprClass:
  case Stmt::NoInitExprClass:
  case Stmt::MaterializeTemporaryExprClass:
  case Stmt::ObjCIndirectCopyRestoreExprClass:
  case Stmt::OffsetOfExprClass:
  case Stmt::ParenListExprClass:
  case Stmt::PredefinedExprClass:
  case Stmt::ShuffleVectorExprClass:
  case Stmt::ConvertVectorExprClass:
  case Stmt::VAArgExprClass:
  case Stmt::ObjCArrayLiteralClass:
  case Stmt::ObjCDictionaryLiteralClass:
  case Stmt::ObjCBoxedExprClass:
  case Stmt::ObjCSubscriptRefExprClass:
    K = CXCursor_UnexposedExpr;
    break;

  case Stmt::OpaqueValueExprClass:
    if (Expr *Src = cast<OpaqueValueExpr>(S)->getSourceExpr())
      return MakeCXCursor(Src, Parent, TU, RegionOfInterest);
    K = CXCursor_UnexposedExpr;
    break;

  case Stmt::PseudoObjectExprClass:
    return MakeCXCursor(cast<PseudoObjectExpr>(S)->getSyntacticForm(),
                        Parent, TU, RegionOfInterest);

  case Stmt::CompoundStmtClass:
    K = CXCursor_CompoundStmt;
    break;

  case Stmt::NullStmtClass:
    K = CXCursor_NullStmt;
    break;

  case Stmt::LabelStmtClass:
    K = CXCursor_LabelStmt;
    break;

  case Stmt::AttributedStmtClass:
    K = CXCursor_UnexposedStmt;
    break;

  case Stmt::DeclStmtClass:
    K = CXCursor_DeclStmt;
    break;

  case Stmt::CapturedStmtClass:
    K = CXCursor_UnexposedStmt;
    break;

  case Stmt::IntegerLiteralClass:
    K = CXCursor_IntegerLiteral;
    break;

  case Stmt::FixedPointLiteralClass:
    K = CXCursor_FixedPointLiteral;
    break;

  case Stmt::FloatingLiteralClass:
    K = CXCursor_FloatingLiteral;
    break;

  case Stmt::ImaginaryLiteralClass:
    K = CXCursor_ImaginaryLiteral;
    break;

  case Stmt::StringLiteralClass:
    K = CXCursor_StringLiteral;
    break;

  case Stmt::CharacterLiteralClass:
    K = CXCursor_CharacterLiteral;
    break;

  case Stmt::ConstantExprClass:
    return MakeCXCursor(cast<ConstantExpr>(S)->getSubExpr(),
                        Parent, TU, RegionOfInterest);

  case Stmt::ParenExprClass:
    K = CXCursor_ParenExpr;
    break;

  case Stmt::UnaryOperatorClass:
    K = CXCursor_UnaryOperator;
    break;

  case Stmt::UnaryExprOrTypeTraitExprClass:
  case Stmt::CXXNoexceptExprClass:
    K = CXCursor_UnaryExpr;
    break;

  case Stmt::MSPropertySubscriptExprClass:
  case Stmt::ArraySubscriptExprClass:
    K = CXCursor_ArraySubscriptExpr;
    break;

  case Stmt::OMPArraySectionExprClass:
    K = CXCursor_OMPArraySectionExpr;
    break;

  case Stmt::BinaryOperatorClass:
    K = CXCursor_BinaryOperator;
    break;

  case Stmt::CompoundAssignOperatorClass:
    K = CXCursor_CompoundAssignOperator;
    break;

  case Stmt::ConditionalOperatorClass:
    K = CXCursor_ConditionalOperator;
    break;

  case Stmt::CStyleCastExprClass:
    K = CXCursor_CStyleCastExpr;
    break;

  case Stmt::CompoundLiteralExprClass:
    K = CXCursor_CompoundLiteralExpr;
    break;

  case Stmt::InitListExprClass:
    K = CXCursor_InitListExpr;
    break;

  case Stmt::AddrLabelExprClass:
    K = CXCursor_AddrLabelExpr;
    break;

  case Stmt::StmtExprClass:
    K = CXCursor_StmtExpr;
    break;

  case Stmt::GenericSelectionExprClass:
    K = CXCursor_GenericSelectionExpr;
    break;

  case Stmt::GNUNullExprClass:
    K = CXCursor_GNUNullExpr;
    break;

  case Stmt::CXXStaticCastExprClass:
    K = CXCursor_CXXStaticCastExpr;
    break;

  case Stmt::CXXDynamicCastExprClass:
    K = CXCursor_CXXDynamicCastExpr;
    break;

  case Stmt::CXXReinterpretCastExprClass:
    K = CXCursor_CXXReinterpretCastExpr;
    break;

  case Stmt::CXXConstCastExprClass:
    K = CXCursor_CXXConstCastExpr;
    break;

  case Stmt::CXXFunctionalCastExprClass:
    K = CXCursor_CXXFunctionalCastExpr;
    break;

  case Stmt::CXXTypeidExprClass:
    K = CXCursor_CXXTypeidExpr;
    break;

  case Stmt::CXXBoolLiteralExprClass:
    K = CXCursor_CXXBoolLiteralExpr;
    break;

  case Stmt::CXXNullPtrLiteralExprClass:
    K = CXCursor_CXXNullPtrLiteralExpr;
    break;

  case Stmt::CXXThisExprClass:
    K = CXCursor_CXXThisExpr;
    break;

  case Stmt::CXXThrowExprClass:
    K = CXCursor_CXXThrowExpr;
    break;

  case Stmt::CXXNewExprClass:
    K = CXCursor_CXXNewExpr;
    break;

  case Stmt::CXXDeleteExprClass:
    K = CXCursor_CXXDeleteExpr;
    break;

  case Stmt::ObjCStringLiteralClass:
    K = CXCursor_ObjCStringLiteral;
    break;

  case Stmt::ObjCEncodeExprClass:
    K = CXCursor_ObjCEncodeExpr;
    break;

  case Stmt::ObjCSelectorExprClass:
    K = CXCursor_ObjCSelectorExpr;
    break;

  case Stmt::ObjCProtocolExprClass:
    K = CXCursor_ObjCProtocolExpr;
    break;

  case Stmt::ObjCBoolLiteralExprClass:
    K = CXCursor_ObjCBoolLiteralExpr;
    break;

  case Stmt::ObjCAvailabilityCheckExprClass:
    K = CXCursor_ObjCAvailabilityCheckExpr;
    break;

  case Stmt::ObjCBridgedCastExprClass:
    K = CXCursor_ObjCBridgedCastExpr;
    break;

  case Stmt::BlockExprClass:
    K = CXCursor_BlockExpr;
    break;

  case Stmt::PackExpansionExprClass:
    K = CXCursor_PackExpansionExpr;
    break;

  case Stmt::SizeOfPackExprClass:
    K = CXCursor_SizeOfPackExpr;
    break;

  case Stmt::DeclRefExprClass:
#if OBJC_SUPPORT
    if (const ImplicitParamDecl *IPD =
         dyn_cast_or_null<ImplicitParamDecl>(cast<DeclRefExpr>(S)->getDecl())) {
      if (const ObjCMethodDecl *MD =
            dyn_cast<ObjCMethodDecl>(IPD->getDeclContext())) {
        if (MD->getSelfDecl() == IPD) {
          K = CXCursor_ObjCSelfExpr;
          break;
        }
      }
    }
#endif // OBJC_SUPPORT

    K = CXCursor_DeclRefExpr;
    break;

  case Stmt::DependentScopeDeclRefExprClass:
  case Stmt::SubstNonTypeTemplateParmExprClass:
  case Stmt::SubstNonTypeTemplateParmPackExprClass:
  case Stmt::FunctionParmPackExprClass:
  case Stmt::UnresolvedLookupExprClass:
  case Stmt::TypoExprClass: // A typo could actually be a DeclRef or a MemberRef
    K = CXCursor_DeclRefExpr;
    break;

  case Stmt::CXXDependentScopeMemberExprClass:
  case Stmt::CXXPseudoDestructorExprClass:
  case Stmt::MemberExprClass:
  case Stmt::MSPropertyRefExprClass:
  case Stmt::ObjCIsaExprClass:
  case Stmt::ObjCIvarRefExprClass:
  case Stmt::ObjCPropertyRefExprClass:
  case Stmt::UnresolvedMemberExprClass:
    K = CXCursor_MemberRefExpr;
    break;

  case Stmt::CallExprClass:
  case Stmt::CXXOperatorCallExprClass:
  case Stmt::CXXMemberCallExprClass:
  case Stmt::CUDAKernelCallExprClass:
  case Stmt::CXXConstructExprClass:
  case Stmt::CXXInheritedCtorInitExprClass:
  case Stmt::CXXTemporaryObjectExprClass:
  case Stmt::CXXUnresolvedConstructExprClass:
  case Stmt::UserDefinedLiteralClass:
    K = CXCursor_CallExpr;
    break;

  case Stmt::LambdaExprClass:
    K = CXCursor_LambdaExpr;
    break;

#if OBJC_SUPPORT
  case Stmt::ObjCMessageExprClass: {
    K = CXCursor_ObjCMessageExpr;
    int SelectorIdIndex = -1;
    // Check if cursor points to a selector id.
    if (RegionOfInterest.isValid() &&
        RegionOfInterest.getBegin() == RegionOfInterest.getEnd()) {
      SmallVector<SourceLocation, 16> SelLocs;
      cast<ObjCMessageExpr>(S)->getSelectorLocs(SelLocs);
      SmallVectorImpl<SourceLocation>::iterator
        I=std::find(SelLocs.begin(), SelLocs.end(),RegionOfInterest.getBegin());
      if (I != SelLocs.end())
        SelectorIdIndex = I - SelLocs.begin();
    }
    CXCursor C = { K, 0, { Parent, S, TU } };
    return getSelectorIdentifierCursor(SelectorIdIndex, C);
  }
#endif // OBJC_SUPPORT

  case Stmt::MSDependentExistsStmtClass:
    K = CXCursor_UnexposedStmt;
    break;
  case Stmt::OMPParallelDirectiveClass:
    K = CXCursor_OMPParallelDirective;
    break;
  case Stmt::OMPSimdDirectiveClass:
    K = CXCursor_OMPSimdDirective;
    break;
  case Stmt::OMPForDirectiveClass:
    K = CXCursor_OMPForDirective;
    break;
  case Stmt::OMPForSimdDirectiveClass:
    K = CXCursor_OMPForSimdDirective;
    break;
  case Stmt::OMPSectionsDirectiveClass:
    K = CXCursor_OMPSectionsDirective;
    break;
  case Stmt::OMPSectionDirectiveClass:
    K = CXCursor_OMPSectionDirective;
    break;
  case Stmt::OMPSingleDirectiveClass:
    K = CXCursor_OMPSingleDirective;
    break;
  case Stmt::OMPMasterDirectiveClass:
    K = CXCursor_OMPMasterDirective;
    break;
  case Stmt::OMPCriticalDirectiveClass:
    K = CXCursor_OMPCriticalDirective;
    break;
  case Stmt::OMPParallelForDirectiveClass:
    K = CXCursor_OMPParallelForDirective;
    break;
  case Stmt::OMPParallelForSimdDirectiveClass:
    K = CXCursor_OMPParallelForSimdDirective;
    break;
  case Stmt::OMPParallelSectionsDirectiveClass:
    K = CXCursor_OMPParallelSectionsDirective;
    break;
  case Stmt::OMPTaskDirectiveClass:
    K = CXCursor_OMPTaskDirective;
    break;
  case Stmt::OMPTaskyieldDirectiveClass:
    K = CXCursor_OMPTaskyieldDirective;
    break;
  case Stmt::OMPBarrierDirectiveClass:
    K = CXCursor_OMPBarrierDirective;
    break;
  case Stmt::OMPTaskwaitDirectiveClass:
    K = CXCursor_OMPTaskwaitDirective;
    break;
  case Stmt::OMPTaskgroupDirectiveClass:
    K = CXCursor_OMPTaskgroupDirective;
    break;
  case Stmt::OMPFlushDirectiveClass:
    K = CXCursor_OMPFlushDirective;
    break;
  case Stmt::OMPOrderedDirectiveClass:
    K = CXCursor_OMPOrderedDirective;
    break;
  case Stmt::OMPAtomicDirectiveClass:
    K = CXCursor_OMPAtomicDirective;
    break;
  case Stmt::OMPTargetDirectiveClass:
    K = CXCursor_OMPTargetDirective;
    break;
  case Stmt::OMPTargetDataDirectiveClass:
    K = CXCursor_OMPTargetDataDirective;
    break;
  case Stmt::OMPTargetEnterDataDirectiveClass:
    K = CXCursor_OMPTargetEnterDataDirective;
    break;
  case Stmt::OMPTargetExitDataDirectiveClass:
    K = CXCursor_OMPTargetExitDataDirective;
    break;
  case Stmt::OMPTargetParallelDirectiveClass:
    K = CXCursor_OMPTargetParallelDirective;
    break;
  case Stmt::OMPTargetParallelForDirectiveClass:
    K = CXCursor_OMPTargetParallelForDirective;
    break;
  case Stmt::OMPTargetUpdateDirectiveClass:
    K = CXCursor_OMPTargetUpdateDirective;
    break;
  case Stmt::OMPTeamsDirectiveClass:
    K = CXCursor_OMPTeamsDirective;
    break;
  case Stmt::OMPCancellationPointDirectiveClass:
    K = CXCursor_OMPCancellationPointDirective;
    break;
  case Stmt::OMPCancelDirectiveClass:
    K = CXCursor_OMPCancelDirective;
    break;
  case Stmt::OMPTaskLoopDirectiveClass:
    K = CXCursor_OMPTaskLoopDirective;
    break;
  case Stmt::OMPTaskLoopSimdDirectiveClass:
    K = CXCursor_OMPTaskLoopSimdDirective;
    break;
  case Stmt::OMPDistributeDirectiveClass:
    K = CXCursor_OMPDistributeDirective;
    break;
  case Stmt::OMPDistributeParallelForDirectiveClass:
    K = CXCursor_OMPDistributeParallelForDirective;
    break;
  case Stmt::OMPDistributeParallelForSimdDirectiveClass:
    K = CXCursor_OMPDistributeParallelForSimdDirective;
    break;
  case Stmt::OMPDistributeSimdDirectiveClass:
    K = CXCursor_OMPDistributeSimdDirective;
    break;
  case Stmt::OMPTargetParallelForSimdDirectiveClass:
    K = CXCursor_OMPTargetParallelForSimdDirective;
    break;
  case Stmt::OMPTargetSimdDirectiveClass:
    K = CXCursor_OMPTargetSimdDirective;
    break;
  case Stmt::OMPTeamsDistributeDirectiveClass:
    K = CXCursor_OMPTeamsDistributeDirective;
    break;
  case Stmt::OMPTeamsDistributeSimdDirectiveClass:
    K = CXCursor_OMPTeamsDistributeSimdDirective;
    break;
  case Stmt::OMPTeamsDistributeParallelForSimdDirectiveClass:
    K = CXCursor_OMPTeamsDistributeParallelForSimdDirective;
    break;
  case Stmt::OMPTeamsDistributeParallelForDirectiveClass:
    K = CXCursor_OMPTeamsDistributeParallelForDirective;
    break;
  case Stmt::OMPTargetTeamsDirectiveClass:
    K = CXCursor_OMPTargetTeamsDirective;
    break;
  case Stmt::OMPTargetTeamsDistributeDirectiveClass:
    K = CXCursor_OMPTargetTeamsDistributeDirective;
    break;
  case Stmt::OMPTargetTeamsDistributeParallelForDirectiveClass:
    K = CXCursor_OMPTargetTeamsDistributeParallelForDirective;
    break;
  case Stmt::OMPTargetTeamsDistributeParallelForSimdDirectiveClass:
    K = CXCursor_OMPTargetTeamsDistributeParallelForSimdDirective;
    break;
  case Stmt::OMPTargetTeamsDistributeSimdDirectiveClass:
    K = CXCursor_OMPTargetTeamsDistributeSimdDirective;
    break;
  }

  CXCursor C = { K, 0, { Parent, S, TU } };
  return C;
}


// ------------------- END: Copied from CXCursor.cpp -------------------


// --------------------------- UnaryOperator ---------------------------
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


// -------------------------- BinaryOperator ---------------------------
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


// ------------------------------ ForStmt ------------------------------
// Make a cursor out of the arguments, unless 'S' is NULL, in which case
// return 'clang_getNullCursor()'.
static CXCursor makeCursorOrNull(const Stmt *S, const Decl *Parent,
                                 CXTranslationUnit TU)
{
  if (S) {
    return MakeCXCursor(S, Parent, TU);
  }
  else {
    return clang_getNullCursor();
  }
}


extern "C"
CXCursor clang_forStmtElement(CXCursor cursor, CXForStmtElement element)
{
  // I'm not sure exactly what the semantics of 'parent' are meant to
  // be.  The comments in clang-c/Index.h do not explain.  So I am
  // simply propagating the existing parent, which is what I seem to see
  // other code in CXCursor.cpp doing.
  Decl const *parent = getCursorDecl(cursor);

  CXTranslationUnit tu = getCursorTU(cursor);

  Stmt const *stmt = getCursorStmt(cursor);
  if (ForStmt const *forStmt = dyn_cast_or_null<ForStmt>(stmt)) {
    switch (element) {
      default:
        break;

      case CXForStmtElement_init:
        return makeCursorOrNull(forStmt->getInit(), parent, tu);

      case CXForStmtElement_cond:
        return makeCursorOrNull(forStmt->getCond(), parent, tu);

      case CXForStmtElement_inc:
        return makeCursorOrNull(forStmt->getInc(), parent, tu);

      case CXForStmtElement_body:
        return makeCursorOrNull(forStmt->getBody(), parent, tu);
    }
  }

  return clang_getNullCursor();
}


// -------------------------- CharacterLiteral -------------------------
extern "C"
unsigned clang_characterLiteralElement(CXCursor characterLiteralCursor,
  CXCharacterLiteralElement element)
{
  Stmt const *stmt = getCursorStmt(characterLiteralCursor);

  if (CharacterLiteral const *lit = dyn_cast_or_null<CharacterLiteral>(stmt)) {
    switch (element) {
      default:
        break;

      case CXCharacterLiteralElement_kind:
        switch (lit->getKind()) {
          default:
            return CXCharacterLiteralKind_Unknown;

          #define KINDCASE(name) \
            case CharacterLiteral::name: return CXCharacterLiteralKind_##name;
          KINDCASE(Ascii)
          KINDCASE(Wide)
          KINDCASE(UTF8)
          KINDCASE(UTF16)
          KINDCASE(UTF32)
          #undef KINDCASE
        }

      case CXCharacterLiteralElement_value:
        return lit->getValue();
    }
  }

  return 0;
}


// --------------------------- StringLiteral ---------------------------
extern "C"
unsigned clang_stringLiteralElement(CXCursor stringLiteralCursor,
  CXStringLiteralElement element)
{
  Stmt const *stmt = getCursorStmt(stringLiteralCursor);

  // Disambiguate versus llvm::StringLiteral.
  using clang::StringLiteral;

  if (StringLiteral const *lit = dyn_cast_or_null<StringLiteral>(stmt)) {
    switch (element) {
      default:
        break;

      case CXStringLiteralElement_length:
        return lit->getLength();

      case CXStringLiteralElement_charByteWidth:
        return lit->getCharByteWidth();

      case CXStringLiteralElement_kind:
        switch (lit->getKind()) {
          default:
            return CXCharacterLiteralKind_Unknown;

          #define KINDCASE(name) \
            case StringLiteral::name: return CXCharacterLiteralKind_##name;
          KINDCASE(Ascii)
          KINDCASE(Wide)
          KINDCASE(UTF8)
          KINDCASE(UTF16)
          KINDCASE(UTF32)
          #undef KINDCASE
        }

      case CXStringLiteralElement_isPascal:
        return lit->isPascal();

      case CXStringLiteralElement_numConcatenated:
        return lit->getNumConcatenated();
    }
  }

  return 0;
}


extern "C"
void clang_getStringLiteralBytes(CXCursor stringLiteralCursor,
  unsigned char *contents, size_t contentsSize)
{
  Stmt const *stmt = getCursorStmt(stringLiteralCursor);

  using clang::StringLiteral;
  if (StringLiteral const *lit = dyn_cast_or_null<StringLiteral>(stmt)) {
    unsigned len = lit->getByteLength();
    assert(contentsSize >= len);

    StringRef bytes = lit->getBytes();
    assert(bytes.size() == len);

    memcpy(contents, bytes.bytes_begin(), len);
  }
}


// EOF
