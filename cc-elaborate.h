// cc-elaborate.h            see license.txt for copyright and terms of use
// semantic elaboration pass
// see also cc-elaborate.ast

#ifndef CC_ELABORATE_H
#define CC_ELABORATE_H


// The concept of semantic elaboration is that we provide, for many
// constructs in the language, equivalent alternative formulations in
// terms of a smaller set of primitive operations.  Then, an analysis
// can use the elaboration instead of building into itself the language
// spec's specified semantics.


#include "ast_build.h"                 // ElsaASTBuild
#include "elab-activities.h"           // ElabActivities
#include "cc-ast.h"                    // AST components, etc.
#include "cc-ast-aux.h"                // class LoweredASTVisitor


// counter for generating unique throw clause names; NOTE: FIX: they
// must not be duplicated across translation units since they are
// global!  Thus, this must survive even multiple Env objects.
// (this is for elaboration)
extern int throwClauseSerialNumber;


// this visitor is responsible for conducting all the
// elaboration activities
// Intended to be used with LoweredASTVisitor
class ElabVisitor : private ASTVisitor, private SourceLocProvider {
public:      // data
  LoweredASTVisitor loweredVisitor; // use this as the argument for traverse()

  // similar fields to Env
  StringTable &str;
  TypeFactory &tfac;
  TranslationUnit *tunit;      // doh! out damned spot!

  // For synthesizing AST.
  ElsaASTBuild m_astBuild;

  // little trick: as code moves about it's convenient if 'env'
  // always works, even inside ElabVisitor methods
  ElabVisitor &env;            // refers to *this

  // stack of functions, topmost being the one we're in now
  ArrayStack<Function*> functionStack;

  // so that we can find the closest nesting S_compound for when we
  // need to insert temporary variables; its scope should always be
  // the current scope.
  ArrayStack<FullExpressionAnnot*> fullExpressionAnnotStack;

  // location of the most recent Statement.. used for approximate
  // loc info when visiting Expressions
  SourceLoc enclosingStmtLoc;

  // strings
  StringRef receiverName;

  // counters for generating unique temporary names; not unique
  // across translation units
  int tempSerialNumber;
  int e_newSerialNumber;

  // ---------- elaboration parameters -----------
  // These get set to default values by the ctor, but then the client
  // can change them after construction.  I did it this way to avoid
  // making tons of ctor parameters.

  // what we're doing; this defaults to EA_ALL
  ElabActivities activities;

  // When true, we retain cloned versions of subtrees whose semantics
  // is captured (and therefore the tree obviated) by the elaboration.
  // When false, we just nullify those subtrees, which results in
  // sometimes-invalid AST, but makes some analyses happy anway.  This
  // defaults to false.
  bool cloneDefunctChildren;

private:     // methods
  // Implement SourceLocProvider.
  SourceLoc provideLoc() const override;

public:      // funcs
  // true if a particular activity is requested
  bool doing(ElabActivities a) const { return !!(activities & a); }

  // fresh names
  StringRef makeTempName();
  StringRef makeE_newVarName();
  StringRef makeThrowClauseVarName();
  StringRef makeCatchClauseVarName();

  // similar to a function in Env
  Variable *makeVariable(SourceLoc loc, StringRef name,
                         Type *type, DeclFlags dflags);

  // syntactic convenience
  void push(FullExpressionAnnot *a)
    { fullExpressionAnnotStack.push(a); }
  void pop(FullExpressionAnnot *a)
    { FullExpressionAnnot *tmp = fullExpressionAnnotStack.pop(); xassert(a == tmp); }

public:      // funcs
  // This section is organized like the .cc file, but all the comments
  // are in the .cc file, so look there first.

  // AST creation
  IDeclarator *makeInnermostDeclarator(SourceLoc loc, Variable *var);
  Declarator *makeDeclarator(SourceLoc loc, Variable *var, DeclaratorContext context);
  Declaration *makeDeclaration(SourceLoc loc, Variable *var, DeclaratorContext context);
  Function *makeFunction(SourceLoc loc, Variable *var,
                         FakeList<MemberInit> *inits,
                         S_compound *body);
  E_variable *makeE_variable(SourceLoc loc, Variable *var);
  E_fieldAcc *makeE_fieldAcc
    (SourceLoc loc, Expression *obj, Variable *field);
  E_funCall *makeMemberCall
    (SourceLoc loc, Expression *obj, Variable *func, FakeList<ArgExpression> *args);
  FakeList<ArgExpression> *emptyArgs();
  Expression *makeThisRef(SourceLoc loc);
  S_expr *makeS_expr(SourceLoc loc, Expression *e);
  S_compound *makeS_compound(SourceLoc loc);

  // makeCtor
  E_constructor *makeCtorExpr(
    SourceLoc loc,
    Expression *target,
    CVAtomicType *type,
    Variable *ctor,
    FakeList<ArgExpression> *args);
  Statement *makeCtorStatement(
    SourceLoc loc,
    Expression *target,
    CVAtomicType *type,
    Variable *ctor,
    FakeList<ArgExpression> *args);

  // makeDtor
  Expression *makeDtorExpr(SourceLoc loc, Expression *target,
                           Type *type);
  Statement *makeDtorStatement(SourceLoc loc, Expression *target,
                               Type *type);

  // cloning
  FakeList<ArgExpression> *cloneExprList(FakeList<ArgExpression> *args0);
  Expression *cloneExpr(Expression *e);

  // elaborateCDtors
  void elaborateCDtorsDeclaration(Declaration *decl);

  // elaborateCallSite
  Declaration *makeTempDeclaration
    (SourceLoc loc, Type *retType, Variable *&var /*OUT*/, DeclaratorContext context);
  Variable *insertTempDeclaration(SourceLoc loc, Type *retType);
  Expression *elaborateCallByValue
    (SourceLoc loc, CVAtomicType *paramType, Expression *argExpr);
  Expression *elaborateCallSite(
    SourceLoc loc,
    FunctionType *ft,
    FakeList<ArgExpression> *args,
    bool artificalCtor);

  // elaborateFunctionStart
  void elaborateFunctionStart(Function *f);

  // completeNoArgMemberInits
  bool wantsMemberInit(Variable *var);
  MemberInit *findMemberInitDataMember
    (FakeList<MemberInit> *inits,
     Variable *memberVar);
  MemberInit *findMemberInitSuperclass
    (FakeList<MemberInit> *inits,
     CompoundType *superclass);
  void completeNoArgMemberInits(Function *ctor, CompoundType *ct);

  // make compiler-supplied member funcs
  Variable *makeCtorReceiver(SourceLoc loc, CompoundType *ct);
  MR_func *makeNoArgCtorBody(CompoundType *ct, Variable *ctor);
  MemberInit *makeCopyCtorMemberInit(
    Variable *target,
    Variable *srcParam,
    SourceLoc loc);
  MR_func *makeCopyCtorBody(CompoundType *ct, Variable *ctor);
  S_return *make_S_return_this(SourceLoc loc);
  S_expr *make_S_expr_memberCopyAssign
    (SourceLoc loc, Variable *member, Variable *other);
  S_expr *make_S_expr_superclassCopyAssign
    (SourceLoc loc, CompoundType *w, Variable *other);
  MR_func *makeCopyAssignBody
    (SourceLoc loc, CompoundType *ct, Variable *assign);
  S_expr *make_S_expr_memberDtor
    (SourceLoc loc, Expression *member, CompoundType *memberType);
  void completeDtorCalls(
    Function *func,
    CompoundType *ct);
  MR_func *makeDtorBody(CompoundType *ct, Variable *dtor);

  // some special member functions
  Variable *getDefaultCtor(CompoundType *ct);    // C(); might be NULL at any time
  Variable *getCopyCtor(CompoundType *ct);       // C(C const &);
  Variable *getAssignOperator(CompoundType *ct); // C& operator= (C const &);
  Variable *getDtor(CompoundType *ct);           // ~C();

public:
  ElabVisitor(StringTable &str, TypeFactory &tfac,
              CCLang const &lang, TranslationUnit *tunit);
  virtual ~ElabVisitor();

  // ASTVisitor funcs
  bool visitTopForm(TopForm *tf) override;
  bool visitFunction(Function *f) override;
  void postvisitFunction(Function *f) override;
  bool visitMemberInit(MemberInit *mi) override;
  void postvisitMemberInit(MemberInit *mi) override;
  bool visitTypeSpecifier(TypeSpecifier *ts) override;
  bool visitMember(Member *m) override;
  bool visitStatement(Statement *s) override;
  bool visitCondition(Condition *c) override;
  bool visitHandler(Handler *h) override;
  void postvisitHandler(Handler *h) override;
  bool visitExpression(Expression *e) override;
  bool visitFullExpression(FullExpression *fe) override;
  void postvisitFullExpression(FullExpression *fe) override;
  bool visitInitializer(Initializer *in) override;
  void postvisitInitializer(Initializer *in) override;
};


#endif // CC_ELABORATE_H
