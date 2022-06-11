// ast_build.h
// code for ast_build.cc

#include "ast_build.h"                 // this module

#include "cc-lang.h"                   // CCLang


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
                           CCLang const &lang, SourceLocProvider &locProvider)
  : m_stringTable(stringTable),
    m_typeFactory(tfac),
    m_lang(lang),
    m_locProvider(locProvider)
{}


FakeList<ArgExpression> *ElsaASTBuild::makeExprList1(Expression *e)
{
  return FakeList<ArgExpression>::makeList(new ArgExpression(e));
}

FakeList<ArgExpression> *ElsaASTBuild::makeExprList2(Expression *e1, Expression *e2)
{
  return fl_prepend(makeExprList1(e2), new ArgExpression(e1));
}

FakeList<ArgExpression> *ElsaASTBuild::makeExprList3(
  Expression *e1, Expression *e2, Expression *e3)
{
  return fl_prepend(makeExprList2(e2, e3), new ArgExpression(e1));
}

FakeList<ArgExpression> *ElsaASTBuild::makeExprList4(
  Expression *e1, Expression *e2, Expression *e3, Expression *e4)
{
  return fl_prepend(makeExprList3(e2, e3, e4), new ArgExpression(e1));
}


IDeclarator *ElsaASTBuild::makeInnermostDeclarator(Variable *var)
{
  PQName *pqname = makePQName(var);
  if (var->isBitfield()) {
    int sz = var->getBitfieldSize();
    return new D_bitfield(loc(), pqname, makeE_intLit(sz));
  }
  else {
    return new D_name(loc(), pqname);
  }
}


D_func *ElsaASTBuild::makeD_func(FunctionType const *ftype, IDeclarator *base)
{
  // Build up the syntax for each destination parameter.  We will
  // build it in reverse order initially.
  FakeList<ASTTypeId> *destParams = FakeList<ASTTypeId>::emptyList();

  // Iterate over the parameter types in the source.
  SObjListIter<Variable> srcParamIter(ftype->nonReceiverParamIterC());

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

  if (ftype->acceptsVarargs()) {
    // Add the '...' parameter.
    destParams = fl_prepend(destParams, makeSimpleTypeTypeId(ST_ELLIPSIS));
  }

  else if (ftype->params.isEmpty() &&
           !ftype->hasFlag(FF_NO_PARAM_INFO) &&
           m_lang.emptyParamsMeansNoInfo) {
    // Add the single 'void' parameter to say there are no parameters.
    destParams = fl_prepend(destParams, makeSimpleTypeTypeId(ST_VOID));
  }

  // Reverse the constructed parameters.
  destParams = fl_reverse(destParams);

  // Get exception specification syntax.
  ExceptionSpec *exnSpec = makeExceptionSpec(ftype->exnSpec);

  // Package them as D_func.
  return new D_func(loc(), base, destParams,
                    ftype->getReceiverCV(), exnSpec);
}


Type const *ElsaASTBuild::buildUpDeclarator(
  Type const *type, IDeclarator *&idecl)
{
  // Loop until we hit a TypedefType or an atomic type.
  while (!type->isTypedefType() && !type->isCVAtomicType()) {
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
        else {
          // For both unsized and dynamically sized arrays, yield a
          // declarator with no size.  In the latter case, the caller
          // can check for dynamic sizing (which must be at the top
          // level of the Type) and fill in the size by digging down
          // with IDeclarator::getSecondFromBottom() to find the D_array
          // and set its size.
          idecl = new D_array(loc(), idecl, NULL /*size*/);
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

  return type;
}


Type const *ElsaASTBuild::makeLvalueType(Type const *type)
{
  if (type->isReferenceType()) {
    return type;
  }
  else {
    return m_typeFactory.makeReferenceType(legacyTypeNC(type));
  }
}


TS_name *ElsaASTBuild::makeTS_name(Variable *typedefVar)
{
  xassert(typedefVar->isType());
  TS_name *tsn = new TS_name(loc(),
    makePQName(typedefVar), false /*typenameUsed*/);
  tsn->var = typedefVar;
  return tsn;
}


TypeSpecifier *ElsaASTBuild::makeTypeSpecifier(Type const *type)
{
  if (TypedefType const *tt = type->ifTypedefTypeC()) {
    TS_name *tsn = makeTS_name(tt->m_typedefVar);
    tsn->cv = tt->m_cv;
    return tsn;
  }

  CVAtomicType const *atype = type->asCVAtomicTypeC();
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

      if (ntype->name) {
        // If we are making a specifier for a compound or enum directly,
        // then use the elaborated form.  (This assumes the definition
        // appears elsewhere.)  If instead we were to make a specifier
        // for a TypedefType of one of these, then we would make a
        // TS_name.
        TS_elaborated *tse = new TS_elaborated(loc(), ntype->getTypeIntr(),
                                               makePQName(ntype->typedefVar));
        tse->atype = ntype;
        tspec = tse;
      }
      else {
        // For an anonymous type, we must use the definition.
        if (CompoundType const *ct = atype->atomic->ifCompoundTypeC()) {
          tspec = makeTS_classSpec(ct);
        }
        else {
          xunimp("makeTypeSpecifier for anonymous enum");
        }
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


TS_classSpec *ElsaASTBuild::makeTS_classSpec(CompoundType const *ct)
{
  xassert(ct->typedefVar);

  // Construct new members to denote that type.
  MemberList *memberList = new MemberList(NULL /*list*/);
  SFOREACH_OBJLIST(Variable, ct->dataMembers, memberIter) {
    Variable const *varC = memberIter.data();

    // The Declaration wants a non-const pointer, but no modification
    // should happen due to letting it have one.
    Variable *var = const_cast<Variable*>(varC);

    // Make a declaration for the member.
    Declaration *declaration = makeDeclaration(var, DC_MR_DECL);
    Member *member = new MR_decl(var->loc, declaration);

    // Add it.
    memberList->list.append(member);
  }

  if (ct->bases.isNotEmpty()) {
    xunimp("makeTS_classSpec: class with base classes");
  }

  // Package as a TS_classSpec.
  TS_classSpec *classSpec = new TS_classSpec(
    ct->typedefVar->loc,
    ct->getTypeIntr(),
    makePQName(ct->typedefVar),
    FakeList<BaseClassSpec>::emptyList(),
    memberList
  );
  classSpec->ctype = legacyTypeNC(ct);

  return classSpec;
}


std::pair<TypeSpecifier*, Declarator*>
  ElsaASTBuild::makeTSandDeclarator(Variable *var, DeclaratorContext context)
{
  return makeTSandDeclaratorForType(var, var->type, context);
}

std::pair<TypeSpecifier*, Declarator*>
  ElsaASTBuild::makeTSandDeclaratorForType(Variable *var, Type const *type,
                                           DeclaratorContext context)
{
  // Inner declarator for 'var' that, together with the type specifier,
  // denotes 'type'.
  IDeclarator *idecl = makeInnermostDeclarator(var);

  // Add type constructors on top of 'idecl'.
  Type const *atype = buildUpDeclarator(type, idecl /*INOUT*/);

  // Express what remains as a type specifier.
  TypeSpecifier *tspec = makeTypeSpecifier(atype);

  // Stack a Declarator on 'idecl', and annotate it with the given 'var'
  // so the declaration as a whole is seen as declaring that specific
  // Variable, not just one with the same name and type.
  Declarator *declarator = new Declarator(idecl, NULL /*init*/);
  declarator->var = var;
  declarator->type = legacyTypeNC(type);
  declarator->context = context;

  return std::make_pair(tspec, declarator);
}


ASTTypeId *ElsaASTBuild::makeASTTypeId(Type const *type, PQName *name,
  DeclaratorContext context)
{
  // Construct a Variable out of 'type' and 'name'.
  StringRef nameSR = name? name->getName() : NULL;
  Variable *var = m_typeFactory.makeVariable(loc(), nameSR,
    legacyTypeNC(type), context==DC_D_FUNC? DF_PARAMETER : DF_NONE);

  // Build a type specifier and declarator to represent 'type'.
  std::pair<TypeSpecifier*, Declarator*> ts_and_decl =
    makeTSandDeclarator(var, context);

  // Package those as an ASTTypeId.
  return new ASTTypeId(ts_and_decl.first, ts_and_decl.second);
}


ASTTypeId *ElsaASTBuild::makeSimpleTypeTypeId(SimpleTypeId tid)
{
  return makeASTTypeId(
    m_typeFactory.getSimpleType(tid),
    NULL /*name*/,
    DC_D_FUNC);
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


PQName *ElsaASTBuild::makePQName(Variable *var)
{
  if (var->name == NULL) {
    // Abstract declarator is represented by a NULL pointer.
    return NULL;
  }

  return new PQ_variable(loc(), var);

  // What follows has been tested in the non-template cases and is
  // semi-satisfactory.  The template cases are incomplete and do not
  // compile.  I'm leaving this here in case I decide to try again to
  // construct syntactic PQNames.
#if 0
  // Build the innermost element of a possibly-qualified name.
  PQName *ret = NULL;
  TemplateInfo *templateInfo = var->templateInfo();
  if (templateInfo) {
    // Build 'destArgs' from 'templateInfo->arguments'.
    TemplateArgument *destArgs = NULL;
    FOREACH_OBJLIST(STemplateArgument, templateInfo->arguments, iter) {
      STemplateArgument *srcArg = iter.data();
      if (srcArg->isType()) {
        destArgs = new TA_type(makeASTTypeId(srcArg->getType()), destArgs);
      }
      else {
        destArgs = new TA_nontype(makeExpressionFromSTA(srcArg), destArgs);
      }
    }

    // Reverse 'destArgs'.
    if (destArgs) {
      // 'TemplateArgument' satisfies the 'FakeList' constraints, so
      // we will use 'fl_reverse'.
      FakeList<TemplateArgument> *list =
        FakeList<TemplateArgument>::makeList(destArgs);
      list = fl_reverse(list);
      destArgs = fl_first(lit);
    }

    ret = new PQ_template(var->name, destArgs);
  }
  else {
    ret = new PQ_name(loc(), var->name)
  }

  // TODO: The way operators are recorded as Variables is they simply
  // have particular names, such as "operator+".  Therefore to recognize
  // an operator Variable, I need to examine the name (as a string) to
  // see if it is one of the special names.  It would be preferable to
  // have a more robust scheme like a field in Variable.  For now I'm
  // going to ignore this problem and just use PQ_name.

  while (var->m_containingScope) {
    Scope *scope = var->m_containingScope;
    if (scope->isGlobalScope()) {
      // For now I'm going to not add the global scope qualifier.  Doing
      // so would add significant clutter, and would make the result not
      // usable with C.
      return ret;
    }

    var = scope->namespaceVar;
    if (!var) {
      // TODO: This happens for a structure defined inside a function,
      // for example test/pqname/pqname1.c.  This seems like it violates
      // the stated constraints for 'm_containingScope'.  For the moment
      // I'll bypass it here.
      return ret;
    }

    StringRef qualifier = var->name;
    if (!qualifier) {
      // Happens for anonymous namespaces.  No qualification is
      // possible, but the unqualified name should be in scope, so no
      // qualifier should be required (unless there is a collision,
      // which I'm not dealing with anyway).
      return ret;
    }

    TemplateInfo *templateInfo = var->templateInfo();
    if (templateInfo) {
      xunimp("makePQName of a variable whose scope has template info");
    }

    ret = new PQ_qualifier(loc(), qualifier, NULL /*templArgs*/, ret);
  }

  return ret;
#endif // 0
}


E_intLit *ElsaASTBuild::makeE_intLit(int n)
{
  E_intLit *ret = new E_intLit(m_stringTable.add(stringb(n).c_str()));

  // TODO: This is not really safe, but will suffice for my immediate purpose.
  ret->i = (unsigned long)n;

  ret->type = m_typeFactory.getSimpleType(ST_INT);

  return ret;
}


E_stringLit *ElsaASTBuild::makeE_stringLit(char const *text)
{
  // This is kind of ugly; I have to turn 'text' into C syntax because
  // my AST representation keeps it after parsing, and the
  // pretty-printer in turn relies on that...
  string s = quoted(text);

  StringRef sref = m_stringTable.add(s);
  E_stringLit *ret = new E_stringLit(sref);

  // This is the result of interpreting the syntax.
  ret->m_stringData = DataBlock(text);

  return ret;
}


E_variable *ElsaASTBuild::makeE_variable(Variable *var)
{
  E_variable *evar = new E_variable(makePQName(var));
  evar->var = var;
  evar->type = var->type;
  return evar;
}


E_funCall *ElsaASTBuild::makeE_funCall(
  Expression *func, FakeList<ArgExpression> *args)
{
  E_funCall *call = new E_funCall(func, args);
  call->type = func->type->asRval()->asFunctionType()->retType;
  return call;
}


E_funCall *ElsaASTBuild::makeNamedFunCall0(
  Variable *callee)
{
  return makeE_funCall(makeE_variable(callee), NULL /*args*/);
}


E_funCall *ElsaASTBuild::makeNamedFunCall1(
  Variable *callee, Expression *arg1)
{
  FakeList<ArgExpression> *args = makeExprList1(arg1);
  return makeE_funCall(makeE_variable(callee), args);
}


E_funCall *ElsaASTBuild::makeNamedFunCall2(
  Variable *callee, Expression *arg1, Expression *arg2)
{
  FakeList<ArgExpression> *args = makeExprList2(arg1, arg2);
  return makeE_funCall(makeE_variable(callee), args);
}


E_funCall *ElsaASTBuild::makeNamedFunCall3(
  Variable *callee, Expression *arg1, Expression *arg2, Expression *arg3)
{
  FakeList<ArgExpression> *args = makeExprList3(arg1, arg2, arg3);
  return makeE_funCall(makeE_variable(callee), args);
}


E_funCall *ElsaASTBuild::makeNamedFunCall4(
  Variable *callee, Expression *arg1, Expression *arg2, Expression *arg3, Expression *arg4)
{
  FakeList<ArgExpression> *args = makeExprList4(arg1, arg2, arg3, arg4);
  return makeE_funCall(makeE_variable(callee), args);
}


E_fieldAcc *ElsaASTBuild::makeE_fieldAcc(Expression *obj, Variable *field)
{
  E_fieldAcc *fa = new E_fieldAcc(obj, makePQName(field));
  fa->field = field;
  fa->type = legacyTypeNC(makeLvalueType(field->type));
  return fa;
}


E_sizeof *ElsaASTBuild::makeE_sizeof(Expression *expr)
{
  E_sizeof *szof = new E_sizeof(expr);
  szof->type = m_typeFactory.getSimpleType(
    m_lang.m_typeSizes.m_type_of_size_t);
  return szof;
}


E_unary *ElsaASTBuild::makeE_unary(UnaryOp op, Expression *expr)
{
  E_unary *un = new E_unary(op, expr);

  // TODO: The rules for this are more complicated.
  un->type = expr->type;

  return un;
}


E_effect *ElsaASTBuild::makeE_effect(
  EffectOp op, Expression *expr)
{
  E_effect *eff = new E_effect(op, expr);

  switch (op) {
    default:
      xfailure("bad effect op");

    case EFF_POSTINC:
    case EFF_POSTDEC:
      eff->type = expr->type->asRval();
      break;

    case EFF_PREINC:
    case EFF_PREDEC:
      eff->type = expr->type;
      break;
  }

  return eff;
}


Type const *ElsaASTBuild::typeForE_binary(
  Expression *e1, BinaryOp op, Expression *e2)
{
  // The code here is similar to computation in E_binary::itcheck_x.  It
  // is not ideal to repeat this, but it's not obvious how to factor the
  // commonality without making a mess.

  // For most operators, we do lvalue to rvalue conversion.
  Type const *lhsType = e1->type->asRval();
  Type const *rhsType = e2->type->asRval();

  switch (op) {
    default:
      xunimp("typeForE_binary: unimplemented operator");

    case BIN_EQUAL:
    case BIN_NOTEQUAL:
    case BIN_LESS:
    case BIN_GREATER:
    case BIN_LESSEQ:
    case BIN_GREATEREQ:
    case BIN_AND:
    case BIN_OR:
      return m_typeFactory.getSimpleType(
               getBooleanOperatorResultSimpleTypeId(m_lang));

    case BIN_PLUS:
      // case: p + n
      if (lhsType->isPointerType()) {
        return lhsType;
      }

      // case: n + p
      if (lhsType->isIntegerType() && rhsType->isPointerType()) {
        return rhsType;
      }

      return usualArithmeticConversions(m_typeFactory,
        legacyTypeNC(lhsType), legacyTypeNC(rhsType));

    case BIN_MINUS:
      // case: p - p
      if (lhsType->isPointerType() && rhsType->isPointerType()) {
        // TODO: This is wrong.  I need to add ptrdiff_t to TypeSizes.
        return m_typeFactory.getSimpleType(ST_INT);
      }

      // case: p - n
      if (lhsType->isPointerType()) {
        return lhsType;
      }

      return usualArithmeticConversions(m_typeFactory,
        legacyTypeNC(lhsType), legacyTypeNC(rhsType));

    case BIN_MULT:
    case BIN_DIV:
    case BIN_MOD:
      return usualArithmeticConversions(m_typeFactory,
        legacyTypeNC(lhsType), legacyTypeNC(rhsType));

    case BIN_LSHIFT:
    case BIN_RSHIFT:
    case BIN_BITAND:
    case BIN_BITXOR:
    case BIN_BITOR:
    case BIN_MINIMUM:
    case BIN_MAXIMUM:
      // I'll assume this is good enough.
      return lhsType;

    case BIN_COMMA:
      if (m_lang.isCplusplus) {
        // In C++, "the result is of the same value category as its
        // right operand" (C++14 5.19p1), where "value category" is
        // defined in C++14 3.10p1 and includes things like "lvalue".
        return e2->type;
      }
      else {
        // In C, a comma expression is not an lvalue (C11 6.5.17p2,
        // footnote 114).
        return rhsType;
      }

    case BIN_BRACKETS:
      if (ArrayType const *at = lhsType->ifArrayTypeC()) {
        return m_typeFactory.makeReferenceType(at->eltType);
      }
      else {
        PointerType const *pt = lhsType->asPointerTypeC();
        return m_typeFactory.makeReferenceType(pt->atType);
      }
  }
}


E_binary *ElsaASTBuild::makeE_binary(
  Expression *e1, BinaryOp op, Expression *e2)
{
  E_binary *bin = new E_binary(e1, op, e2);
  bin->type = legacyTypeNC(typeForE_binary(e1, op, e2));
  return bin;
}


E_binary *ElsaASTBuild::makeCommaExpr(
  Expression *e1,
  Expression *e2)
{
  return makeE_binary(e1, BIN_COMMA, e2);
}


E_binary *ElsaASTBuild::makeCommaExpr3(
  Expression *e1,
  Expression *e2,
  Expression *e3)
{
  return makeCommaExpr(makeCommaExpr(e1, e2), e3);
}


E_binary *ElsaASTBuild::makeCommaExpr4(
  Expression *e1,
  Expression *e2,
  Expression *e3,
  Expression *e4)
{
  return makeCommaExpr(makeCommaExpr3(e1, e2, e3), e4);
}


E_addrOf *ElsaASTBuild::makeE_addrOf(Expression *expr)
{
  E_addrOf *ao = new E_addrOf(expr);
  ao->type = m_typeFactory.makePointerType(CV_NONE, expr->type->asRval());
  return ao;
}


E_deref *ElsaASTBuild::makeE_deref(Expression *ptr)
{
  Type const *atType = ptr->type->asRval()->asPointerType()->atType;
  E_deref *deref = new E_deref(ptr);

  // Dereferencing produces an lvalue.
  deref->type = legacyTypeNC(makeLvalueType(atType));

  return deref;
}


E_cast *ElsaASTBuild::makeE_cast(Type const *type, Expression *src)
{
  E_cast *cast = new E_cast(
    makeASTTypeId(type, NULL /*name*/, DC_E_CAST),
    src);
  cast->type = legacyTypeNC(type);
  return cast;
}


E_cond *ElsaASTBuild::makeE_cond(
  Expression *cond, Expression *th, Expression *el)
{
  E_cond *ret = new E_cond(cond, th, el);

  // TODO: Rules are more complicated here.
  ret->type = th->type;

  return ret;
}


E_assign *ElsaASTBuild::makeE_assign(
  Expression *target, BinaryOp op, Expression *src)
{
  E_assign *assign = new E_assign(target, op, src);
  assign->type = target->type;
  return assign;
}


E_assign *ElsaASTBuild::makeVarAssign(Variable *target, Expression *src)
{
  return makeE_assign(makeE_variable(target), BIN_ASSIGN, src);
}


E_sizeofType *ElsaASTBuild::makeE_sizeofType(Type const *type)
{
  ASTTypeId *typeId =
    makeASTTypeId(type, NULL /*name*/, DC_E_SIZEOFTYPE);
  E_sizeofType *st = new E_sizeofType(typeId);
  st->type = m_typeFactory.getSimpleType(
    m_lang.m_typeSizes.m_type_of_size_t);
  return st;
}


E_offsetof *ElsaASTBuild::makeE_offsetof(Type const *structType, Variable *field)
{
  ASTTypeId *typeId =
    makeASTTypeId(structType, NULL /*name*/, DC_E_OFFSETOF);
  E_offsetof *eo = new E_offsetof(typeId, makePQName(field));
  eo->type = m_typeFactory.getSimpleType(
    m_lang.m_typeSizes.m_type_of_size_t);
  eo->field = field;
  return eo;
}


#ifdef GNU_EXTENSION
E_compoundLit *ElsaASTBuild::makeE_compoundLit(ASTTypeId *typeId,
  IN_compound *init)
{
  E_compoundLit *ec = new E_compoundLit(typeId, init);
  ec->type = typeId->getType();
  return ec;
}

E_compoundLit *ElsaASTBuild::makeE_compoundLit(Type const *type,
  IN_compound *init)
{
  ASTTypeId *tid = makeASTTypeId(type, NULL /*name*/, DC_E_COMPOUNDLIT);
  return makeE_compoundLit(tid, init);
}


E___builtin_va_arg *ElsaASTBuild::makeE___builtin_va_arg(SourceLoc loc,
  Expression *expr, ASTTypeId *atype)
{
  E___builtin_va_arg *vaa = new E___builtin_va_arg(loc, expr, atype);
  vaa->type = atype->getType();
  return vaa;
}
#endif // GNU_EXTENSION


S_expr *ElsaASTBuild::makeS_expr(SourceLoc loc, Expression *expr)
{
  return new S_expr(loc, new FullExpression(expr));
}


// EOF
