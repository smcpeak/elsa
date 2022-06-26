// clang-print.cc
// Code for clang-print.h.

#include "clang-print.h"               // this module

// ast
#include "asthelp.h"                   // ind

// libc++
#include <iostream>                    // std::ostream
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


std::string toLCString(CXSourceLocation loc)
{
  unsigned line;
  unsigned column;
  clang_getFileLocation(loc, nullptr /*file*/, &line, &column,
                        nullptr /*offset*/);

  std::ostringstream oss;
  oss << line << ':' << column;
  return oss.str();
}


std::string cursorLocLC(CXCursor cursor)
{
  return toLCString(clang_getCursorLocation(cursor));
}


char const *toString(CX_StorageClass storageClass)
{
  struct Entry {
    CX_StorageClass m_storageClass;
    char const *m_name;
  };

  static Entry const table[] = {
    #define ENTRY(name) { CX_SC_##name, #name }
    ENTRY(Invalid),
    ENTRY(None),
    ENTRY(Extern),
    ENTRY(Static),
    ENTRY(PrivateExtern),
    ENTRY(OpenCLWorkGroupLocal),
    ENTRY(Auto),
    ENTRY(Register),
    #undef ENTRY
  };

  for (Entry const &e : table) {
    if (e.m_storageClass == storageClass) {
      return e.m_name;
    }
  }

  return "(invalid storage class)";
}


std::string typeSpelling(CXType type)
{
  return toString(clang_getTypeSpelling(type));
}


static std::vector<CXCursor> getTypeFields(CXType cxType)
{
  std::vector<CXCursor> fields;

  clang_Type_visitFields(cxType,
    [](CXCursor child, CXClientData client_data)
    {
      std::vector<CXCursor> *fields =
        static_cast<std::vector<CXCursor>*>(client_data);
      fields->push_back(child);
      return CXVisit_Continue;
    },
    &fields);

  return fields;
}


static CXChildVisitResult childCollector(CXCursor cursor,
  CXCursor parent, CXClientData client_data)
{
  std::vector<CXCursor> *children =
    static_cast<std::vector<CXCursor>*>(client_data);
  children->push_back(cursor);
  return CXChildVisit_Continue;
}

std::vector<CXCursor> getChildren(CXCursor cursor)
{
  std::vector<CXCursor> children;
  clang_visitChildren(cursor, childCollector, &children);
  return children;
}


// --------------------------- ClangPrint ------------------------------
bool ClangPrint::maybePrintType(char const *label, CXType cxType)
{
  if (cxType.kind != CXType_Invalid) {
    m_os << " " << label << "=\"" << typeSpelling(cxType) << "\"";
    return true;
  }
  return false;
}


void ClangPrint::printCursor(CXCursor cursor, int indent)
{
  std::string spelling = toString(clang_getCursorSpelling(cursor));
  std::string display = toString(clang_getCursorDisplayName(cursor));

  CXSourceRange extent = clang_getCursorExtent(cursor);

  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  ind(m_os, indent)
    << "cursor: kind=" << toString(cursorKind)
    << "(" << cursorKindClassificationsString(cursorKind) << ")"
    << " loc=" << cursorLocLC(cursor)
    << " range=[" << toLCString(clang_getRangeStart(extent))
    << "," << toLCString(clang_getRangeEnd(extent))
    << ") spelling=\"" << spelling << "\"";

  if (spelling != display) {
    m_os << " display=\"" << display << "\"";
  }

  bool hasType = maybePrintType("type", clang_getCursorType(cursor));

  if (clang_isExpression(cursorKind)) {
    // 'getReceiverType' crashes if passed a non-expression.
    maybePrintType("receiverType", clang_Cursor_getReceiverType(cursor));
  }

  CXCursor decl = clang_getCursorReferenced(cursor);
  if (!clang_Cursor_isNull(decl)) {
    m_os << " references=" << cursorLocLC(decl);
  }

  CX_StorageClass storageClass = clang_Cursor_getStorageClass(cursor);
  if (storageClass != CX_SC_Invalid && storageClass != CX_SC_None) {
    m_os << " storage=" << toString(storageClass);
  }

  m_os << "\n";

  if (m_maxTokensToPrint) {
    printTokens(cursor, indent+2);
  }

  if (hasType && m_printTypes) {
    printTypeTree(clang_getCursorType(cursor), indent+2);
  }

  for (CXCursor const &child : getChildren(cursor)) {
    printCursor(child, indent+2);
  }
}


void ClangPrint::printTokens(CXCursor cursor, int indent)
{
  ind(m_os, indent) << "tokens:";

  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

  CXToken *tokens;
  unsigned numTokens;
  clang_tokenize(tu, clang_getCursorExtent(cursor), &tokens, &numTokens);

  int i;
  for (i=0; i < (int)numTokens && i < m_maxTokensToPrint; ++i) {
    std::string tok = toString(clang_getTokenSpelling(tu, tokens[i]));
    m_os << ' ' << tok;
  }

  if (i < (int)numTokens) {
    m_os << " (+" << (numTokens-i) << " more)";
  }

  // Clean up.
  clang_disposeTokens(tu, tokens, numTokens);

  m_os << '\n';
}


void ClangPrint::printTypeTree(CXType cxType, int indent)
{
  CXTypeKind typeKind = cxType.kind;
  std::string kindName = toString(typeKind);

  ind(m_os, indent) << "type: kind=" << kindName;

  CXCursor cxDecl = clang_getTypeDeclaration(cxType);
  if (!clang_Cursor_isNull(cxDecl)) {
    m_os << " declLoc=" << cursorLocLC(cxDecl);
  }

  m_os << " spelling=\"" << typeSpelling(cxType) << "\"";

  CXType canonical = clang_getCanonicalType(cxType);
  if (!clang_equalTypes(cxType, canonical)) {
    m_os << " canonical=\"" << typeSpelling(canonical) << "\"";
  }

  if (clang_isConstQualifiedType(cxType)) {
    m_os << " const";
  }

  if (clang_isVolatileQualifiedType(cxType)) {
    m_os << " volatile";
  }

  if (clang_isRestrictQualifiedType(cxType)) {
    m_os << " restrict";
  }

  m_os << " tsize=" << clang_Type_getSizeOf(cxType);
  m_os << " align=" << clang_Type_getAlignOf(cxType);

  // Inline attributes of certain types.
  switch (typeKind) {
    default:
      break;

    case CXType_Pointer:
    case CXType_LValueReference:
    case CXType_RValueReference:
      break;

    case CXType_Record:
      if (clang_isPODType(cxType)) {
        m_os << " pod";
      }
      break;

    case CXType_Enum:
      break;

    case CXType_Typedef:
      m_os << " typedefName=\"" << toString(clang_getTypedefName(cxType)) << "\"";
      if (clang_Type_isTransparentTagTypedef(cxType)) {
        m_os << " transparent";
      }
      break;

    case CXType_FunctionNoProto:
    case CXType_FunctionProto:
      // TODO: Calling convention.
      // TODO: Exception specification.
      m_os << " args=" << clang_getNumArgTypes(cxType);
      if (clang_isFunctionTypeVariadic(cxType)) {
        m_os << " variadic";
      }
      if (CXRefQualifierKind rq = clang_Type_getCXXRefQualifier(cxType)) {
        m_os << " refqual=" << rq;
      }
      break;

    case CXType_ConstantArray:
      m_os << " asize=" << clang_getArraySize(cxType);
      break;

    case CXType_IncompleteArray:
    case CXType_VariableArray:
    case CXType_DependentSizedArray:
      break;

    case CXType_MemberPointer:
      break;

    case CXType_Elaborated:
      break;
  }

  m_os << "\n";
  indent += 2;

  // Child types.
  switch (typeKind) {
    default:
      break;

    case CXType_Pointer:
      printTypeTree(clang_getPointeeType(cxType), indent);
      break;

    case CXType_LValueReference:
    case CXType_RValueReference:
      break;

    case CXType_Record: {
      std::vector<CXCursor> fields = getTypeFields(cxType);
      for (CXCursor const &field : fields) {
        std::string name = toString(clang_getCursorSpelling(field));

        // I do not recursively print details about field types because
        // that could lead to an infinite loop.
        std::string type = typeSpelling(clang_getCursorType(field));

        ind(m_os, indent) << "field: name=" << name
                          << " type=\"" << type << "\"\n";
      }
      break;
    }

    case CXType_Enum:
    case CXType_Typedef:
      break;

    case CXType_FunctionNoProto:
    case CXType_FunctionProto: {
      printTypeTree(clang_getResultType(cxType), indent);
      int numArgs = clang_getNumArgTypes(cxType);
      for (int i=0; i < numArgs; i++) {
        printTypeTree(clang_getArgType(cxType, i), indent);
      }
      break;
    }

    case CXType_Complex:
    case CXType_ConstantArray:
    case CXType_IncompleteArray:
    case CXType_VariableArray:
    case CXType_DependentSizedArray:
      printTypeTree(clang_getElementType(cxType), indent);
      break;

    case CXType_MemberPointer:
      printTypeTree(clang_Type_getClassType(cxType), indent);
      break;

    case CXType_Elaborated:
      printTypeTree(clang_Type_getNamedType(cxType), indent);
      break;
  }
}


// EOF
