// ast_build.h
// code for ast_build.cc

#include "ast_build.h"      // this module


// ----------------------- SourceLocProvider ---------------------------
SourceLoc SourceLocProvider::provideLoc() const
{
  return SL_UNKNOWN;
}


// -------------------- MemberSourceLocProvider ------------------------
MemberSourceLocProvider::MemberSourceLocProvider(SourceLoc locForProvider)
  : SourceLocProvider(),
    m_locForProvider(locForProvider)
{}


SourceLoc MemberSourceLocProvider::provideLoc() const
{
  return m_locForProvider;
}


// -------------------------- ElsaASTBuild -----------------------------
ElsaASTBuild::ElsaASTBuild(StringTable &stringTable, TypeFactory &tfac,
                           SourceLocProvider &locProvider)
  : m_stringTable(stringTable),
    m_typeFactory(tfac),
    m_locProvider(locProvider),
    m_useTypedefsForNamedAtomics(true)
{}


FakeList<ArgExpression> *ElsaASTBuild::makeExprList1(Expression *e)
{
  return FakeList<ArgExpression>::makeList(new ArgExpression(e));
}

FakeList<ArgExpression> *ElsaASTBuild::makeExprList2(Expression *e1, Expression *e2)
{
  return fl_prepend(makeExprList1(e2), new ArgExpression(e1));
}


D_name *ElsaASTBuild::makeD_name(Variable *var)
{
  return new D_name(loc(), makePQName(var));
}


D_func *ElsaASTBuild::makeD_func(FunctionType const *ftype, IDeclarator *base)
{
  // Build up the syntax for each destination parameter.  We will
  // build it in reverse order initially.
  FakeList<ASTTypeId> *destParams = FakeList<ASTTypeId>::emptyList();

  // Iterate over the parameter types in the source.
  SObjListIter<Variable> srcParamIter(ftype->params);

  // Skip the receiver parameter.
  if (ftype->isMethod()) {
    xassert(!srcParamIter.isDone());
    srcParamIter.adv();
  }

  for (; !srcParamIter.isDone(); srcParamIter.adv()) {
    Variable const *srcParamVar = srcParamIter.data();

    // Make a new Variable for this parameter.  At the moment, my
    // only motivation for cloning rather than reusing is it means
    // 'ftype' can continue to be 'const'.
    Variable *destParamVar = m_typeFactory.makeVariable(
      loc(), srcParamVar->name, srcParamVar->type, srcParamVar->flags);

    // Recursively translate the parameter type to its syntax.
    ASTTypeId *destParam = makeParameter(destParamVar);
    destParams = fl_prepend(destParams, destParam);
  }

  // Reverse the constructed parameters.
  destParams = fl_reverse(destParams);

  // Get exception specification syntax.
  ExceptionSpec *exnSpec = makeExceptionSpec(ftype->exnSpec);

  // Package them as D_func.
  return new D_func(loc(), base, destParams,
                    ftype->getReceiverCV(), exnSpec);
}


CVAtomicType const *ElsaASTBuild::buildUpDeclarator(
  Type const *type, IDeclarator *&idecl)
{
  // Loop until we hit an atomic type.
  while (!type->isCVAtomicType()) {
    switch (type->getTag()) {
      default:
        xfailure("bad tag");

      case Type::T_POINTER: {
        PointerType const *ptype = type->asPointerTypeC();
        idecl = new D_pointer(loc(), ptype->cv, idecl);
        type = ptype->atType;
        break;
      }

      case Type::T_REFERENCE: {
        ReferenceType const *rtype = type->asReferenceTypeC();
        idecl = new D_reference(loc(), idecl);
        type = rtype->atType;
        break;
      }

      case Type::T_FUNCTION: {
        FunctionType const *ftype = type->asFunctionTypeC();
        idecl = makeD_func(ftype, idecl);
        type = ftype->retType;
        break;
      }

      case Type::T_ARRAY: {
        ArrayType const *atype = type->asArrayTypeC();

        if (atype->size >= 0) {
          idecl = new D_array(loc(), idecl, makeE_intLit(atype->size));
        }
        else if (atype->size == ArrayType::NO_SIZE) {
          idecl = new D_array(loc(), idecl, NULL /*size*/);
        }
        else {
          xunimp("array with dynamic size");
        }

        type = atype->eltType;
        break;
      }

      case Type::T_POINTERTOMEMBER: {
        PointerToMemberType const *ptmtype = type->asPointerToMemberTypeC();

        idecl = new D_ptrToMember(loc(),
          makePQName(ptmtype->inClassNAT->typedefVar),
          ptmtype->cv, idecl);

        type = ptmtype->atType;
        break;
      }
    } // switch (tag)
  } // while (!atomic)

  // Loop terminates when 'type' is atomic.
  return type->asCVAtomicTypeC();
}


TypeSpecifier *ElsaASTBuild::makeTypeSpecifier(CVAtomicType const *atype)
{
  TypeSpecifier *tspec = NULL;
  switch (atype->atomic->getTag()) {
    case AtomicType::T_SIMPLE: {
      SimpleType const *stype = atype->atomic->asSimpleTypeC();
      tspec = new TS_simple(loc(), stype->type);
      break;
    }

    case AtomicType::T_COMPOUND:
    case AtomicType::T_ENUM: {
      NamedAtomicType *ntype = atype->atomic->asNamedAtomicType();

      // Whenever possible, I want to use the typedefName.  However, I
      // can't tell here whether typedefName is in fact accessible...
      //
      // TODO: Straighten this out.
      if (m_useTypedefsForNamedAtomics) {
        TS_name *tsn = new TS_name(loc(),
          makePQName(ntype->typedefVar), false /*typenameUsed*/);
        tsn->var = ntype->typedefVar;
        tspec = tsn;
      }
      else {
        TS_elaborated *tse = new TS_elaborated(loc(), ntype->getTypeIntr(),
                                               makePQName(ntype->typedefVar));
        tse->atype = ntype;
        tspec = tse;
      }

      break;
    }

    default:
      // The template types get here.  I'm not ready to do them yet.
      xunimp(stringb("makeASTTypeId for: " << atype->toString()));
  }

  tspec->cv = atype->cv;
  return tspec;
}


std::pair<TypeSpecifier*, Declarator*>
  ElsaASTBuild::makeTSandDeclarator(Variable *var, DeclaratorContext context)
{
  // Inner declarator for 'var' that, together with the type specifier,
  // denotes 'var->type'.
  IDeclarator *idecl = makeD_name(var);

  // Add type constructors on top of 'idecl'.
  CVAtomicType const *atype = buildUpDeclarator(var->type, idecl /*INOUT*/);

  // Express the atomic type as a type specifier.
  TypeSpecifier *tspec = makeTypeSpecifier(atype);

  // Stack a Declarator on 'idecl', and annotate it with the given 'var'
  // so the declaration as a whole is seen as declaring that specific
  // Variable, not just one with the same name and type.
  Declarator *declarator = new Declarator(idecl, NULL /*init*/);
  declarator->var = var;
  declarator->type = var->type;
  declarator->context = context;

  return std::make_pair(tspec, declarator);
}


ASTTypeId *ElsaASTBuild::makeASTTypeId(Type *type, PQName *name,
  DeclaratorContext context)
{
  // Construct a Variable out of 'type' and 'name'.
  StringRef nameSR = name? name->getName() : NULL;
  Variable *var = m_typeFactory.makeVariable(loc(), nameSR, type, DF_NONE);

  // Build a type specifier and declarator to represent 'type'.
  std::pair<TypeSpecifier*, Declarator*> ts_and_decl =
    makeTSandDeclarator(var, context);

  // Package those as an ASTTypeId.
  return new ASTTypeId(ts_and_decl.first, ts_and_decl.second);
}


ASTTypeId *ElsaASTBuild::makeParameter(Variable *var)
{
  // Build a type specifier and declarator to represent 'var->type'.
  std::pair<TypeSpecifier*, Declarator*> ts_and_decl =
    makeTSandDeclarator(var, DC_D_FUNC);

  // Package those as an ASTTypeId.
  return new ASTTypeId(ts_and_decl.first, ts_and_decl.second);
}


Declaration *ElsaASTBuild::makeDeclaration(
  Variable *var, DeclaratorContext context)
{
  // Make a specifier and declarator.
  std::pair<TypeSpecifier*, Declarator*> ts_and_decl =
    makeTSandDeclarator(var, context);

  // Wrap them in a declaration.
  Declaration *declaration =
    new Declaration(var->flags & DF_SOURCEFLAGS,
      ts_and_decl.first,
      FakeList<Declarator>::makeList(ts_and_decl.second));

  return declaration;
}


ExceptionSpec *ElsaASTBuild::makeExceptionSpec(FunctionType::ExnSpec *srcSpec)
{
  if (!srcSpec) {
    return NULL;
  }

  // Translate the Types to ASTTypeIds, building the list in reverse.
  FakeList<ASTTypeId> *destTypes = FakeList<ASTTypeId>::emptyList();
  SFOREACH_OBJLIST_NC(Type, srcSpec->types, srcIter) {
    Type *type = srcIter.data();
    ASTTypeId *tid = makeASTTypeId(type, NULL /*name*/, DC_EXCEPTIONSPEC);
    destTypes = fl_prepend(destTypes, tid);
  }

  // Fix the reversal.
  destTypes = fl_reverse(destTypes);

  return new ExceptionSpec(destTypes);
}


E_intLit *ElsaASTBuild::makeE_intLit(int n)
{
  E_intLit *ret = new E_intLit(m_stringTable.add(stringb(n).c_str()));

  // TODO: This is not really safe, but will suffice for my immediate purpose.
  ret->i = (unsigned long)n;

  ret->type = m_typeFactory.getSimpleType(ST_INT);

  return ret;
}


PQName *ElsaASTBuild::makePQName(Variable *var)
{
  if (var->name == NULL) {
    // Abstract declarator is represented by a NULL pointer.
    return NULL;
  }

  // TODO: This is only right when the name is unqualified.
  //
  // Now I'm sort of grasping at straws.  Notably, I see cases where
  // 'var->scope' is set to the global scope.  Anyway, the goal is to
  // get rid of PQ_variable so I'll push forward.
  if (var->m_containingScope) {
    return new PQ_variable(loc(), var);
  }
  else {
    return new PQ_name(loc(), var->name);
  }
}


E_funCall *ElsaASTBuild::makeNamedFunCall2(
  Variable *callee, Expression *arg1, Expression *arg2)
{
  FakeList<ArgExpression> *args = makeExprList2(arg1, arg2);
  E_funCall *call = new E_funCall(
    makeE_variable(callee),
    args);
  call->type = callee->type->asRval()->asFunctionType()->retType;
  return call;
}


E_binary *ElsaASTBuild::makeE_binary(
  Expression *e1, BinaryOp op, Expression *e2)
{
  return new E_binary(e1, op, e2);
}


E_variable *ElsaASTBuild::makeE_variable(Variable *var)
{
  E_variable *evar = new E_variable(makePQName(var));
  evar->var = var;
  evar->type = var->type;
  return evar;
}


// EOF
