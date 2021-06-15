// ast_build.h
// code for ast_build.cc

#include "ast_build.h"      // this module


SourceLoc SourceLocProvider::provideLoc() const
{
  return SL_UNKNOWN;
}


ElsaASTBuild::ElsaASTBuild(StringTable &stringTable, TypeFactory &tfac,
                           SourceLocProvider &locProvider)
  : m_stringTable(stringTable),
    m_typeFactory(tfac),
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


ASTTypeId *ElsaASTBuild::makeASTTypeId(Type *type, PQName *name)
{
  // Inner declarator that, together with the type specifier built at
  // the end, denotes 'type'.  This gets built up as we examine 'type'.
  IDeclarator *idecl = new D_name(loc(), name);

  // Loop until we hit an atomic type.
  while (!type->isCVAtomicType()) {
    // Currently, my declarator printing code relies on the presence of
    // D_grouping to know where to insert parens.  If the inner
    // declarator is a function or array, then we need them since those
    // use postfix rather than prefix syntax (although this means we
    // will add parens in some cases we don't need them, like an array
    // of arrays).
    //
    // TODO: Change the printing code to make this unnecessary.
    if (idecl->isD_func() || idecl->isD_array()) {
      idecl = new D_grouping(loc(), idecl);
    }

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
          // Recursively translate the parameter type to its syntax.
          ASTTypeId *destParam = makeASTTypeId(srcParamIter.data()->type);
          destParams = fl_prepend(destParams, destParam);
        }

        // Reverse the constructed parameters.
        destParams = fl_reverse(destParams);

        // Get exception specification syntax.
        ExceptionSpec *exnSpec = makeExceptionSpec(ftype->exnSpec);

        // Package them as D_func.
        idecl = new D_func(loc(), idecl, destParams,
                           ftype->getReceiverCV(), exnSpec);

        // Continue with the return type.
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
  CVAtomicType const *atype = type->asCVAtomicTypeC();

  // Express the atomic type as a type specifier.
  TypeSpecifier *tspec = NULL;
  switch (atype->atomic->getTag()) {
    case AtomicType::T_SIMPLE: {
      SimpleType const *stype = atype->atomic->asSimpleTypeC();
      tspec = new TS_simple(loc(), stype->type);
      break;
    }

    case AtomicType::T_COMPOUND:
    case AtomicType::T_ENUM: {
      NamedAtomicType const *ntype = atype->atomic->asNamedAtomicTypeC();
      tspec = new TS_elaborated(loc(), ntype->getTypeIntr(),
                                makePQName(ntype->typedefVar));
      break;
    }

    default:
      // The template types get here.  I'm not ready to do them yet.
      xunimp(stringb("makeASTTypeId for: " << type->toString()));
  }
  tspec->cv = atype->cv;

  // Wrap up 'tspec' and 'idecl' in an ASTTypeId.
  Declarator *decl = new Declarator(idecl, NULL /*init*/);
  decl->type = type;
  return new ASTTypeId(tspec, decl);
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
    destTypes = fl_prepend(destTypes, makeASTTypeId(type));
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
  return new PQ_name(loc(), var->name);
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
