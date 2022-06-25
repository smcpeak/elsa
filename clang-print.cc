// clang-print.cc
// Code for clang-print.h.

#include "clang-print.h"               // this module

#include <sstream>                     // std::ostringstream


std::string toString(CXString cxString)
{
  std::string ret(clang_getCString(cxString));
  clang_disposeString(cxString);
  return ret;
}


std::string toString(CXCursorKind cursorKind)
{
  struct Entry {
    CXCursorKind m_kind;
    char const *m_name;
  };

  static Entry const table[] = {
    #define ENTRY(name) { CXCursor_##name, #name }

    ENTRY(UnexposedDecl),
    ENTRY(StructDecl),
    ENTRY(UnionDecl),
    ENTRY(ClassDecl),
    ENTRY(EnumDecl),
    ENTRY(FieldDecl),
    ENTRY(EnumConstantDecl),
    ENTRY(FunctionDecl),
    ENTRY(VarDecl),
    ENTRY(ParmDecl),
    ENTRY(ObjCInterfaceDecl),
    ENTRY(ObjCCategoryDecl),
    ENTRY(ObjCProtocolDecl),
    ENTRY(ObjCPropertyDecl),
    ENTRY(ObjCIvarDecl),
    ENTRY(ObjCInstanceMethodDecl),
    ENTRY(ObjCClassMethodDecl),
    ENTRY(ObjCImplementationDecl),
    ENTRY(ObjCCategoryImplDecl),
    ENTRY(TypedefDecl),
    ENTRY(CXXMethod),
    ENTRY(Namespace),
    ENTRY(LinkageSpec),
    ENTRY(Constructor),
    ENTRY(Destructor),
    ENTRY(ConversionFunction),
    ENTRY(TemplateTypeParameter),
    ENTRY(NonTypeTemplateParameter),
    ENTRY(TemplateTemplateParameter),
    ENTRY(FunctionTemplate),
    ENTRY(ClassTemplate),
    ENTRY(ClassTemplatePartialSpecialization),
    ENTRY(NamespaceAlias),
    ENTRY(UsingDirective),
    ENTRY(UsingDeclaration),
    ENTRY(TypeAliasDecl),
    ENTRY(ObjCSynthesizeDecl),
    ENTRY(ObjCDynamicDecl),
    ENTRY(CXXAccessSpecifier),
    ENTRY(ObjCSuperClassRef),
    ENTRY(ObjCProtocolRef),
    ENTRY(ObjCClassRef),
    ENTRY(TypeRef),
    ENTRY(CXXBaseSpecifier),
    ENTRY(TemplateRef),
    ENTRY(NamespaceRef),
    ENTRY(MemberRef),
    ENTRY(LabelRef),
    ENTRY(OverloadedDeclRef),
    ENTRY(VariableRef),
    ENTRY(InvalidFile),
    ENTRY(NoDeclFound),
    ENTRY(NotImplemented),
    ENTRY(InvalidCode),
    ENTRY(UnexposedExpr),
    ENTRY(DeclRefExpr),
    ENTRY(MemberRefExpr),
    ENTRY(CallExpr),
    ENTRY(ObjCMessageExpr),
    ENTRY(BlockExpr),
    ENTRY(IntegerLiteral),
    ENTRY(FloatingLiteral),
    ENTRY(ImaginaryLiteral),
    ENTRY(StringLiteral),
    ENTRY(CharacterLiteral),
    ENTRY(ParenExpr),
    ENTRY(UnaryOperator),
    ENTRY(ArraySubscriptExpr),
    ENTRY(BinaryOperator),
    ENTRY(CompoundAssignOperator),
    ENTRY(ConditionalOperator),
    ENTRY(CStyleCastExpr),
    ENTRY(CompoundLiteralExpr),
    ENTRY(InitListExpr),
    ENTRY(AddrLabelExpr),
    ENTRY(StmtExpr),
    ENTRY(GenericSelectionExpr),
    ENTRY(GNUNullExpr),
    ENTRY(CXXStaticCastExpr),
    ENTRY(CXXDynamicCastExpr),
    ENTRY(CXXReinterpretCastExpr),
    ENTRY(CXXConstCastExpr),
    ENTRY(CXXFunctionalCastExpr),
    ENTRY(CXXTypeidExpr),
    ENTRY(CXXBoolLiteralExpr),
    ENTRY(CXXNullPtrLiteralExpr),
    ENTRY(CXXThisExpr),
    ENTRY(CXXThrowExpr),
    ENTRY(CXXNewExpr),
    ENTRY(CXXDeleteExpr),
    ENTRY(UnaryExpr),
    ENTRY(ObjCStringLiteral),
    ENTRY(ObjCEncodeExpr),
    ENTRY(ObjCSelectorExpr),
    ENTRY(ObjCProtocolExpr),
    ENTRY(ObjCBridgedCastExpr),
    ENTRY(PackExpansionExpr),
    ENTRY(SizeOfPackExpr),
    ENTRY(LambdaExpr),
    ENTRY(ObjCBoolLiteralExpr),
    ENTRY(ObjCSelfExpr),
    ENTRY(OMPArraySectionExpr),
    ENTRY(ObjCAvailabilityCheckExpr),
    ENTRY(FixedPointLiteral),
    ENTRY(OMPArrayShapingExpr),
    ENTRY(OMPIteratorExpr),
    ENTRY(CXXAddrspaceCastExpr),
    ENTRY(UnexposedStmt),
    ENTRY(LabelStmt),
    ENTRY(CompoundStmt),
    ENTRY(CaseStmt),
    ENTRY(DefaultStmt),
    ENTRY(IfStmt),
    ENTRY(SwitchStmt),
    ENTRY(WhileStmt),
    ENTRY(DoStmt),
    ENTRY(ForStmt),
    ENTRY(GotoStmt),
    ENTRY(IndirectGotoStmt),
    ENTRY(ContinueStmt),
    ENTRY(BreakStmt),
    ENTRY(ReturnStmt),
    ENTRY(GCCAsmStmt),
    ENTRY(ObjCAtTryStmt),
    ENTRY(ObjCAtCatchStmt),
    ENTRY(ObjCAtFinallyStmt),
    ENTRY(ObjCAtThrowStmt),
    ENTRY(ObjCAtSynchronizedStmt),
    ENTRY(ObjCAutoreleasePoolStmt),
    ENTRY(ObjCForCollectionStmt),
    ENTRY(CXXCatchStmt),
    ENTRY(CXXTryStmt),
    ENTRY(CXXForRangeStmt),
    ENTRY(SEHTryStmt),
    ENTRY(SEHExceptStmt),
    ENTRY(SEHFinallyStmt),
    ENTRY(MSAsmStmt),
    ENTRY(NullStmt),
    ENTRY(DeclStmt),
    ENTRY(OMPParallelDirective),
    ENTRY(OMPSimdDirective),
    ENTRY(OMPForDirective),
    ENTRY(OMPSectionsDirective),
    ENTRY(OMPSectionDirective),
    ENTRY(OMPSingleDirective),
    ENTRY(OMPParallelForDirective),
    ENTRY(OMPParallelSectionsDirective),
    ENTRY(OMPTaskDirective),
    ENTRY(OMPMasterDirective),
    ENTRY(OMPCriticalDirective),
    ENTRY(OMPTaskyieldDirective),
    ENTRY(OMPBarrierDirective),
    ENTRY(OMPTaskwaitDirective),
    ENTRY(OMPFlushDirective),
    ENTRY(SEHLeaveStmt),
    ENTRY(OMPOrderedDirective),
    ENTRY(OMPAtomicDirective),
    ENTRY(OMPForSimdDirective),
    ENTRY(OMPParallelForSimdDirective),
    ENTRY(OMPTargetDirective),
    ENTRY(OMPTeamsDirective),
    ENTRY(OMPTaskgroupDirective),
    ENTRY(OMPCancellationPointDirective),
    ENTRY(OMPCancelDirective),
    ENTRY(OMPTargetDataDirective),
    ENTRY(OMPTaskLoopDirective),
    ENTRY(OMPTaskLoopSimdDirective),
    ENTRY(OMPDistributeDirective),
    ENTRY(OMPTargetEnterDataDirective),
    ENTRY(OMPTargetExitDataDirective),
    ENTRY(OMPTargetParallelDirective),
    ENTRY(OMPTargetParallelForDirective),
    ENTRY(OMPTargetUpdateDirective),
    ENTRY(OMPDistributeParallelForDirective),
    ENTRY(OMPDistributeParallelForSimdDirective),
    ENTRY(OMPDistributeSimdDirective),
    ENTRY(OMPTargetParallelForSimdDirective),
    ENTRY(OMPTargetSimdDirective),
    ENTRY(OMPTeamsDistributeDirective),
    ENTRY(OMPTeamsDistributeSimdDirective),
    ENTRY(OMPTeamsDistributeParallelForSimdDirective),
    ENTRY(OMPTeamsDistributeParallelForDirective),
    ENTRY(OMPTargetTeamsDirective),
    ENTRY(OMPTargetTeamsDistributeDirective),
    ENTRY(OMPTargetTeamsDistributeParallelForDirective),
    ENTRY(OMPTargetTeamsDistributeParallelForSimdDirective),
    ENTRY(OMPTargetTeamsDistributeSimdDirective),
    ENTRY(BuiltinBitCastExpr),
    ENTRY(OMPMasterTaskLoopDirective),
    ENTRY(OMPParallelMasterTaskLoopDirective),
    ENTRY(OMPMasterTaskLoopSimdDirective),
    ENTRY(OMPParallelMasterTaskLoopSimdDirective),
    ENTRY(OMPParallelMasterDirective),
    ENTRY(OMPDepobjDirective),
    ENTRY(OMPScanDirective),
    ENTRY(OMPTileDirective),
    ENTRY(OMPCanonicalLoop),
    ENTRY(OMPInteropDirective),
    ENTRY(OMPDispatchDirective),
    ENTRY(OMPMaskedDirective),
    ENTRY(OMPUnrollDirective),
    ENTRY(OMPMetaDirective),
    ENTRY(OMPGenericLoopDirective),
    ENTRY(TranslationUnit),
    ENTRY(UnexposedAttr),
    ENTRY(IBActionAttr),
    ENTRY(IBOutletAttr),
    ENTRY(IBOutletCollectionAttr),
    ENTRY(CXXFinalAttr),
    ENTRY(CXXOverrideAttr),
    ENTRY(AnnotateAttr),
    ENTRY(AsmLabelAttr),
    ENTRY(PackedAttr),
    ENTRY(PureAttr),
    ENTRY(ConstAttr),
    ENTRY(NoDuplicateAttr),
    ENTRY(CUDAConstantAttr),
    ENTRY(CUDADeviceAttr),
    ENTRY(CUDAGlobalAttr),
    ENTRY(CUDAHostAttr),
    ENTRY(CUDASharedAttr),
    ENTRY(VisibilityAttr),
    ENTRY(DLLExport),
    ENTRY(DLLImport),
    ENTRY(NSReturnsRetained),
    ENTRY(NSReturnsNotRetained),
    ENTRY(NSReturnsAutoreleased),
    ENTRY(NSConsumesSelf),
    ENTRY(NSConsumed),
    ENTRY(ObjCException),
    ENTRY(ObjCNSObject),
    ENTRY(ObjCIndependentClass),
    ENTRY(ObjCPreciseLifetime),
    ENTRY(ObjCReturnsInnerPointer),
    ENTRY(ObjCRequiresSuper),
    ENTRY(ObjCRootClass),
    ENTRY(ObjCSubclassingRestricted),
    ENTRY(ObjCExplicitProtocolImpl),
    ENTRY(ObjCDesignatedInitializer),
    ENTRY(ObjCRuntimeVisible),
    ENTRY(ObjCBoxable),
    ENTRY(FlagEnum),
    ENTRY(ConvergentAttr),
    ENTRY(WarnUnusedAttr),
    ENTRY(WarnUnusedResultAttr),
    ENTRY(AlignedAttr),
    ENTRY(PreprocessingDirective),
    ENTRY(MacroDefinition),
    ENTRY(MacroExpansion),
    ENTRY(InclusionDirective),
    ENTRY(ModuleImportDecl),
    ENTRY(TypeAliasTemplateDecl),
    ENTRY(StaticAssert),
    ENTRY(FriendDecl),
    ENTRY(OverloadCandidate),

    #undef ENTRY
  };

  for (Entry const &entry : table) {
    if (entry.m_kind == cursorKind) {
      return entry.m_name;
    }
  }

  std::ostringstream oss;
  oss << "(unknown kind " << (int)cursorKind << ")";
  return oss.str();
}


std::string cursorKindClassificationsString(CXCursorKind cursorKind)
{
  std::ostringstream oss;

  #define KIND_FLAG(name, letter)   \
    if (clang_##name(cursorKind)) { \
      oss << letter;                \
    }

  // Columns: \S+ \S+
  KIND_FLAG(isDeclaration,     "d")
  KIND_FLAG(isReference,       "r")
  KIND_FLAG(isExpression,      "e")
  KIND_FLAG(isStatement,       "s")
  KIND_FLAG(isAttribute,       "a")
  KIND_FLAG(isInvalid,         "I")
  KIND_FLAG(isTranslationUnit, "T")
  KIND_FLAG(isPreprocessing,   "P")
  KIND_FLAG(isUnexposed,       "u")

  #undef KIND_FLAG

  return oss.str();
}


std::string toString(CXTypeKind typeKind)
{
  return toString(clang_getTypeKindSpelling(typeKind));
}


// EOF
