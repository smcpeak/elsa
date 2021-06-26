// cc-type-visitor.cc
// Code for cc-type-visitor.h.

#include "cc-type-visitor.h"           // this module

// elsa
#include "cc-ast.h"                    // Expression
#include "cc-scope.h"                  // Scope
#include "cc-type.h"                   // Type, etc.
#include "template.h"                  // STemplateArgument, etc.
#include "variable.h"                  // Variable


bool TypeVisitor::visitType(Type *obj)
  { return true; }
void TypeVisitor::postvisitType(Type *obj)
  {  }

bool TypeVisitor::visitFunctionType_params(SObjList<Variable> &params)
  { return true; }
void TypeVisitor::postvisitFunctionType_params(SObjList<Variable> &params)
  { }
bool TypeVisitor::visitFunctionType_params_item(Variable *param)
  { return true; }
void TypeVisitor::postvisitFunctionType_params_item(Variable *param)
  { }

bool TypeVisitor::visitVariable(Variable *var)
  { return true; }
void TypeVisitor::postvisitVariable(Variable *var)
  { }

bool TypeVisitor::visitAtomicType(AtomicType *obj)
  { return true; }
void TypeVisitor::postvisitAtomicType(AtomicType *obj)
  {  }

bool TypeVisitor::visitEnumType_Value(EnumType::Value *obj)
  { return true; }
void TypeVisitor::postvisitEnumType_Value(EnumType::Value *obj)
  {  }

bool TypeVisitor::visitScope(Scope *obj)
  { return true; }
void TypeVisitor::postvisitScope(Scope *obj)
  {  }

bool TypeVisitor::visitScope_variables(StringRefMap<Variable> &variables)
  { return true; }
void TypeVisitor::postvisitScope_variables(StringRefMap<Variable> &variables)
  {  }
bool TypeVisitor::visitScope_variables_entry(StringRef name, Variable *var)
  { return true; }
void TypeVisitor::postvisitScope_variables_entry(StringRef name, Variable *var)
  {  }

bool TypeVisitor::visitScope_typeTags(StringRefMap<Variable> &typeTags)
  { return true; }
void TypeVisitor::postvisitScope_typeTags(StringRefMap<Variable> &typeTags)
  {  }
bool TypeVisitor::visitScope_typeTags_entry(StringRef name, Variable *var)
  { return true; }
void TypeVisitor::postvisitScope_typeTags_entry(StringRef name, Variable *var)
  {  }

bool TypeVisitor::visitScope_templateParams(SObjList<Variable> &templateParams)
  { return true; }
void TypeVisitor::postvisitScope_templateParams(SObjList<Variable> &templateParams)
  {  }
bool TypeVisitor::visitScope_templateParams_item(Variable *var)
  { return true; }
void TypeVisitor::postvisitScope_templateParams_item(Variable *var)
  {  }

bool TypeVisitor::visitBaseClass(BaseClass *bc)
  { return true; }
void TypeVisitor::postvisitBaseClass(BaseClass *bc)
  {  }

bool TypeVisitor::visitBaseClassSubobj(BaseClassSubobj *bc)
  { return true; }
void TypeVisitor::postvisitBaseClassSubobj(BaseClassSubobj *bc)
  {  }

bool TypeVisitor::visitBaseClassSubobj_parents(SObjList<BaseClassSubobj> &parents)
  { return true; }
void TypeVisitor::postvisitBaseClassSubobj_parents(SObjList<BaseClassSubobj> &parents)
  {  }
bool TypeVisitor::visitBaseClassSubobj_parents_item(BaseClassSubobj *parent)
  { return true; }
void TypeVisitor::postvisitBaseClassSubobj_parents_item(BaseClassSubobj *parent)
  {  }

bool TypeVisitor::visitSTemplateArgument(STemplateArgument *obj)
  { return true; }
void TypeVisitor::postvisitSTemplateArgument(STemplateArgument *obj)
  {  }

bool TypeVisitor::visitPseudoInstantiation_args(ObjList<STemplateArgument> &args)
  { return true; }
void TypeVisitor::postvisitPseudoInstantiation_args(ObjList<STemplateArgument> &args)
  {  }
bool TypeVisitor::visitPseudoInstantiation_args_item(STemplateArgument *arg)
  { return true; }
void TypeVisitor::postvisitPseudoInstantiation_args_item(STemplateArgument *arg)
  {  }

bool TypeVisitor::visitDependentQTypePQTArgsList(ObjList<STemplateArgument> &list)
  { return true; }
void TypeVisitor::postvisitDependentQTypePQTArgsList(ObjList<STemplateArgument> &list)
  {  }
bool TypeVisitor::visitDependentQTypePQTArgsList_item(STemplateArgument *sta)
  { return true; }
void TypeVisitor::postvisitDependentQTypePQTArgsList_item(STemplateArgument *sta)
  {  }

bool TypeVisitor::visitExpression(Expression *obj)
  {
    // if this ever fires, at the same location be sure to put a
    // postvisitExpression()
    xfailure("wanted to find out if this is ever called; I can't find it if it is");
    return true;
  }
void TypeVisitor::postvisitExpression(Expression *obj)
  {  }



// ------------------- cc-type.h 'traverse' methods --------------------
void SimpleType::traverse(TypeVisitor &vis)
{
  if (!vis.visitAtomicType(this)) {
    return;
  }
  vis.postvisitAtomicType(this);
}


void BaseClass::traverse(TypeVisitor &vis)
{
  if (!vis.visitBaseClass(this)) {
    return;
  }
  ct->traverse(vis);
  vis.postvisitBaseClass(this);
}


void BaseClassSubobj::traverse(TypeVisitor &vis)
{
  if (!vis.visitBaseClassSubobj(this)) {
    return;
  }

  if (vis.visitBaseClassSubobj_parents(parents)) {
    SFOREACH_OBJLIST_NC(BaseClassSubobj, parents, iter) {
      BaseClassSubobj *parent = iter.data();
      if (vis.visitBaseClassSubobj_parents_item(parent)) {
        parent->traverse(vis);
        vis.postvisitBaseClassSubobj_parents_item(parent);
      }
    }
    vis.postvisitBaseClassSubobj_parents(parents);
  }

  vis.postvisitBaseClassSubobj(this);
}


void CompoundType::traverse(TypeVisitor &vis)
{
  if (!vis.visitAtomicType(this)) {
    return;
  }

  // traverse the superclass
  Scope::traverse_internal(vis);

  // 2005-07-28: Disabled because I don't remember why I wanted
  // it and it is a little weird (why not traverse the params too?).
  //if (isTemplate()) {
  //  templateInfo()->traverseArguments(vis);
  //}

  vis.postvisitAtomicType(this);
}


void EnumType::traverse(TypeVisitor &vis)
{
  if (!vis.visitAtomicType(this)) {
    return;
  }
  vis.postvisitAtomicType(this);
}


void CVAtomicType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  atomic->traverse(vis);

  vis.postvisitType(this);
}


void PointerType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  atType->traverse(vis);

  vis.postvisitType(this);
}


void ReferenceType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  atType->traverse(vis);

  vis.postvisitType(this);
}


void FunctionType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  retType->traverse(vis);

  // dsw: I need to know when the params list starts and stops
  if (vis.visitFunctionType_params(params)) {
    SFOREACH_OBJLIST_NC(Variable, params, iter) {
      // I am choosing not to traverse into any of the other fields of
      // the parameters, including default args.  For the application I
      // have in mind right now (matching definitions to declarations),
      // I do not need or want anything other than the parameter type.
      // So, if this code is later extended to traverse into default
      // args, there should be a flag to control that, and it should
      // default to off (or change the existing call sites).
      //
//        iter.data()->type->traverse(vis);
      // dsw: we now traverse the variable directly; the above comment
      // should probably be moved into Variable::traverse()
      if (vis.visitFunctionType_params_item(iter.data())) {
        iter.data()->traverse(vis);
        vis.postvisitFunctionType_params_item(iter.data());
      }
    }
    vis.postvisitFunctionType_params(params);
  }

  // similarly, I don't want traversal into exception specs right now

  vis.postvisitType(this);
}


void ArrayType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  eltType->traverse(vis);

  vis.postvisitType(this);
}


void PointerToMemberType::traverse(TypeVisitor &vis)
{
  if (!vis.visitType(this)) {
    return;
  }

  inClassNAT->traverse(vis);
  atType->traverse(vis);

  vis.postvisitType(this);
}


void TypedefType::traverse(TypeVisitor &vis)
{
  return underlyingType()->traverse(vis);
}


// ------------------ template.h 'traverse' methods --------------------
void TypeVariable::traverse(TypeVisitor &vis)
{
  if (!vis.visitAtomicType(this)) {
    return;
  }
  vis.postvisitAtomicType(this);
}


void PseudoInstantiation::traverse(TypeVisitor &vis)
{
  if (!vis.visitAtomicType(this)) {
    return;
  }

  primary->traverse(vis);

  if (vis.visitPseudoInstantiation_args(args)) {
    FOREACH_OBJLIST_NC(STemplateArgument, args, iter) {
      STemplateArgument *arg = iter.data();
      if (vis.visitPseudoInstantiation_args_item(arg)) {
        arg->traverse(vis);
        vis.postvisitPseudoInstantiation_args_item(arg);
      }
    }
    vis.postvisitPseudoInstantiation_args(args);
  }

  vis.postvisitAtomicType(this);
}


void traverseTargs(TypeVisitor &vis, ObjList<STemplateArgument> &list)
{
  if (vis.visitDependentQTypePQTArgsList(list)) {
    FOREACH_OBJLIST_NC(STemplateArgument, list, iter) {
      STemplateArgument *sta = iter.data();
      if (vis.visitDependentQTypePQTArgsList_item(sta)) {
        sta->traverse(vis);
        vis.postvisitDependentQTypePQTArgsList_item(sta);
      }
    }
    vis.postvisitDependentQTypePQTArgsList(list);
  }
}

void DependentQType::traverse(TypeVisitor &vis)
{
  if (!vis.visitAtomicType(this)) {
    return;
  }

  first->traverse(vis);

  PQName *name = rest;
  while (name->isPQ_qualifier()) {
    PQ_qualifier *qual = name->asPQ_qualifier();

    traverseTargs(vis, qual->sargs);
    name = qual->rest;
  }

  if (name->isPQ_template()) {
    traverseTargs(vis, name->asPQ_template()->sargs);
  }

  vis.postvisitAtomicType(this);
}


void TemplateInfo::traverseArguments(TypeVisitor &vis)
{
  FOREACH_OBJLIST_NC(STemplateArgument, arguments, iter) {
    iter.data()->traverse(vis);
  }
}


void STemplateArgument::traverse(TypeVisitor &vis)
{
  if (!vis.visitSTemplateArgument(this)) {
    return;
  }

  switch (kind) {
    case STA_TYPE:
      value.t->traverse(vis);
      break;

    case STA_DEPEXPR:
      // TODO: at some point I will have to store actual expressions,
      // and then I should traverse the expr
      break;

    default:
      break;
  }

  vis.postvisitSTemplateArgument(this);
}


// ------------------ cc-scope.h 'traverse' methods --------------------
void Scope::traverse(TypeVisitor &vis)
{
  if (!vis.visitScope(this)) {
    return;
  }
  traverse_internal(vis);
  vis.postvisitScope(this);
}

void Scope::traverse_internal(TypeVisitor &vis)
{
  if (vis.visitScope_variables(variables)) {
    for(PtrMap<char const, Variable>::Iter iter(variables);
        !iter.isDone();
        iter.adv()) {
      StringRef name = iter.key();
      Variable *var = iter.value();
      if (vis.visitScope_variables_entry(name, var)) {
        var->traverse(vis);
        vis.postvisitScope_variables_entry(name, var);
      }
    }
    vis.postvisitScope_variables(variables);
  }

  if (vis.visitScope_typeTags(typeTags)) {
    for(PtrMap<char const, Variable>::Iter iter(typeTags);
        !iter.isDone();
        iter.adv()) {
      StringRef name = iter.key();
      Variable *var = iter.value();
      if (vis.visitScope_typeTags_entry(name, var)) {
        var->traverse(vis);
        vis.postvisitScope_typeTags_entry(name, var);
      }
    }
    vis.postvisitScope_typeTags(typeTags);
  }

  if (parentScope) {
    parentScope->traverse(vis);
  }
  if (namespaceVar) {
    namespaceVar->traverse(vis);
  }

  if (vis.visitScope_templateParams(templateParams)) {
    SFOREACH_OBJLIST_NC(Variable, templateParams, iter) {
      Variable *var = iter.data();
      if (vis.visitScope_templateParams_item(var)) {
        var->traverse(vis);
        vis.postvisitScope_templateParams_item(var);
      }
    }
    vis.postvisitScope_templateParams(templateParams);
  }

  // I don't think I need this; see Scott's comments in the scope
  // class
//    Variable *parameterizedEntity;          // (nullable serf)

  // --------------- for using-directives ----------------
  // Scott says that I don't need these

  // it is basically a bug that we need to serialize this but we do so
  // there it is.
//    CompoundType *curCompound;          // (serf) CompoundType we're building
//    Should not be being used after typechecking, but in theory could omit.
  if (curCompound) {
    curCompound->traverse(vis);
  }
}


// ------------------ variable.h 'traverse' methods --------------------
void Variable::traverse(TypeVisitor &vis) {
  if (!vis.visitVariable(this)) {
    return;
  }
  if (type) {
    type->traverse(vis);
  }
  vis.postvisitVariable(this);
}


// EOF
