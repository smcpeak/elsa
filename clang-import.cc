// clang-import.cc
// Code for clang-import.h.

#include "clang-import-internal.h"     // this module

// elsa
#include "clang-print.h"               // toString(CXCursorKind), etc.
#include "clang-additions.h"           // clang_getUnaryExpressionOperator, etc.

// smbase
#include "codepoint.h"                 // isASCIIDigit, isCIdentifierCharacter
#include "exc.h"                       // xfatal
#include "map-utils.h"                 // insertMapUnique
#include "parsestring.h"               // ParseString
#include "str.h"                       // string
#include "trace.h"                     // tracingSys

// clang
#include "clang/AST/DeclBase.h"                            // Decl
#include "clang/AST/OperationKinds.h"                      // UnaryOperatorKind, BinaryOperatorKind
#include "clang/Basic/Diagnostic.h"                        // DiagnosticEngine
#include "clang/Basic/DiagnosticOptions.h"                 // DiagnosticOptions
#include "clang/Basic/LLVM.h"                              // IntrusiveRefCntPtr
#include "clang/Driver/Driver.h"                           // driver::Driver
#include "clang/Frontend/CompilerInstance.h"               // CompilerInstance
#include "clang/Serialization/PCHContainerOperations.h"    // PCHContainerOperations

// libc++
#include <sstream>                     // std::ostringstream

// libc
#include <stdlib.h>                    // atoi, getenv


// Unfortunately, I cannot just say "using namespace clang" because
// clang/Basic/LLVM.h contains "using llvm::StringRef", and
// llvm::StringRef collides with the one I define in strtable.h.  So,
// instead, pull in the needed classes and namespaces one by one.
//
// Also, for AST classes, I want them to always be explicitly qualified
// with "clang::" in this file so they can readily be distinguished from
// Elsa AST classes, so I do not pull any of those in with a 'using'
// declaration.
using llvm::IntrusiveRefCntPtr;
using llvm::dyn_cast;
using clang::CompilerInstance;
using clang::DiagnosticsEngine;
using clang::DiagnosticOptions;
using clang::PCHContainerOperations;
namespace driver = clang::driver;


#if 1
  #define CI_TRACE(stuff) TRACE("clang-import", stuff)
#else
  #define CI_TRACE(stuff) ((void)0)
#endif


// Downcast 'oldVar' to 'destClass', which is in the 'clang' namespace,
// storing the result in 'newVar', and asserting it succeeds.
#define CLANG_DOWNCAST(destClass, newVar, oldVar)                      \
  clang::destClass const *newVar = dyn_cast<clang::destClass>(oldVar); \
  xassert(newVar) /* user ; */


// Downcast within an 'if' condition such that the 'if' will do the test.
#define CLANG_DOWNCAST_IN_IF(destClass, newVar, oldVar) \
  clang::destClass const *newVar = dyn_cast<clang::destClass>(oldVar)


// This is a candidate to go into smbase.
template <class DEST, class SRC>
void checkedAssignment(DEST &dest, SRC src)
{
  dest = static_cast<DEST>(src);
  if (static_cast<SRC>(dest) != src ||
      (dest < 0) != (src < 0)) {
    xfatal(stringb("Value " << src << " is outside representable range."));
  }
}


static std::string stmtToString(clang::Stmt const *clangStmt)
{
  std::string ret;
  llvm::raw_string_ostream rso(ret);

  clang::LangOptions lo;
  clang::PrintingPolicy pp(lo);

  clangStmt->printPretty(rso, nullptr, pp);

  return ret;
}


// Entry point to this module.
void clangParseTranslationUnit(
  ElsaParse &elsaParse,
  std::vector<std::string> const &gccOptions)
{
  // Copy the arguments into a vector of char pointers since that is
  // what 'LoadFromCommandLine' wants.
  std::vector<char const *> commandLine;
  {
    // The documentation for clang::createInvocation (which is what
    // LoadFromCommandLine calls internally) says the first argument
    // should be the driver name, and that an absolute path is
    // preferred, although I would expect system headers to be found
    // using 'resourcesPath', not this, so I'll try the simplest thing.
    commandLine.push_back("clang");

    // Optionally suppress warnings.
    if (!elsaParse.m_printWarnings) {
      commandLine.push_back("-w");
    }

    for (std::string const &s : gccOptions) {
      commandLine.push_back(s.c_str());
    }
  }

  std::shared_ptr<PCHContainerOperations> pchContainerOps(
    new PCHContainerOperations());

  // Arrange to print the option names with emitted diagnostics.
  //
  // NOTE: Setting some things in 'diagnosticOptions', such as its
  // 'Warnings' field, will not work here because they will be
  // overridden when the command line is processed later.  So we have to
  // configure options using command line syntax.
  DiagnosticOptions *diagnosticOptions = new DiagnosticOptions;
  diagnosticOptions->ShowOptionNames = true;

  // The diagnostics engine created this way will simply print all
  // warnings and errors to stderr as they occur, and also maintain a
  // count of each.
  IntrusiveRefCntPtr<DiagnosticsEngine> diagnosticsEngine(
    CompilerInstance::createDiagnostics(
      diagnosticOptions /*callee takes ownership*/));

  // Get the path to the headers (etc.) Clang uses while parsing.  The
  // 'GetResourcesPath' function accepts a path to a file in the Clang
  // lib or bin directory, and immediately discards the file name itself
  // in order to get the directory (and then append more things).  So I
  // append a dummy file name to the lib directory.
  //
  // I'm not actually sure if I need or want this to be set properly
  // since my intent for the moment is to always supply my own header
  // files rather than using Clang's, but as an experiment I'll start by
  // trying to specify it correctly.
  std::string resourcesPath =
    driver::Driver::GetResourcesPath(CLANG_LLVM_LIB_DIR "/dummy");

  // Run the Clang parser to produce an AST.
  std::unique_ptr<clang::ASTUnit> ast(clang::ASTUnit::LoadFromCommandLine(
    commandLine.data(),
    commandLine.data() + commandLine.size(),
    pchContainerOps,
    diagnosticsEngine,
    resourcesPath));

  if (ast == nullptr) {
    // Error messages should already have been printed, so this might
    // be redundant.
    xfatal("clang parse failed to produce an AST");
  }

  if (diagnosticsEngine->getNumErrors() > 0) {
    // The errors should have been printed already.
    xfatal("clang reported errors, stopping");
  }

  ClangImport importClang(std::move(ast), elsaParse);
  importClang.importTranslationUnit();
}


// ----------------------- PartitionedChildren -------------------------
static CXChildVisitResult partitionedChildCollector(CXCursor cursor,
  CXCursor parent, CXClientData client_data)
{
  PartitionedChildren *children =
    static_cast<PartitionedChildren*>(client_data);

  CXCursorKind kind = clang_getCursorKind(cursor);
  if (clang_isDeclaration(kind)) {
    children->m_declarations.push_back(cursor);
  }
  else if (clang_isExpression(kind)) {
    children->m_expressions.push_back(cursor);
  }
  else if (clang_isStatement(kind)) {
    children->m_statements.push_back(cursor);
  }
  else if (clang_isAttribute(kind)) {
    children->m_attributes.push_back(cursor);
  }
  else if (kind == CXCursor_TypeRef) {
    // These are entirely useless, so I don't even bother to save them.
  }
  else {
    xunimp(stringb("partitionedChildCollector: kind: " << toString(kind)));
  }

  return CXChildVisit_Continue;
}


PartitionedChildren::PartitionedChildren(CXCursor cursor)
  : m_declarations(),
    m_expressions(),
    m_statements(),
    m_attributes()
{
  clang_visitChildren(cursor, partitionedChildCollector, this);
}


bool PartitionedChildren::empty() const
{
  return m_declarations.empty() &&
         m_expressions.empty() &&
         m_statements.empty() &&
         m_attributes.empty();
}


// --------------------------- ClangImport -----------------------------
ClangImport::ClangImport(std::unique_ptr<clang::ASTUnit> &&clangASTUnit,
                         ElsaParse &elsaParse)
  : ElsaASTBuild(elsaParse.m_stringTable,
                 elsaParse.m_typeFactory,
                 elsaParse.m_lang,
                 *this),
    m_clangASTUnit(std::move(clangASTUnit)),
    m_elsaParse(elsaParse),
    m_tfac(elsaParse.m_typeFactory),
    m_globalScope(new Scope(SK_GLOBAL, 0 /*changeCount*/, SL_INIT)),
    m_clangFileIDToName(),
    m_clangDeclToVariable()
{}


void ClangImport::importTranslationUnit()
{
  bool dump = tracingSys("dumpClang");

  m_elsaParse.m_translationUnit = new TranslationUnit(nullptr);

  // An ASTUnit should have a TranslationUnitDecl inside it, acting as
  // the context for all of the top-level declarations, but I do not see
  // a way to directly get the TranslationUnitDecl.  Instead it seems I
  // have to iterate over the top-level declarations.
  for (auto it = m_clangASTUnit->top_level_begin();
       it != m_clangASTUnit->top_level_end();
       ++it) {
    clang::Decl *decl = *it;
    if (dump) {
      decl->dump();
    }
    m_elsaParse.m_translationUnit->topForms.append(importTopForm(decl));
  }
}


clang::ASTContext const &ClangImport::getASTContext() const
{
  return m_clangASTUnit->getASTContext();
}


StringRef ClangImport::importStringRef(clang::StringRef clangString)
{
  // Generally, Elsa uses NULL pointers to represent absent names.
  if (clangString.empty()) {
    return nullptr;
  }

  // This could be made more efficient.
  std::string s(clangString.data(), clangString.size());
  return addStringRef(s.c_str());
}


StringRef ClangImport::importFileName(clang::FileID clangFileID)
{
  auto it = m_clangFileIDToName.find(clangFileID);
  if (it == m_clangFileIDToName.end()) {
    clang::SourceManager const &srcMgr =
      m_clangASTUnit->getSourceManager();
    clang::FileEntry const *fileEntry =
      srcMgr.getFileEntryForID(clangFileID);
    StringRef fname = importStringRef(fileEntry->getName());
    insertMapUnique(m_clangFileIDToName, clangFileID, fname);
    return fname;
  }
  else {
    return (*it).second;
  }
}


SourceLoc ClangImport::importSourceLocation(clang::SourceLocation clangLoc)
{
  clang::SourceManager const &srcMgr =
    m_clangASTUnit->getSourceManager();

  clang::FileID fid = srcMgr.getFileID(clangLoc);
  if (!fid.isValid()) {
    return SL_UNKNOWN;
  }
  StringRef fname = importFileName(fid);

  // TODO: It would probably be more efficient to use the offset.
  unsigned line = srcMgr.getSpellingLineNumber(clangLoc);
  unsigned column = srcMgr.getSpellingColumnNumber(clangLoc);

  return sourceLocManager->encodeLineCol(fname, line, column);
}


SourceLoc ClangImport::declLocation(clang::Decl const *clangDecl)
{
  // Decl has both 'getBeginLoc' and 'getLocation'.  I'm not sure how
  // they are different, but for now I will use the latter since it is
  // perhaps less specific.
  return importSourceLocation(clangDecl->getLocation());
}


SourceLoc ClangImport::stmtLocation(clang::Stmt const *clangStmt)
{
  return importSourceLocation(clangStmt->getBeginLoc());
}


SourceLoc ClangImport::exprLocation(clang::Expr const *clangExpr)
{
  // For the moment, just use the fact that expressions are statements.
  // I might later want to adjust how expression locations work.
  return stmtLocation(clangExpr);
}


SourceLoc ClangImport::cursorLocation(CXCursor cxCursor)
{
  xunimp("old");
  return SL_UNKNOWN;
  //return importSourceLocation(clang_getCursorLocation(cxCursor));
}


StringRef ClangImport::cursorSpelling(CXCursor cxCursor)
{
  xunimp("old");
  return nullptr;
  //return importString(clang_getCursorSpelling(cxCursor));
}


TopForm *ClangImport::importTopForm(clang::Decl const *clangTopFormDecl)
{
  SourceLoc loc = declLocation(clangTopFormDecl);

  if (clang::NamedDecl const *cnd =
        dyn_cast<clang::NamedDecl>(clangTopFormDecl)) {
    // Handle function definitions separately.
    if (clang::FunctionDecl const *cfd =
          dyn_cast<clang::FunctionDecl>(cnd)) {
      // The 'hasBody' method reports true for a function prototype if
      // the TU also has a definition somewhere else.  So, use the query
      // that is specific to this particular declaration.
      if (cfd->doesThisDeclarationHaveABody()) {
        Function *func = importFunctionDefinition(cfd);
        return new TF_func(loc, func);
      }
    }

    return new TF_decl(loc, importNamedDecl(cnd, DC_TF_DECL));
  }

  xunimp(stringb("importTopForm: Unknown kind: " <<
                 clangTopFormDecl->getDeclKindName()));
  return nullptr; // not reached
}


#if 0 // old
Declaration *ClangImport::importDeclaration(CXCursor cxDecl,
  DeclaratorContext context)
{
  CXCursorKind cxKind = clang_getCursorKind(cxDecl);
  switch (cxKind) {
    default:
      xfailure(stringb("importDeclaration: Unknown cxKind: " << toString(cxKind)));

    case CXCursor_StructDecl: // 2
    case CXCursor_UnionDecl: // 3
    case CXCursor_ClassDecl: // 4
      if (clang_isCursorDefinition(cxDecl)) {
        return importCompoundTypeDefinition(cxDecl);
      }
      else {
        return importCompoundTypeForwardDeclaration(cxDecl);
      }

    case CXCursor_EnumDecl: // 5
      if (clang_isCursorDefinition(cxDecl)) {
        return importEnumDefinition(cxDecl);
      }
      else {
        printSubtree(cxDecl);
        xunimp("enum forward declaration");
      }

    case CXCursor_FunctionDecl: // 8
    case CXCursor_CXXMethod: // 21
      // This is only meant for use with non-definition declarations.
      xassert(!clang_isCursorDefinition(cxDecl));
      // Fallthrough.

    case CXCursor_VarDecl: // 9
    case CXCursor_TypedefDecl: // 20
      return importNamedDecl(cxDecl, context);
  }

  // Not reached.
}
#endif // 0


Variable *ClangImport::importEnumTypeAsForward(
  clang::EnumDecl const *clangEnumDecl)
{
  StringRef enumName = declName(clangEnumDecl);

  // Have we already created a record for it?
  Variable *&var = variableRefForDeclaration(clangEnumDecl);
  if (!var) {
    // Conceptually, we would like to make a forward-declared enum, but
    // the Elsa type system cannot explicitly do that.  So we just make
    // a regular EnumType without any values.
    SourceLoc loc = declLocation(clangEnumDecl);

    EnumType *enumType = m_tfac.makeEnumType(enumName);
    Type *type = m_tfac.makeCVAtomicType(enumType, CV_NONE);
    var = enumType->typedefVar =
      makeVariable_setScope(loc, enumName, type, DF_TYPEDEF, clangEnumDecl);
  }

  else {
    // Verify the existing type matches.
    xassert(var->name == enumName);
    xassert(var->type->isEnumType());
  }

  return var;
}


Declaration *ClangImport::importEnumDefinition(
  clang::EnumDecl const *clangEnumDecl)
{
  Variable *enumVar = importEnumTypeAsForward(clangEnumDecl);
  EnumType *enumType = enumVar->type->asNamedAtomicType()->asEnumType();

  // Begin the syntactic list.
  FakeList<Enumerator> *enumerators = FakeList<Enumerator>::emptyList();

  // Get all of the enumerators and add them to 'enumType' and
  // 'enumerators'.
  for (auto it = clangEnumDecl->enumerator_begin();
       it != clangEnumDecl->enumerator_end();
       ++it) {
    clang::EnumConstantDecl const *clangEnumConstantDecl = *it;
    SourceLoc enumeratorLoc = declLocation(clangEnumConstantDecl);

    // Get the enumerator name and value.
    StringRef enumeratorName = declName(clangEnumConstantDecl);
    int intValue = importAPSIntAsInt(clangEnumConstantDecl->getInitVal());

    // Create a Variable to represent the enumerator, associated with
    // its declaration.
    Variable *&enumeratorVar = variableRefForDeclaration(clangEnumConstantDecl);
    xassert(!enumeratorVar);
    enumeratorVar = makeVariable_setScope(
      enumeratorLoc,
      enumeratorName,
      enumVar->type,
      DF_ENUMERATOR,
      clangEnumConstantDecl);

    // Add it to the semantic type.
    enumType->addValue(enumeratorName, intValue, enumeratorVar);

    // Get the expression that specifies the value, if any.
    Expression * NULLABLE expr = nullptr;
    if (clang::Expr const *clangInitExpr =
          clangEnumConstantDecl->getInitExpr()) {
      expr = importExpression(clangInitExpr);
    }

    // Build the enumerator syntax.
    Enumerator *enumerator =
      new Enumerator(enumeratorLoc, enumeratorName, expr);
    enumerator->var = enumeratorVar;
    enumerator->enumValue = intValue;
    enumerators = fl_prepend(enumerators, enumerator);
  }

  // The list was built in reverse order due to prepending.
  enumerators = fl_reverse(enumerators);

  TS_enumSpec *enumTS = new TS_enumSpec(
    declLocation(clangEnumDecl),
    enumVar->name,
    enumerators);
  enumTS->etype = legacyTypeNC(enumType);

  // Wrap up the type specifier in a declaration with no declarators.
  return new Declaration(DF_NONE, enumTS, nullptr /*decllist*/);
}


Variable *ClangImport::importCompoundTypeAsForward(
  clang::RecordDecl const *clangRecordDecl)
{
  StringRef compoundName = declName(clangRecordDecl);

  // What kind of compound is this?
  CompoundType::Keyword keyword;
  switch (clangRecordDecl->getTagKind()) {
    default:
      xunimp(stringb("importCompoundTypeAsForward: " <<
                     importStringRef(clangRecordDecl->getKindName())));
      // fake fallthrough

    case clang::TTK_Struct: keyword = CompoundType::K_STRUCT; break;
    case clang::TTK_Union:  keyword = CompoundType::K_UNION;  break;
    case clang::TTK_Class:  keyword = CompoundType::K_CLASS;  break;
  }

  // Have we already created a record for it?
  Variable *&var = variableRefForDeclaration(clangRecordDecl);
  if (!var) {
    // Make a forward-declared one.
    SourceLoc loc = declLocation(clangRecordDecl);

    // This makes a CompoundType with 'm_isForwardDeclared==true'.
    CompoundType *compoundType = m_tfac.makeCompoundType(keyword, compoundName);
    Type *type = m_tfac.makeCVAtomicType(compoundType, CV_NONE);
    var = compoundType->typedefVar =
      makeVariable_setScope(loc, compoundName, type, DF_TYPEDEF, clangRecordDecl);
  }

  else {
    // Verify the existing type matches.
    xassert(var->name == compoundName);
    CompoundType const *ct = var->type->asCompoundTypeC();
    xassert(ct->keyword == keyword);
  }

  return var;
}


Declaration *ClangImport::importCompoundTypeDefinition(
  clang::RecordDecl const *clangRecordDecl)
{
  Variable *var = importCompoundTypeAsForward(clangRecordDecl);
  CompoundType *ct = var->type->asCompoundType();

  // Begin the syntactic lists.
  FakeList<BaseClassSpec> *bases = FakeList<BaseClassSpec>::emptyList();
  MemberList *memberList = new MemberList(nullptr /*list*/);

  // Process the children of the definition.
  //
  // I'm processing the syntactic declaration list rather than, say,
  // separately iterating over fields, then methods, etc., because I'd
  // like the resulting Elsa AST to be similar to what would have been
  // obtained when using its parser.
  for (clang::Decl const *clangChildDecl : clangRecordDecl->decls()) {
    if (clangChildDecl->isImplicit()) {
      // For now, just ignore any implicit members.
      continue;
    }

    SourceLoc childLoc = declLocation(clangChildDecl);

    CVFlags methodCV = CV_NONE;

    Member *newMember = nullptr;
    switch (clangChildDecl->getKind()) {
      default:
        xunimp(stringb("compound defn child kind: " <<
                       clangChildDecl->getDeclKindName()));

      case clang::Decl::Enum:
      case clang::Decl::Record:
      case clang::Decl::CXXRecord: {
        CLANG_DOWNCAST(TagDecl, ctd, clangChildDecl);

        newMember = new MR_decl(childLoc,
                                importNamedDecl(ctd, DC_MR_DECL));
        break;
      }

      importDecl:
      case clang::Decl::Field: // 6: non-static data
      case clang::Decl::Var: { // 9: static data
        CLANG_DOWNCAST(DeclaratorDecl, cdd, clangChildDecl);

        // Associate the Variable we will create with the declaration of
        // the field.
        Variable *&fieldVar = variableRefForDeclaration(cdd);
        xassert(!fieldVar);

        Type *fieldType = importQualType_maybeMethod(
          cdd->getType(), ct, methodCV);

        // For static data members, get the storage class.
        DeclFlags dflags = DF_NONE;
        if (CLANG_DOWNCAST_IN_IF(VarDecl, cvd, cdd)) {
          dflags = importStorageClass(cvd->getStorageClass());
        }

        fieldVar = makeVariable_setScope(
          childLoc,
          declName(cdd),
          fieldType,
          dflags,
          cdd);

        ct->addField(fieldVar);

        newMember = new MR_decl(childLoc,
          makeDeclaration(fieldVar, DC_MR_DECL));
        break;
      }

      case clang::Decl::CXXMethod:
      case clang::Decl::CXXConstructor:
      case clang::Decl::CXXConversion:
      case clang::Decl::CXXDestructor: {
        CLANG_DOWNCAST(CXXMethodDecl, ccm, clangChildDecl);

        if (ccm->doesThisDeclarationHaveABody()) {
          Function *func = importFunctionDefinition(ccm);
          newMember = new MR_func(childLoc, func);
        }
        else {
          methodCV = importMethodQualifiers(ccm);
          goto importDecl;
        }
        break;
      }

      case clang::Decl::AccessSpec: {
        CLANG_DOWNCAST(AccessSpecDecl, casd, clangChildDecl);

        // Note: 'getAccess' is a method on 'Decl'.  All declarations
        // can be queried for their access level, but for access
        // specifier declarations themselves, the access level is the
        // only information they carry (beyond source location details).
        newMember = new MR_access(childLoc,
          importAccessKeyword(casd->getAccess()));
        break;
      }
    }

    xassert(newMember);
    memberList->list.append(newMember);
  }

  // Now that the members have been populated, 'ct' is no longer forward.
  ct->m_isForwardDeclared = false;

  // Finish making the definition syntax.
  TS_classSpec *specifier = new TS_classSpec(
    declLocation(clangRecordDecl),
    CompoundType::toTypeIntr(ct->keyword),
    makePQName(var),
    bases,
    memberList);
  specifier->ctype = ct;

  // Wrap up the type specifier in a declaration with no declarators.
  return new Declaration(DF_NONE, specifier, nullptr /*decllist*/);
}


Declaration *ClangImport::importCompoundTypeForwardDeclaration(
  clang::RecordDecl const *clangRecordDecl)
{
  Variable *var = importCompoundTypeAsForward(clangRecordDecl);
  CompoundType *ct = var->type->asCompoundType();

  TS_elaborated *specifier = new TS_elaborated(
    declLocation(clangRecordDecl),
    CompoundType::toTypeIntr(ct->keyword),
    makePQName(var));
  specifier->atype = ct;

  return new Declaration(DF_NONE, specifier, nullptr /*decllist*/);
}


CVFlags ClangImport::importMethodQualifiers(
  clang::CXXMethodDecl const *clangCXXMethodDecl)
{
  return importQualifiers(clangCXXMethodDecl->getMethodQualifiers());
}


Function *ClangImport::importFunctionDefinition(
  clang::FunctionDecl const *clangFunctionDecl)
{
  xassert(clangFunctionDecl->doesThisDeclarationHaveABody());

  // Build a representative for the function.  This will have an
  // associated FunctionType, but without parameter names.
  Variable *funcVar = variableForDeclaration(clangFunctionDecl);
  FunctionType const *declFuncType = funcVar->type->asFunctionTypeC();

  CI_TRACE("importFunctionDefinition: " << funcVar->name);

  // Build a FunctionType specific to this definition, containing the
  // parameter names.
  FunctionType *defnFuncType = m_tfac.makeFunctionType(declFuncType->retType);

  // Possibly add the receiver parameter.
  if (clang::CXXMethodDecl const *ccmd =
        dyn_cast<clang::CXXMethodDecl>(clangFunctionDecl)) {
    if (!ccmd->isStatic()) {
      // The containing class is the semantic parent.
      clang::CXXRecordDecl const *clangCXXRecordDecl = ccmd->getParent();
      Variable *classTypedefVar =
        existingVariableForDeclaration(clangCXXRecordDecl);
      CVFlags cv = importMethodQualifiers(ccmd);
      addReceiver(defnFuncType, declLocation(clangFunctionDecl),
                  classTypedefVar->type->asCompoundTypeC(), cv);
    }
  }

  // Add parameters.
  unsigned numParams = clangFunctionDecl->getNumParams();
  for (unsigned i = 0; i < numParams; ++i) {
    clang::ParmVarDecl const *clangParmVarDecl =
      clangFunctionDecl->getParamDecl(i);
    Variable *paramVar =
      variableForDeclaration(clangParmVarDecl, DF_PARAMETER);
    defnFuncType->addParam(paramVar);
  }

  defnFuncType->flags = declFuncType->flags;
  m_tfac.doneParams(defnFuncType);

  std::pair<TypeSpecifier*, Declarator*> tsAndDeclarator =
    makeTSandDeclaratorForType(funcVar, defnFuncType, DC_FUNCTION);

  DeclFlags declFlags = possiblyAddNameQualifiers_and_getStorageClass(
    tsAndDeclarator.second, clangFunctionDecl);

  clang::Stmt const *clangFunctionBody = clangFunctionDecl->getBody();
  S_compound *body = nullptr;
  if (clang::CompoundStmt const *ccs =
        dyn_cast<clang::CompoundStmt>(clangFunctionBody)) {
    body = importCompoundStatement(ccs);
  }
  else {
    // I think the only other possibility is CXXTryStmt.
    xunimp(stringb("importFunctionDefinition: unhandled body type: " <<
                   clangFunctionBody->getStmtClassName()));
  }

  return new Function(
    declFlags,
    tsAndDeclarator.first,
    tsAndDeclarator.second,
    nullptr,       // inits
    body,
    nullptr);      // handlers
}


Variable *&ClangImport::variableRefForDeclaration(
  clang::NamedDecl const *clangNamedDecl)
{
  // Go to the canonical declaration to handle the case of an entity
  // that is declared multiple times.
  clangNamedDecl = dyn_cast<clang::NamedDecl>(clangNamedDecl->getCanonicalDecl());
  xassert(clangNamedDecl);

  return m_clangDeclToVariable[clangNamedDecl];
}


Variable *ClangImport::variableForDeclaration(
  clang::NamedDecl const *clangNamedDecl,
  DeclFlags declFlags)
{
  StringRef name = declName(clangNamedDecl);

  Variable *&var = variableRefForDeclaration(clangNamedDecl);
  if (!var) {
    Type *type;

    if (clang::TypedefNameDecl const *ctnd =
          dyn_cast<clang::TypedefNameDecl>(clangNamedDecl)) {
      declFlags |= DF_TYPEDEF;

      // For a typedef, the 'type' is the underlying type.
      type = importQualType(ctnd->getUnderlyingType());
    }

    else if (clang::ValueDecl const *cvd =
               dyn_cast<clang::ValueDecl>(clangNamedDecl)) {
      // For value, the type is what the declaration says it is.
      type = importQualType(cvd->getType());
    }

    else {
      xunimp(stringb("variableForDeclaration: unhandled kind: " <<
                     clangNamedDecl->getDeclKindName()));
    }

    SourceLoc loc = declLocation(clangNamedDecl);

    xassert(!var);
    var = makeVariable_setScope(loc, name, type, declFlags, clangNamedDecl);

    CI_TRACE("variableForDeclaration: name=\"" << name <<
             "\" clangNamedDecl=" << clangNamedDecl <<
             ": created new Variable at " << var <<
             " with flags {" << toString(declFlags) <<
             "}");
  }

  else {
    CI_TRACE("variableForDeclaration: name=\"" << name <<
             "\" clangNamedDecl=" << clangNamedDecl <<
             ": reusing Variable at " << var <<
             "; existing flags {" << toString(var->flags) <<
             "}, incoming flags {" << toString(declFlags) <<
             "}");

    // Detect location collisions.
    xassert(var->name == name);

#if 0 // old
    if (declKind == CXCursor_FunctionDecl) {
      FunctionType const *ft = var->type->asFunctionTypeC();
      if (ft->hasFlag(FF_NO_PARAM_INFO)) {
        // This is another declaration of a function for which we, so
        // far, do not have parameter info.  Import the type at this
        // location to see if it has parameter info.
        FunctionType const *ft2 =
          importQualType(clang_getCursorType(cxDecl))->asFunctionTypeC();
        if (!ft2->hasFlag(FF_NO_PARAM_INFO)) {
          // Update the Variable accordingly.
          var->type = legacyTypeNC(ft2);
        }
      }
    }
#endif // 0
  }

  return var;
}


Variable *ClangImport::existingVariableForDeclaration(
  clang::NamedDecl const *clangNamedDecl)
{
  Variable *&var = variableRefForDeclaration(clangNamedDecl);
  xassert(var);
  return var;
}


#if 0 // temporarily unneeded
// Return true if 'type' is a variable-length array.
static bool isVariableLengthArray(Type const *type)
{
  if (ArrayType const *at = type->ifArrayTypeC()) {
    return at->getSize() == ArrayType::DYN_SIZE;
  }
  else {
    return false;
  }
}
#endif // 0


Declaration *ClangImport::importNamedDecl(
  clang::NamedDecl const *clangNamedDecl,
  DeclaratorContext context)
{
  if (CLANG_DOWNCAST_IN_IF(EnumDecl, ced, clangNamedDecl)) {
    return importEnumDefinition(ced);
  }

  if (CLANG_DOWNCAST_IN_IF(RecordDecl, crd, clangNamedDecl)) {
    if (crd->isCompleteDefinition()) {
      return importCompoundTypeDefinition(crd);
    }
    else {
      return importCompoundTypeForwardDeclaration(crd);
    }
  }

  Variable *var = variableForDeclaration(clangNamedDecl);
  Declaration *decl = makeDeclaration(var, context);
  Declarator *declarator = fl_first(decl->decllist);

  // Add qualifiers if needed.
  decl->dflags |= possiblyAddNameQualifiers_and_getStorageClass(
    declarator, clangNamedDecl);

  // TODO: Attributes.

  // TODO: Get variable length array size.

  // Get initializer if there is one.
  if (CLANG_DOWNCAST_IN_IF(VarDecl, cvd, clangNamedDecl)) {
    if (cvd->hasInit()) {
      clang::Expr const *clangInitExpr = cvd->getInit();
      declarator->init = importInitializer(clangInitExpr);
    }
  }

  return decl;
}


DeclFlags ClangImport::declStorageClass(
  clang::NamedDecl const *clangNamedDecl)
{
  // VarDecl and FunctionDecl each have a storage class.

  if (clang::VarDecl const *cvd = dyn_cast<clang::VarDecl>(clangNamedDecl)) {
    return importStorageClass(cvd->getStorageClass());
  }

  if (clang::FunctionDecl const *cfd = dyn_cast<clang::FunctionDecl>(clangNamedDecl)) {
    return importStorageClass(cfd->getStorageClass());
  }

  return DF_NONE;
}


DeclFlags ClangImport::possiblyAddNameQualifiers_and_getStorageClass(
  Declarator *declarator, clang::NamedDecl const *clangNamedDecl)
{
  // Add qualifiers if needed.
  bool qualified = possiblyAddNameQualifiers(declarator, clangNamedDecl);

  DeclFlags declFlags = declStorageClass(clangNamedDecl);

  if (qualified) {
    // If a declaration uses a qualifier, it is not allowed to also
    // say 'static'.
    //
    // TODO: This seems aimed at handling static methods, where 'static'
    // can appear at a declaration or an inline definition, but not an
    // out-of-line definition, but I'm not sure this is correct.
    declFlags &= ~DF_STATIC;
  }

  return declFlags;
}


bool ClangImport::possiblyAddNameQualifiers(Declarator *declarator,
  clang::NamedDecl const *clangNamedDecl)
{
  SourceLoc loc = declLocation(clangNamedDecl);
  PQName *declaratorName = declarator->getDeclaratorId();
  int loopCounter = 0;

  clang::DeclContext const *lexParent =
    clangNamedDecl->getLexicalDeclContext();
  if (lexParent->getDeclKind() == clang::Decl::Function) {
    // A qualified name cannot be declared local to a function.
    return false;
  }

  clang::DeclContext const *semParent = clangNamedDecl->getDeclContext();
  xassert(semParent);
  while (!( lexParent == semParent ||
            semParent->getDeclKind() == clang::Decl::TranslationUnit )) {
    if (++loopCounter > 100) {
      // Guard against an infinite loop.
      xfatal("possiblyAddNameQualifiers: hit loop counter limit");
    }

    StringRef semParentName = declContextName(semParent);
    xassert(semParentName);

    // TODO: Handle template arguments.
    TemplateArgument *templArgs = nullptr;

    declaratorName = new PQ_qualifier(loc,
      semParentName, templArgs, declaratorName);

    semParent = semParent->getParent();
    xassert(semParent);
  }

  declarator->setDeclaratorId(declaratorName);
  return loopCounter > 0;
}


StringRef ClangImport::declName(clang::NamedDecl const *clangNamedDecl)
{
  clang::DeclarationName dname = clangNamedDecl->getDeclName();
  if (dname.isIdentifier()) {
    return importStringRef(clangNamedDecl->getName());
  }
  else {
    // Handle special names like constructors and conversion operators.
    //
    // TODO: This does not match what Elsa does.
    std::string str = dname.getAsString();
    return addStringRef(str.c_str());
  }
}


StringRef ClangImport::declContextName(
  clang::DeclContext const *clangDeclContext)
{
  if (clang::TagDecl const *ctd =
        dyn_cast<clang::TagDecl>(clangDeclContext)) {
    return declName(ctd);
  }

  xunimp(stringb("declContextName: unhandled kind: " <<
                 clangDeclContext->getDeclKindName()));
  return nullptr; // not reached
}


void ClangImport::importDeclaratorAttributes(Declarator *declarator,
  std::vector<CXCursor> const &cxAttributes)
{
  for (CXCursor const &cxAttribute : cxAttributes) {
    Attribute *attr = importAttribute(cxAttribute);
    AttributeSpecifier *aspec =
      new AttributeSpecifier(attr, nullptr /*next*/);
    AttributeSpecifierList *asl =
      new AttributeSpecifierList(aspec, nullptr /*next*/);

    // I am simply assuming that the attribute is to be associated with
    // the outermost declarator since I can't really tell with Clang.
    declarator->decl->appendASL(asl);
  }
}


Attribute *ClangImport::importAttribute(CXCursor cxAttribute)
{
  SourceLoc loc = cursorLocation(cxAttribute);
  CXAttributeSyntaxKind kind = clang_getAttributeSyntaxKind(cxAttribute);
  WrapCXString attrName(clang_getAttributeName(cxAttribute));
  WrapCXString prettyAttrString(clang_getAttributePrettyString(cxAttribute));

  switch (kind) {
    default:
      printSubtree(cxAttribute);
      xunimp(stringb("importAttribute: kind: " << kind));

    case CXAttributeSyntaxKind_GNU:
      return importGNUAttribute(loc,
        attrName.c_str(), prettyAttrString.c_str());
  }

  // Not reached.
}


Attribute *ClangImport::importGNUAttribute(SourceLoc loc,
  char const *attrName, char const *prettyAttrString)
{
  ParseString parser(prettyAttrString);
  parser.skipWS();
  parser.parseString("__attribute__((");

  // Empty attributes are dropped by Clang.
  xassert(parser.cur() != ')');

  // Get the name from the pretty string, and confirm it matches
  // 'attrName'.
  {
    string parsedName = parser.parseCToken();
    if (parsedName != attrName) {
      // The "pretty" form does not include the surrounding underscores,
      // but the 'attrName' may.
      if (stringb("__" << parsedName << "__") != attrName) {
        xfailure(stringb("importGNUAttribute: attrName=\"" << attrName <<
                         "\" but parsedName=\"" << parsedName << "\""));
      }
    }

    // We use 'attrName' rather than 'parsedName' in the created AST
    // node to preserve the underscore information.
  }

  if (parser.cur() == ')') {
    // The attribute is just the name.
    parser.parseString("))");
    parser.parseEOS();
    return new AT_word(loc, addStringRef(attrName));
  }

  // The attribute is a name followed by some arguments.
  parser.parseString("(");
  FakeList<ArgExpression> *args = FakeList<ArgExpression>::emptyList();

  while (true) {
    StringRef arg = addStringRef(parser.parseCToken().c_str());
    args = fl_prepend(args,
      new ArgExpression(importAttributeArgument(loc, arg)));

    parser.skipWS();
    if (parser.cur() == ')') {
      break;
    }
    else if (parser.cur() == ',') {
      parser.parseString(",");
      parser.skipWS();
    }
    else {
      parser.throwErr(stringb("importGNUAttribute: Unexpected char: '" <<
                              parser.cur() << "'."));
    }
  }

  args = fl_reverse(args);

  parser.parseString(")))");
  parser.parseEOS();

  return new AT_func(loc, addStringRef(attrName), args);
}


Expression *ClangImport::importAttributeArgument(SourceLoc loc,
  StringRef arg)
{
  if (isASCIIDigit(arg[0])) {
    // Number.
    return new E_intLit(arg);
  }
  else if (arg[0] == '"') {
    // String.
    return new E_stringLit(arg);
  }
  else if (arg[0] == '\'') {
    // Character.
    return new E_charLit(arg);
  }
  else if (isCIdentifierCharacter(arg[0])) {
    // Identifier.
    return new E_variable(new PQ_name(loc, arg));
  }
  else {
    xfatal(stringb("importGNUAttribute: Unexpected arg: \"" <<
                   arg << "\"."));
    return nullptr; // Not reached.
  }
}


static SimpleTypeId clangBuiltinTypeKindToSimpleTypeId(
  clang::BuiltinType::Kind kind)
{
  struct Entry {
    // I store the key in the table just so I can assert that it is
    // correct.
    clang::BuiltinType::Kind m_kind;

    // Some of the entries are NUM_SIMPLE_TYPES to indicate that Elsa
    // is missing the corresponding type.
    SimpleTypeId m_id;
  };

  static Entry const table[] = {
    #define ENTRY(kind, id) { clang::BuiltinType::kind, id }

    // Currently, this table only lists the values defined in
    // clang/AST/BuiltinTypes.def, even though the Kind enumeration has
    // more stuff for more specialized targets.  Consequently, the entry
    // indices are offset from the enumeration values by the value of
    // 'Void', the first enumeration.

    // BuiltinTypes.def has comments explaining what each of these are;
    // I've repeated some information in my own comments for non-obvious
    // cases.

    // Columns: \S+ \S+ \S+
    ENTRY(Void,           ST_VOID),
    ENTRY(Bool,           ST_BOOL),
    ENTRY(Char_U,         ST_CHAR),               // 'char' for targets where it is unsigned
    ENTRY(UChar,          ST_UNSIGNED_CHAR),
    ENTRY(WChar_U,        ST_WCHAR_T),            // 'wchar_t' where it is unsigned
    ENTRY(Char8,          NUM_SIMPLE_TYPES),
    ENTRY(Char16,         NUM_SIMPLE_TYPES),
    ENTRY(Char32,         NUM_SIMPLE_TYPES),
    ENTRY(UShort,         ST_UNSIGNED_SHORT_INT),
    ENTRY(UInt,           ST_UNSIGNED_INT),
    ENTRY(ULong,          ST_UNSIGNED_LONG_INT),
    ENTRY(ULongLong,      ST_UNSIGNED_LONG_LONG),
    ENTRY(UInt128,        NUM_SIMPLE_TYPES),
    ENTRY(Char_S,         ST_CHAR),               // 'char' where it is signed
    ENTRY(SChar,          ST_SIGNED_CHAR),
    ENTRY(WChar_S,        ST_WCHAR_T),            // 'wchar_t' where it is signed
    ENTRY(Short,          ST_SHORT_INT),
    ENTRY(Int,            ST_INT),
    ENTRY(Long,           ST_LONG_INT),
    ENTRY(LongLong,       ST_LONG_LONG),
    ENTRY(Int128,         NUM_SIMPLE_TYPES),
    ENTRY(ShortAccum,     NUM_SIMPLE_TYPES),
    ENTRY(Accum,          NUM_SIMPLE_TYPES),
    ENTRY(LongAccum,      NUM_SIMPLE_TYPES),
    ENTRY(UShortAccum,    NUM_SIMPLE_TYPES),
    ENTRY(UAccum,         NUM_SIMPLE_TYPES),
    ENTRY(ULongAccum,     NUM_SIMPLE_TYPES),
    ENTRY(ShortFract,     NUM_SIMPLE_TYPES),
    ENTRY(Fract,          NUM_SIMPLE_TYPES),
    ENTRY(LongFract,      NUM_SIMPLE_TYPES),
    ENTRY(UShortFract,    NUM_SIMPLE_TYPES),
    ENTRY(UFract,         NUM_SIMPLE_TYPES),
    ENTRY(ULongFract,     NUM_SIMPLE_TYPES),
    ENTRY(SatShortAccum,  NUM_SIMPLE_TYPES),
    ENTRY(SatAccum,       NUM_SIMPLE_TYPES),
    ENTRY(SatLongAccum,   NUM_SIMPLE_TYPES),
    ENTRY(SatUShortAccum, NUM_SIMPLE_TYPES),
    ENTRY(SatUAccum,      NUM_SIMPLE_TYPES),
    ENTRY(SatULongAccum,  NUM_SIMPLE_TYPES),
    ENTRY(SatShortFract,  NUM_SIMPLE_TYPES),
    ENTRY(SatFract,       NUM_SIMPLE_TYPES),
    ENTRY(SatLongFract,   NUM_SIMPLE_TYPES),
    ENTRY(SatUShortFract, NUM_SIMPLE_TYPES),
    ENTRY(SatUFract,      NUM_SIMPLE_TYPES),
    ENTRY(SatULongFract,  NUM_SIMPLE_TYPES),
    ENTRY(Half,           NUM_SIMPLE_TYPES),
    ENTRY(Float,          ST_FLOAT),
    ENTRY(Double,         ST_DOUBLE),
    ENTRY(LongDouble,     ST_LONG_DOUBLE),
    ENTRY(Float16,        NUM_SIMPLE_TYPES),
    ENTRY(BFloat16,       NUM_SIMPLE_TYPES),
    ENTRY(Float128,       NUM_SIMPLE_TYPES),
    ENTRY(Ibm128,         NUM_SIMPLE_TYPES),
    ENTRY(NullPtr,        NUM_SIMPLE_TYPES),

    #undef ENTRY
  };

  // The table covers the subrange from 'Void' to 'NullPtr'.
  STATIC_ASSERT(TABLESIZE(table) ==
    clang::BuiltinType::NullPtr - clang::BuiltinType::Void + 1);

  if (clang::BuiltinType::Void <= kind &&
                                  kind <= clang::BuiltinType::NullPtr) {
    unsigned index = kind - clang::BuiltinType::Void;

    xassert(index < TABLESIZE(table));
    Entry const &entry = table[index];
    if (entry.m_id != NUM_SIMPLE_TYPES) {
      return entry.m_id;
    }
  }

  // Unfortunately there isn't an easy way to stringify 'kind'.
  xunimp(stringb("clangBuiltinTypeKindToSimpleTypeId: Unhandled builtin type kind: " << kind));

  return NUM_SIMPLE_TYPES; // not reached
}


CVFlags ClangImport::importQualifiers(clang::Qualifiers clangQualifiers)
{
  CVFlags cv = CV_NONE;
  if (clangQualifiers.hasConst()) {
    cv |= CV_CONST;
  }
  if (clangQualifiers.hasVolatile()) {
    cv |= CV_VOLATILE;
  }
  if (clangQualifiers.hasRestrict()) {
    cv |= CV_RESTRICT;
  }
  return cv;
}


Type *ClangImport::importQualType(clang::QualType clangQualType)
{
  return importQualType_maybeMethod(clangQualType, nullptr /*methodCV*/, CV_NONE);
}


Type *ClangImport::importQualType_maybeMethod(clang::QualType clangQualType,
  CompoundType const * NULLABLE methodClass, CVFlags methodCV)
{
  xassert(!clangQualType.isNull());

  CVFlags cv = importQualifiers(clangQualType.getQualifiers());

  clang::Type const *clangType = clangQualType.getTypePtr();
  switch (clangType->getTypeClass()) {
    default:
      xunimp(stringb("Unhandled type class: " << clangType->getTypeClassName()));
      // fake fallthrough

    #define CASE(kind, var, code)                   \
      case clang::Type::kind: {                     \
        CLANG_DOWNCAST(kind##Type, var, clangType); \
        code                                        \
      }

    CASE(Builtin, cbt,
      return m_tfac.getSimpleType(
        clangBuiltinTypeKindToSimpleTypeId(cbt->getKind()), cv);
    )

    CASE(Pointer, cpt,
      return m_tfac.makePointerType(cv,
        importQualType(cpt->getPointeeType()));
    )

    CASE(LValueReference, clvr,
      return m_tfac.makeReferenceType(
        importQualType(clvr->getPointeeType()));
    )

    CASE(RValueReference, clvr,
      // TODO: Add rvalue references to Elsa so I can record this
      // properly.
      return m_tfac.makeReferenceType(
        importQualType(clvr->getPointeeType()));
    )

    // For a Record or Enum that does not go through Elaborated, create
    // a TypedefType, the same as the Typedef case.
    case clang::Type::Record:
    case clang::Type::Enum: {
      CLANG_DOWNCAST(TagType, ctt, clangType);

      Variable *typedefVar = existingVariableForDeclaration(ctt->getDecl());
      return m_tfac.makeTypedefType(typedefVar, cv);
    }

    CASE(Elaborated, cet,
      // If the syntax is 'enum E', get the enumeration E.
      clang::Type const *clangType = cet->getNamedType().getTypePtr();

      // It should be a type defined using a type tag keyword since
      // the tag is what appears in the elaborated specifier.
      CLANG_DOWNCAST(TagType, ctt, clangType);

      // Use its declaration to find my typedef Variable.
      Variable *typedefVar = existingVariableForDeclaration(ctt->getDecl());
      return typedefVar->type;
    )

    CASE(Typedef, ctt,
      clang::TypedefNameDecl const *ctnd = ctt->getDecl();
      Variable *typedefVar = existingVariableForDeclaration(ctnd);
      return m_tfac.makeTypedefType(typedefVar, cv);
    )

    case clang::Type::FunctionNoProto:
    case clang::Type::FunctionProto: {
      clang::FunctionType const *cft = dyn_cast<clang::FunctionType>(clangType);
      xassert(cft);
      xassert(cv == CV_NONE);
      return importFunctionType(cft, methodClass, methodCV);
    }

#if 0 // old
    case CXType_ConstantArray: // 112
    case CXType_IncompleteArray: // 114
    case CXType_VariableArray: // 115
    case CXType_DependentSizedArray: // 116
      xassert(cv == CV_NONE);
      return importArrayType(cxType);
#endif // 0

    CASE(Paren, cpt,
      return importQualType_maybeMethod(cpt->getInnerType(),
                                        methodClass, methodCV);
    )

    #undef CASE
  }
}


FunctionType *ClangImport::importFunctionType(
  clang::FunctionType const *clangFunctionType,
  CompoundType const * NULLABLE methodClass, CVFlags methodCV)
{
  Type *retType = importQualType(clangFunctionType->getReturnType());

  FunctionType *ft = m_tfac.makeFunctionType(retType);

  if (methodClass) {
    // Note: Even if 'clangFunctionType' refers to a method type, clang
    // does *not* report it as cv-qualified.  Instead, the declaration
    // itself can be const.  Consequently, 'methodCV' must be passed in
    // by the caller.
    addReceiver(ft, SL_UNKNOWN, methodClass, methodCV);
  }

  if (clang::FunctionProtoType const *cfpt =
        dyn_cast<clang::FunctionProtoType>(clangFunctionType)) {
    unsigned numParams = cfpt->getNumParams();
    for (unsigned i=0; i < numParams; i++) {
      Type *paramType = importQualType(cfpt->getParamType(i));

      // The Clang AST does not appear to record the names of function
      // parameters for non-definitions anywhere other than as tokens
      // that would have to be parsed.
      //
      // TODO: That is wrong; non-definition FunctionProto nodes have
      // ParmDecl children.  I should get the names from them.
      Variable *paramVar = makeVariable(SL_UNKNOWN, nullptr /*name*/,
        paramType, DF_PARAMETER);

      ft->addParam(paramVar);
    }

    if (cfpt->isVariadic()) {
      ft->setFlag(FF_VARARGS);
    }
  }

  else {
    xassert(dyn_cast<clang::FunctionNoProtoType>(clangFunctionType));

    // Note: If this declaration is also a definition, then C11
    // 6.7.6.3p14 says it accepts no parameters.  But the response
    // to Defect Report 317 says that, even so, the function type
    // still has no parameter info.
    //
    // https://www.open-std.org/jtc1/sc22/wg14/www/docs/dr_317.htm
    ft->setFlag(FF_NO_PARAM_INFO);
  }

  m_tfac.doneParams(ft);
  return ft;
}


void ClangImport::addReceiver(FunctionType *methodType,
  SourceLoc loc, CompoundType const *containingClass, CVFlags cv)
{
  Type *qualifiedCT =
    m_tfac.makeCVAtomicType(legacyTypeNC(containingClass), cv);
  Type *recvRefType = m_tfac.makeReferenceType(qualifiedCT);
  methodType->addReceiver(makeVariable(
    loc,
    addStringRef("__receiver"), // Like Env::receiverName.
    recvRefType,
    DF_PARAMETER));
}


ArrayType *ClangImport::importArrayType(CXType cxArrayType)
{
  Type *eltType = nullptr; // old: importQualType(clang_getElementType(cxArrayType));

  int size;
  switch (cxArrayType.kind) {
    default:
      xfailure("bad array type");

    case CXType_ConstantArray: // 112
      checkedAssignment(size, clang_getArraySize(cxArrayType));
      break;

    case CXType_IncompleteArray: // 114
      size = ArrayType::NO_SIZE;
      break;

    case CXType_VariableArray: // 115
      size = ArrayType::DYN_SIZE;
      break;

    case CXType_DependentSizedArray: // 116
      // Elsa really should have this as a separate case too.  I will
      // just say the size is unspecified.
      size = ArrayType::NO_SIZE;
      break;
  }

  return m_tfac.makeArrayType(eltType, size);
}


S_compound *ClangImport::importCompoundStatement(
  clang::CompoundStmt const *clangCompoundStmt)
{
  SourceLoc loc = stmtLocation(clangCompoundStmt);
  S_compound *comp = new S_compound(loc, nullptr /*stmts*/);

  for (clang::CompoundStmt::const_body_iterator it =
         clangCompoundStmt->body_begin();
       it != clangCompoundStmt->body_end();
       ++it) {
    clang::Stmt const *cs = *it;
    comp->stmts.append(importStatement(cs));
  }

  return comp;
}


static void assertEqualPQNames(
  PQName const *pq1,
  PQName const *pq2)
{
  // For expedience, just compare the string representations.
  xassert(pq1->toString() == pq2->toString());
}


// Assert that 'ts1' and 'ts2' say the same thing.
static void assertEqualTypeSpecifiers(
  TypeSpecifier const *ts1,
  TypeSpecifier const *ts2)
{
  xassert(ts1->cv == ts2->cv);
  xassert(ts1->kind() == ts2->kind());

  switch (ts1->kind()) {
    default:
      xfailure("bad TypeSpecifier kind");

    case TypeSpecifier::TS_NAME: {
      TS_name const *n1 = ts1->asTS_nameC();
      TS_name const *n2 = ts2->asTS_nameC();
      assertEqualPQNames(n1->name, n2->name);
      xassert(n1->typenameUsed == n2->typenameUsed);
      break;
    }

    case TypeSpecifier::TS_SIMPLE: {
      TS_simple const *s1 = ts1->asTS_simpleC();
      TS_simple const *s2 = ts2->asTS_simpleC();
      xassert(s1->id == s2->id);
      break;
    }

    case TypeSpecifier::TS_ELABORATED: {
      TS_elaborated const *e1 = ts1->asTS_elaboratedC();
      TS_elaborated const *e2 = ts2->asTS_elaboratedC();
      xassert(e1->keyword == e2->keyword);
      assertEqualPQNames(e1->name, e2->name);
      break;
    }

    case TypeSpecifier::TS_CLASSSPEC: {
      TS_classSpec const *c1 = ts1->asTS_classSpecC();
      TS_classSpec const *c2 = ts2->asTS_classSpecC();
      xassert(c1->keyword == c2->keyword);
      assertEqualPQNames(c1->name, c2->name);

      // I will just assume that the bases and fields agree if the
      // names agree.

      break;
    }

    case TypeSpecifier::TS_ENUMSPEC: {
      TS_enumSpec const *e1 = ts1->asTS_enumSpecC();
      TS_enumSpec const *e2 = ts2->asTS_enumSpecC();
      xassert(e1->name == e2->name);

      // Similarly, assume the enumerators agree.

      break;
    }

    case TypeSpecifier::TS_TYPEOF: {
      // TODO: Actually compare these.
      break;
    }
  }
}


Statement *ClangImport::importStatement(clang::Stmt const *clangStmt)
{
#if 0 // needed?
  if (clang_Cursor_isNull(cxStmt)) {
    // This is for a case like a missing initializer in a 'for'
    // statement.
    return new S_skip(SL_UNKNOWN, MI_IMPLICIT);
  }
#endif // 0

  SourceLoc loc = stmtLocation(clangStmt);

  if (CLANG_DOWNCAST_IN_IF(Expr, clangExpr, clangStmt)) {
    FullExpression *fullExpr = importFullExpression(clangExpr);
    return new S_expr(loc, fullExpr);
  }

  switch (clangStmt->getStmtClass()) {
    default:
      xunimp(stringb("importStatement: unhandled: " <<
                     clangStmt->getStmtClassName()));
      // fake fallthrough

    case clang::Stmt::LabelStmtClass: {
      CLANG_DOWNCAST(LabelStmt, cls, clangStmt);

      StringRef name = addStringRef(cls->getName());
      return new S_label(loc, name, importStatement(cls->getSubStmt()));
    }

    case clang::Stmt::CompoundStmtClass: {
      CLANG_DOWNCAST(CompoundStmt, ccs, clangStmt);

      return importCompoundStatement(ccs);
    }

    case clang::Stmt::CaseStmtClass: {
      CLANG_DOWNCAST(CaseStmt, ccs, clangStmt);

      if (ccs->caseStmtIsGNURange()) {
        xunimp("case statement with GNU range");
      }

      S_case *ret = new S_case(loc, importExpression(ccs->getLHS()),
        importStatement(ccs->getSubStmt()));
      ret->labelVal = evalExprAsInt(ccs->getLHS());
      return ret;
    }

    case clang::Stmt::DefaultStmtClass: {
      CLANG_DOWNCAST(DefaultStmt, cds, clangStmt);

      return new S_default(loc, importStatement(cds->getSubStmt()));
    }

    case clang::Stmt::IfStmtClass: {
      CLANG_DOWNCAST(IfStmt, cis, clangStmt);

      Statement *elseStmt;
      if (cis->getElse() == nullptr) {
        elseStmt = new S_skip(loc, MI_IMPLICIT);
      }
      else {
        elseStmt = importStatement(cis->getElse());
      }

      if (clang::DeclStmt const *condDecl =
            cis->getConditionVariableDeclStmt()) {
        // As with 'while', there could be an implicit conversion that
        // we are missing here.
        return new S_if(loc,
          importCondition(condDecl),
          importStatement(cis->getThen()),
          elseStmt);
      }
      else {
        return new S_if(loc,
          importCondition(cis->getCond()),
          importStatement(cis->getThen()),
          elseStmt);
      }
    }

    case clang::Stmt::SwitchStmtClass: {
      CLANG_DOWNCAST(SwitchStmt, css, clangStmt);

      return new S_switch(loc,
        importCondition(css->getCond()),
        importStatement(css->getBody()));
    }

    case clang::Stmt::WhileStmtClass: {
      CLANG_DOWNCAST(WhileStmt, cws, clangStmt);

      if (clang::DeclStmt const *condDecl =
            cws->getConditionVariableDeclStmt()) {
        // There is an implicit conversion to boolean when the condition
        // declares a non-bool variable.  Clang puts that into its
        // 'getCond()' expression, which is separate from the optional
        // variable declaration.  Elsa does not have a place to store
        // that implicit conversion, so I'll just ignore it.
        return new S_while(loc,
          importCondition(condDecl),
          importStatement(cws->getBody()));
      }
      else {
        return new S_while(loc,
          importCondition(cws->getCond()),
          importStatement(cws->getBody()));
      }
    }

    case clang::Stmt::DoStmtClass: {
      CLANG_DOWNCAST(DoStmt, cds, clangStmt);

      return new S_doWhile(loc,
        importStatement(cds->getBody()),
        importFullExpression(cds->getCond()));
    }

    case clang::Stmt::ForStmtClass: {
      CLANG_DOWNCAST(ForStmt, cfs, clangStmt);

      return new S_for(loc,
        importStatement(cfs->getInit()),
        importCondition(cfs->getCond()),
        importFullExpression(cfs->getInc()),
        importStatement(cfs->getBody())
      );
    }

    case clang::Stmt::GotoStmtClass: {
      CLANG_DOWNCAST(GotoStmt, cfs, clangStmt);

      StringRef name = importStringRef(cfs->getLabel()->getName());
      return new S_goto(loc, name);
    }

    // TODO: CXCursor_IndirectGotoStmt: // 211

    case clang::Stmt::ContinueStmtClass:
      return new S_continue(loc);

    case clang::Stmt::BreakStmtClass:
      return new S_break(loc);

    case clang::Stmt::ReturnStmtClass: {
      CLANG_DOWNCAST(ReturnStmt, crs, clangStmt);
      FullExpression *retval = nullptr;
      clang::Expr const *clangExpr = crs->getRetValue();
      if (clangExpr != nullptr) {
        retval = importFullExpression(clangExpr);
      }
      return new S_return(loc, retval);
    }

    case clang::Stmt::CXXTryStmtClass: {
      CLANG_DOWNCAST(CXXTryStmt, ccts, clangStmt);

      Statement *body = importStatement(ccts->getTryBlock());

      FakeList<Handler> *handlers = FakeList<Handler>::emptyList();
      for (unsigned i=0; i < ccts->getNumHandlers(); ++i) {
        handlers = fl_prepend(handlers, importHandler(ccts->getHandler(i)));
      }
      handlers = fl_reverse(handlers);

      return new S_try(loc, body, handlers);
    }

    case clang::Stmt::NullStmtClass:
      return new S_skip(loc, MI_EXPLICIT);

    case clang::Stmt::DeclStmtClass: {
      clang::DeclStmt const *cds = dyn_cast<clang::DeclStmt>(clangStmt);
      xassert(cds);

      // Overall declaration to build.
      Declaration *decl = nullptr;

      // Iterate over all the declarations in this statement.
      for (auto it = cds->decl_begin(); it != cds->decl_end(); ++it) {
        clang::Decl const *clangDecl = *it;
        xassert(clangDecl->getKind() == clang::Decl::Var ||
                clangDecl->getKind() == clang::Decl::Typedef);
        clang::NamedDecl const *cnd = dyn_cast<clang::NamedDecl>(clangDecl);
        xassert(cnd);

        if (!decl) {
          // First declaration.
          decl = importNamedDecl(cnd, DC_S_DECL);
        }
        else {
          // Subsequent declaration.
          Declaration *next = importNamedDecl(cnd, DC_S_DECL);

          // We will merge 'next' onto 'decl'.  First, check that the
          // declaration specifiers and type specifiers agree.
          xassert(decl->dflags == next->dflags);
          assertEqualTypeSpecifiers(decl->spec, next->spec);

          // Now join the declarator lists.
          xassert(fl_count(next->decllist) == 1);
          decl->decllist = fl_prepend(decl->decllist, fl_first(next->decllist));

          // Clean up 'next'.  This won't get everything because AST
          // nodes do not automatically delete their children, but it
          // gets what is easy.
          delete next->spec;
          next->spec = nullptr;
          next->decllist = FakeList<Declarator>::emptyList();
          delete next;
        }
      }

      // Should have seen at least one declaration.
      xassert(decl);

      // When there are multiple, we build the list in reverse order.
      decl->decllist = fl_reverse(decl->decllist);

      return new S_decl(loc, decl);
    }
  }

  return nullptr; // not reached
}


FullExpression *ClangImport::importFullExpression(
  clang::Expr const *clangExpr)
{
  return new FullExpression(importExpression(clangExpr));
}


CXCursor ClangImport::getOnlyChild(CXCursor cxNode)
{
  std::vector<CXCursor> children = getChildren(cxNode);
  xassert(children.size() == 1);
  return children.front();
}


void ClangImport::getTwoChildren(CXCursor &c1, CXCursor &c2,
  CXCursor cxNode)
{
  std::vector<CXCursor> children = getChildren(cxNode);
  xassert(children.size() == 2);
  c1 = children[0];
  c2 = children[1];
}


static UnaryOp clangUnaryToElsaUnary(clang::UnaryOperatorKind op)
{
  switch (op) {
    default: xfailure("bad code");
    case clang::UO_Plus:  return UNY_PLUS;
    case clang::UO_Minus: return UNY_MINUS;
    case clang::UO_Not:   return UNY_BITNOT;
    case clang::UO_LNot:  return UNY_NOT;
  }
}


static EffectOp clangUnaryToElsaEffect(clang::UnaryOperatorKind op)
{
  switch (op) {
    default: xfailure("bad code");
    case clang::UO_PostInc: return EFF_POSTINC;
    case clang::UO_PostDec: return EFF_POSTDEC;
    case clang::UO_PreInc:  return EFF_PREINC;
    case clang::UO_PreDec:  return EFF_PREDEC;
  }
}


static BinaryOp clangBinaryToElsaBinary(clang::BinaryOperatorKind op)
{
  switch (op) {
    default: xfailure("bad code");

    // Columns: case return
    case clang::BO_PtrMemD:   return BIN_DOT_STAR;
    case clang::BO_PtrMemI:   return BIN_ARROW_STAR;
    case clang::BO_Mul:       return BIN_MULT;
    case clang::BO_Div:       return BIN_DIV;
    case clang::BO_Rem:       return BIN_MOD;
    case clang::BO_Add:       return BIN_PLUS;
    case clang::BO_Sub:       return BIN_MINUS;
    case clang::BO_Shl:       return BIN_LSHIFT;
    case clang::BO_Shr:       return BIN_RSHIFT;
    case clang::BO_Cmp:       xunimp("spaceship");
    case clang::BO_LT:        return BIN_LESS;
    case clang::BO_GT:        return BIN_GREATER;
    case clang::BO_LE:        return BIN_LESSEQ;
    case clang::BO_GE:        return BIN_GREATEREQ;
    case clang::BO_EQ:        return BIN_EQUAL;
    case clang::BO_NE:        return BIN_NOTEQUAL;
    case clang::BO_And:       return BIN_BITAND;
    case clang::BO_Xor:       return BIN_BITXOR;
    case clang::BO_Or:        return BIN_BITOR;
    case clang::BO_LAnd:      return BIN_AND;
    case clang::BO_LOr:       return BIN_OR;
    case clang::BO_Assign:    return BIN_ASSIGN;
    case clang::BO_MulAssign: return BIN_MULT;
    case clang::BO_DivAssign: return BIN_DIV;
    case clang::BO_RemAssign: return BIN_MOD;
    case clang::BO_AddAssign: return BIN_PLUS;
    case clang::BO_SubAssign: return BIN_MINUS;
    case clang::BO_ShlAssign: return BIN_LSHIFT;
    case clang::BO_ShrAssign: return BIN_RSHIFT;
    case clang::BO_AndAssign: return BIN_BITAND;
    case clang::BO_XorAssign: return BIN_BITXOR;
    case clang::BO_OrAssign:  return BIN_BITOR;
    case clang::BO_Comma:     return BIN_COMMA;
  }
}


int ClangImport::importAPIntAsInt(llvm::APInt const &apInt, bool isSigned)
{
  int ret;
  if (isSigned) {
    int64_t i = apInt.getSExtValue();
    checkedAssignment(ret, i);
  }
  else {
    uint64_t i = apInt.getZExtValue();
    checkedAssignment(ret, i);
  }
  return ret;
}


#if 0   // not needed for the moment
static std::string apsIntToString(llvm::APSInt const &n)
{
  // Use an intentionally small size to force heap allocation so I can
  // confirm I'm using this right.
  llvm::SmallVector<char, 2> v;
  n.toString(v);

  // Copy 'v' to a std::string.  I believe 'toString' does not add a NUL
  // terminator, so I ensure there is one beyond what has been added.
  std::string s(v.size()+1, 0);
  for (size_t i=0; i < v.size(); ++i) {
    s[i] = v[i];
  }

  return s;
}
#endif // 0


int ClangImport::importAPSIntAsInt(llvm::APSInt const &apsInt)
{
#if 0   // Evidently my version of llvm lacks this check; it was added in 0a89825a28 on 2022-12-26.
  if (!apsInt.isRepresentableByInt64()) {
    xunimp(stringb("enumerator value is not representable with int64_t: " <<
                   apsIntToString(apInt)));
  }
#endif // 0

  int64_t i64Value = apsInt.getExtValue();
  int intValue;
  checkedAssignment(intValue, i64Value);
  return intValue;
}


int ClangImport::evalExprAsInt(clang::Expr const *clangExpr)
{
  clang::Expr::EvalResult result;
  if (clangExpr->EvaluateAsInt(result, getASTContext())) {
    xassert(result.Val.isInt());
    return importAPSIntAsInt(result.Val.getInt());
  }
  else {
    // TODO: It is possible to extract further diagnostics from
    // 'result'.
    xfailure(stringb("could not evaluate expression as integer: " <<
                     stmtToString(clangExpr)));
    return 0; // not reached
  }
}


long long ClangImport::evalAsLongLong(CXCursor cxExpr)
{
  CXEvalResult cxEvalResult = clang_Cursor_Evaluate(cxExpr);
  xassert(clang_EvalResult_getKind(cxEvalResult) == CXEval_Int);
  long long value = clang_EvalResult_getAsLongLong(cxEvalResult);
  clang_EvalResult_dispose(cxEvalResult);

  return value;
}


std::string ClangImport::evalAsString(CXCursor cxExpr)
{
  CXEvalResult cxEvalResult = clang_Cursor_Evaluate(cxExpr);
  xassert(clang_EvalResult_getKind(cxEvalResult) == CXEval_StrLiteral);
  std::string ret(clang_EvalResult_getAsStr(cxEvalResult));
  clang_EvalResult_dispose(cxEvalResult);

  return ret;
}


namespace {
  // 'ITER_RANGE' is something like 'clang::CXXConstructExpr::arg_range'.
  template <class ITER_RANGE>
  FakeList<ArgExpression> *importArgumentList(ClangImport &clangImport,
    ITER_RANGE const &arguments)
  {
    FakeList<ArgExpression> *args = FakeList<ArgExpression>::emptyList();
    for (clang::Expr const *clangArg : arguments) {
      args = fl_prepend(args, new ArgExpression(
        clangImport.importExpression(clangArg)));
    }
    args = fl_reverse(args);
    return args;
  }
}


Expression *ClangImport::importExpression(clang::Expr const *clangExpr)
{
#if 0 // old
  if (clang_Cursor_isNull(cxExpr)) {
    // This is used for a missing 'for' condition or increment.
    return new E_boolLit(true);
  }
#endif // 0

  Type const *type = importQualType(clangExpr->getType());

  Expression *ret = nullptr;

  switch (clangExpr->getStmtClass()) {
    default:
      xunimp(stringb("importExpression: unhandled: " <<
                     clangExpr->getStmtClassName()));
      // fake fallthrough

    case clang::Stmt::DeclRefExprClass: {
      CLANG_DOWNCAST(DeclRefExpr, cdre, clangExpr);

      clang::ValueDecl const *clangValueDecl = cdre->getDecl();

      // The expression might be referring to an implicitly declared
      // function in C, so we need to allow for the possibility that the
      // declaration has not yet been seen.
      Variable *var = variableForDeclaration(clangValueDecl);

      ret = makeE_variable(var);
      break;
    }

    case clang::Stmt::ConstantExprClass: {
      CLANG_DOWNCAST(ConstantExpr, cce, clangExpr);

      // ConstantExpr is basically just a marker wrapper for an
      // expression that happens to evaluate to a constant.  For the
      // purpose of importing an expression, just skip it.
      return importExpression(cce->getSubExpr());
    }

    case clang::Stmt::MemberExprClass: {
      CLANG_DOWNCAST(MemberExpr, cme, clangExpr);

      Expression *object = importExpression(cme->getBase());
      if (object->type->isPointerType()) {
        // The Clang AST uses the same node for "." and "->", with the
        // latter distinguished only by the fact that the child
        // expression in that case has pointer type.  So I explicitly
        // insert the dereference here.
        object = makeE_deref(object);
      }

      clang::ValueDecl const *clangMemberDecl = cme->getMemberDecl();
      Variable *field = existingVariableForDeclaration(clangMemberDecl);

      E_fieldAcc *acc = new E_fieldAcc(object, makePQName(field));
      acc->field = field;
      ret = acc;
      break;
    }

    case clang::Stmt::CallExprClass: {
      CLANG_DOWNCAST(CallExpr, clangCallExpr, clangExpr);

      // Import the callee expression.
      Expression *callee = importExpression(clangCallExpr->getCallee());

      if (E_implicitStandardConversion *calleeISC =
            callee->ifE_implicitStandardConversion()) {
        if (calleeISC->conv == SC_FUNC_TO_PTR) {
          // For ordinary calls to a named function, Clang inserts an
          // implicit conversion to the function's address.  I suppose
          // that is done to make the AST more uniform, but I don't like
          // it because it clutters the --print-isc output.  Discard the
          // ISC.
          callee = calleeISC->expr;
          calleeISC->expr = nullptr;
          delete calleeISC;
        }
      }

      ret = new E_funCall(callee,
        importArgumentList(*this, clangCallExpr->arguments()));
      break;
    }

    case clang::Stmt::IntegerLiteralClass: {
      CLANG_DOWNCAST(IntegerLiteral, cil, clangExpr);

      bool isSigned =
        clangExpr->getType().getTypePtr()->isSignedIntegerType();
      int value = importAPIntAsInt(cil->getValue(), isSigned);

      E_intLit *intLit = new E_intLit(
        addStringRef(stringbc(value)));
      intLit->i = value;
      ret = intLit;
      break;
    }

#if 0 // old
    case CXCursor_StringLiteral: // 109
      ret = importStringLiteral(cxExpr);
      break;

    case CXCursor_CharacterLiteral: // 110
      ret = importCharacterLiteral(cxExpr);
      break;
#endif // 0

    case clang::Stmt::ParenExprClass: {
      CLANG_DOWNCAST(ParenExpr, cpe, clangExpr);

      // Elsa drops parentheses, so I will do that here as well.
      ret = importExpression(cpe->getSubExpr());
      break;
    }

    case clang::Stmt::UnaryOperatorClass: {
      CLANG_DOWNCAST(UnaryOperator, cuo, clangExpr);

      clang::Expr const *clangChildExpr = cuo->getSubExpr();
      Expression *childExpr = importExpression(clangChildExpr);

      clang::UnaryOperatorKind opcode = cuo->getOpcode();
      switch (opcode) {
        default:
          xunimp(stringb("Unary operator: " << opcode));
          // fake fallthrough

        case clang::UO_PostInc:
        case clang::UO_PostDec:
        case clang::UO_PreInc:
        case clang::UO_PreDec: {
          EffectOp effOp = clangUnaryToElsaEffect(opcode);
          ret = new E_effect(effOp, childExpr);
          break;
        }

        case clang::UO_AddrOf:
          ret = new E_addrOf(childExpr);
          break;

        case clang::UO_Deref:
          ret = new E_deref(childExpr);
          break;

        case clang::UO_Plus:
        case clang::UO_Minus:
        case clang::UO_Not:
        case clang::UO_LNot: {
          UnaryOp unOp = clangUnaryToElsaUnary(opcode);
          ret = new E_unary(unOp, childExpr);
          break;
        }
      }
      break;
    }

#if 0 // old
    case CXCursor_ArraySubscriptExpr: { // 113
      CXCursor cxLHS, cxRHS;
      getTwoChildren(cxLHS, cxRHS, cxExpr);

      Expression *lhs = importExpression(cxLHS);
      Expression *rhs = importExpression(cxRHS);

      // One of the operands should be a pointer, and that determines
      // the type of the addition node.
      E_binary *bin = new E_binary(lhs, BIN_PLUS, rhs);
      if (lhs->type->isPointerType()) {
        bin->type = lhs->type;
      }
      else {
        xassert(rhs->type->isPointerType());
        bin->type = rhs->type;
      }

      // The Elsa AST represents 'a[b]' as '*(a+b)'.
      ret = new E_deref(bin);
      break;
    }
#endif // 0

    case clang::Stmt::BinaryOperatorClass: {
      CLANG_DOWNCAST(BinaryOperator, cbo, clangExpr);

      Expression *lhs = importExpression(cbo->getLHS());
      Expression *rhs = importExpression(cbo->getRHS());

      clang::BinaryOperatorKind opcode = cbo->getOpcode();
      BinaryOp binOp = clangBinaryToElsaBinary(opcode);
      if (clang::BO_Assign <= opcode &&
                              opcode <= clang::BO_OrAssign) {
        ret = new E_assign(lhs, binOp, rhs);
      }
      else {
        ret = new E_binary(lhs, binOp, rhs);
      }
      break;
    }

#if 0 // old
    case CXCursor_ConditionalOperator: { // 116
      std::vector<CXCursor> children = getChildren(cxExpr);
      xassert(children.size() == 3);
      ret = new E_cond(
        importExpression(children[0]),
        importExpression(children[1]),
        importExpression(children[2])
      );
      break;
    }
#endif // 0

    case clang::Stmt::CStyleCastExprClass: {
      CLANG_DOWNCAST(CStyleCastExpr, ccsce, clangExpr);

      // The destination type is simply the type of this expression.
      ASTTypeId *castType =
        makeASTTypeId(type, nullptr /*name*/, DC_E_CAST);

      clang::Expr const *clangChildExpr = ccsce->getSubExpr();

      ret = new E_cast(castType, importExpression(clangChildExpr));
      break;
    }

    case clang::Stmt::ImplicitCastExprClass: {
      CLANG_DOWNCAST(ImplicitCastExpr, cice, clangExpr);

      clang::Expr const *clangChildExpr = cice->getSubExpr();
      Type const *childType = importQualType(clangChildExpr->getType());
      if (type->equals(childType)) {
        // OLD: This happens for lvalue-to-rvalue conversions.  There
        // does not seem to be a way in libclang to see that directly,
        // so I will just drop the conversion.
        //
        // TODO: Maybe I can see it in the C++ API?
        return importExpression(clangChildExpr);
      }
      else {
        // Note: When multiple conversions take place at the same
        // location, Clang uses multiple conversion nodes, whereas Elsa
        // combines them.  For now at least, I'm not collapsing them the
        // way Elsa would.
        StandardConversion conv = describeAsStandardConversion(
          /*dest*/ type, /*src*/ childType, getSpecialExpr(clangChildExpr));
        ret = new E_implicitStandardConversion(conv,
          importExpression(clangChildExpr));
      }
      break;
    }

    case clang::Stmt::CXXThrowExprClass: {
      CLANG_DOWNCAST(CXXThrowExpr, ccte, clangExpr);

      Expression *expr = nullptr;
      if (ccte->getSubExpr()) {
        expr = importExpression(ccte->getSubExpr());
      }

      ret = new E_throw(expr);
      break;
    }

#if 0 // old
    case CXCursor_UnaryExpr: // 136
      ret = importUnaryOperator(cxExpr);
      break;
#endif // 0
  }

  ret->type = legacyTypeNC(type);
  return ret;
}


E_stringLit *ClangImport::importStringLiteral(CXCursor cxExpr)
{
  // The spelling has something like the original quoted syntax,
  // although it is different at least because (1) it concatenates all
  // of the adjacent tokens, and (2) it normalizes octal escape
  // sequences to use three digits.  Nevertheless, it is fine for my
  // purposes.
  StringRef quotedRef = cursorSpelling(cxExpr);

  E_stringLit *slit = new E_stringLit(quotedRef);

  unsigned length = clang_stringLiteralElement(cxExpr,
    CXStringLiteralElement_length);
  unsigned charByteWidth = clang_stringLiteralElement(cxExpr,
    CXStringLiteralElement_charByteWidth);
  xassert(charByteWidth > 0);

  size_t numBytes = length * charByteWidth;

  // Check for overflow.  This is valid because the multiplication
  // was done using unsigned arithmetic.
  xassert(numBytes / charByteWidth == length);

  slit->m_stringData.setAllocated(numBytes);
  clang_getStringLiteralBytes(cxExpr,
    slit->m_stringData.getData(), numBytes);

  return slit;
}


// Create the token denoting a character literal of 'kind' with 'value'.
static std::string getCharLitToken(unsigned value,
  CXCharacterLiteralKind kind)
{
  std::ostringstream litText;

  // If we print any integers, use uppercase hex.
  litText << std::uppercase << std::hex;

  // String literal prefix character.
  switch (kind) {
    case CXCharacterLiteralKind_Ascii:
      break;

    case CXCharacterLiteralKind_Wide:
      litText << 'L';
      break;

    default:
      xunimp(stringb("importCharacterLiteral: unhandled kind: " << kind));
  }

  litText << '\'';

  if (value == 0) {
    litText << "\\0";
  }
  else if (32 <= value && value <= 126) {
    if (value == '\\' || value == '\'') {
      litText << '\\';
    }
    litText << (char)value;
  }
  else if (kind == CXCharacterLiteralKind_Wide) {
    // Express as a single hex escape sequence.
    litText << "\\x" << value;
  }
  else if (value >= (unsigned)(-0x80) || value <= 0xFF) {
    // Express as a single two-digit hex escape sequence.
    litText << "\\x" << (value & 0xFF);
  }
  else {
    // Use a sequence of hex escape sequences, one for each byte.
    //
    // I'm not really sure if this is right.  This all depends on
    // "implementation-defined" behavior.  According to
    // https://en.cppreference.com/w/cpp/language/character_literal,
    // this should be at least approximately right for compilers other
    // than MSVC.
    std::vector<unsigned char> bytes;
    while (value != 0) {
      bytes.push_back(value & 0xFF);
      value >>= 8;
    }
    for (int i = bytes.size()-1; i >= 0; --i) {
      litText << "\\x" << (unsigned)bytes[i];
    }
  }

  litText << '\'';

  return litText.str();
}


E_charLit *ClangImport::importCharacterLiteral(CXCursor cxExpr)
{
  CXCharacterLiteralKind kind = static_cast<CXCharacterLiteralKind>(
    clang_characterLiteralElement(cxExpr, CXCharacterLiteralElement_kind));

  unsigned value = clang_characterLiteralElement(cxExpr,
    CXCharacterLiteralElement_value);

  E_charLit *charLit =
    new E_charLit(addStringRef(getCharLitToken(value, kind).c_str()));
  charLit->c = value;

  return charLit;
}



Expression *ClangImport::importUnaryOperator(
  clang::UnaryOperator const *clangUnaryOperator)
{
  xunimp("importUnaryOperator");
  return nullptr;

#if 0
  CXUnaryExprKind unaryKind = clang_unaryExpr_operator(cxExpr);
  CXCursor child = getOnlyChild(cxExpr);
  Expression *operand = importExpression(child);

  switch (unaryKind) {
    default:
      xunimp(stringb("unary expr operator: " << toString(unaryKind)));

    case CXUnaryExprKind_SizeOf:
      return new E_sizeof(operand);

    case CXUnaryExprKind_AlignOf:
    case CXUnaryExprKind_PreferredAlignOf:
      return new E_alignofExpr(operand);
  }

  // Not reached.
#endif // 0
}


SpecialExpr ClangImport::getSpecialExpr(clang::Expr const *clangExpr)
{
  if (CLANG_DOWNCAST_IN_IF(IntegerLiteral, cil, clangExpr)) {
    if (cil->getValue() == 0) {
      return SE_ZERO;
    }
  }

  if (clangExpr->getStmtClass() == clang::Stmt::StringLiteralClass) {
    return SE_STRINGLIT;
  }

  return SE_NONE;
}


StandardConversion ClangImport::describeAsStandardConversion(
  Type const *destType, Type const *srcType, SpecialExpr srcSpecial)
{
  // Use the Elsa type checker rules.
  string errorMsg;
  StandardConversion sc = getStandardConversion(
    &errorMsg,
    m_elsaParse.m_lang,
    srcSpecial,
    srcType,
    destType);

  if (sc == SC_ERROR) {
    if (destType->isVoid()) {
      // Assume this is something like the ?: exception that allows an
      // implicit conversion to void.
      return SC_ADH_TO_VOID;
    }

    if (destType->isIntegerType() && srcType->isPointerType()) {
      // This shows up for in/c/t0010.c.  Clang issues a warning on the
      // command line, but allows it.
      return SC_ADH_PTR_TO_INT;
    }

    if (destType->isPointerType() && srcType->isIntegerType()) {
      // Another from in/c/t0010.c.
      return SC_ADH_INT_TO_PTR;
    }

    xfatal(stringb(
      "describeAsStandardConversion: destType='" << destType->toString() <<
      "' srcType='" << srcType->toString() <<
      "' srcSpecial=" << toString(srcSpecial) <<
      " error=\"" << errorMsg << "\""));
  }

  return sc;
}


Condition *ClangImport::importCondition(clang::Stmt const *clangCond)
{
  if (CLANG_DOWNCAST_IN_IF(Expr, clangExpr, clangCond)) {
    return new CN_expr(importFullExpression(clangExpr));
  }
  else if (CLANG_DOWNCAST_IN_IF(DeclStmt, clangDecl, clangCond)) {
    xassert(clangDecl->isSingleDecl());
    CLANG_DOWNCAST(VarDecl, clangVarDecl, clangDecl->getSingleDecl());
    xassert(clangVarDecl->hasInit());

    return new CN_decl(importASTTypeId(clangVarDecl, DC_CN_DECL));
  }
  else {
    xunimp(stringb("importCondition: " <<
                    clangCond->getStmtClassName()));
    return nullptr; // not reached
  }
}


ASTTypeId *ClangImport::importASTTypeId(
  clang::VarDecl const *clangVarDecl,
  DeclaratorContext context)
{
  Variable *var = variableForDeclaration(clangVarDecl);
  ASTTypeId *typeId =
    makeASTTypeId(var->type, makePQName(var), context);

  if (clangVarDecl->hasInit()) {
    typeId->decl->init = importInitializer(clangVarDecl->getInit());
  }

  return typeId;
}


Initializer *ClangImport::importInitializer(
  clang::Expr const *clangInitExpr)
{
  SourceLoc loc = exprLocation(clangInitExpr);

  // In the clang AST, the various kinds of initializer are all just
  // treated as kinds of expressions, even though they can't appear in
  // most expression contexts.  In contrast, the Elsa AST represents
  // them with their own Initializer class hierarchy.  So we need to
  // check for specific kinds of clang expressions to map them to Elsa
  // Initializers.

  switch (clangInitExpr->getStmtClass()) {
    case clang::Stmt::ImplicitValueInitExprClass:
      // In Elsa, this is represented with a NULL initializer.
      return nullptr;

    case clang::Stmt::CXXConstructExprClass: {
      CLANG_DOWNCAST(CXXConstructExpr, ccce, clangInitExpr);

      return new IN_ctor(loc,
        importArgumentList(*this, ccce->arguments()));
    }

    case clang::Stmt::InitListExprClass: {
      CLANG_DOWNCAST(InitListExpr, cile, clangInitExpr);
      IN_compound *inc = new IN_compound(loc, nullptr /*inits*/);
      unsigned numInits = cile->getNumInits();
      for (unsigned i=0; i < numInits; ++i) {
        clang::Expr const *clangInitElem = cile->getInit(i);
        inc->inits.append(importInitializer(clangInitElem));
      }
      return inc;
    }

    default:
      break;
  }

  return new IN_expr(loc, importExpression(clangInitExpr));
}


Handler *ClangImport::importHandler(
  clang::CXXCatchStmt const *clangCXXCatchStmt)
{
  ASTTypeId *typeId;
  if (clang::VarDecl const *clangDecl =
        clangCXXCatchStmt->getExceptionDecl()) {
    typeId = importASTTypeId(clangDecl, DC_HANDLER);
  }
  else {
    typeId = Handler::makeEllipsisTypeId(
      importSourceLocation(clangCXXCatchStmt->getCatchLoc()));

    // Fill in the declarator 'type' and 'var' fields similar to how the
    // Elsa type checker does, in order to pacify the integrity checker.
    typeId->decl->type = m_tfac.getSimpleType(ST_ELLIPSIS);
    typeId->decl->var = makeVariable(SL_UNKNOWN, nullptr /*name*/,
      typeId->decl->type, DF_NONE);
  }

  Statement *body = importStatement(clangCXXCatchStmt->getHandlerBlock());

  return new Handler(typeId, body);
}


Variable *ClangImport::makeVariable_setScope(SourceLoc loc,
  StringRef name, Type *type, DeclFlags flags,
  clang::NamedDecl const *clangNamedDecl)
{
  Variable *var = makeVariable(loc, name, type, flags);

  clang::DeclContext const *declContext =
    clangNamedDecl->getDeclContext();
  xassert(declContext);      // Can this be null?

  if (declContext->getDeclKind() == clang::Decl::TranslationUnit) {
    var->m_containingScope = m_globalScope;
  }

  return var;
}


Variable *ClangImport::makeVariable(SourceLoc loc, StringRef name,
  Type *type, DeclFlags flags)
{
  return m_tfac.makeVariable(loc, name, type, flags);
}


StringRef ClangImport::addStringRef(char const *str)
{
  return m_elsaParse.m_stringTable.add(str);
}


DeclFlags ClangImport::importStorageClass(clang::StorageClass storageClass)
{
  switch (storageClass) {
    default: xfailure(stringb("unknown storage class: " << storageClass));

    // Columns: \S+ \S+ \S+ \S+
    case clang::SC_None:          return DF_NONE;
    case clang::SC_Extern:        return DF_EXTERN;
    case clang::SC_Static:        return DF_STATIC;
    case clang::SC_PrivateExtern: return DF_NONE;     // No Elsa equivalent, not sure what it is.
    case clang::SC_Auto:          return DF_AUTO;
    case clang::SC_Register:      return DF_REGISTER;
  }
}


AccessKeyword ClangImport::importAccessKeyword(
  clang::AccessSpecifier accessSpecifier)
{
  switch (accessSpecifier) {
    case clang::AS_public:    return AK_PUBLIC;
    case clang::AS_protected: return AK_PROTECTED;
    case clang::AS_private:   return AK_PRIVATE;
    default:                  return AK_UNSPECIFIED;
  }
}


void ClangImport::printSubtree(CXCursor cursor)
{
  ClangPrint clangPrint(cout);

  static char const *maxTokensStr = getenv("MAX_TOKENS");
  if (maxTokensStr) {
    clangPrint.m_maxTokensToPrint = atoi(maxTokensStr);
  }

  clangPrint.printCursor(cursor, 0 /*indent*/);
}


// EOF
