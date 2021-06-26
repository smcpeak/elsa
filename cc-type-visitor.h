// cc-type-visitor.h
// Class TypeVisitor.

#ifndef ELSA_CC_TYPE_VISITOR_H
#define ELSA_CC_TYPE_VISITOR_H

#include "cc-type-visitor-fwd.h"       // forwards for this module

// elsa
#include "cc-ast-fwd.h"                // Expression
#include "cc-scope-fwd.h"              // Scope
#include "cc-type.h"                   // Type, etc.
#include "strmap.h"                    // StringRefMap
#include "template-fwd.h"              // STemplateArgument, etc.
#include "variable-fwd.h"              // Variable

// smbase
#include "objlist.h"                   // ObjList
#include "sobjlist.h"                  // SObjList


// Visitor that digs down into template arguments, among other things.
// Like the AST visitors, the 'visit' functions return true to
// continue digging into subtrees, or false to prune the search.
class TypeVisitor {
public:
  virtual ~TypeVisitor() {}     // silence warning

  virtual bool visitType(Type *obj);
  virtual void postvisitType(Type *obj);

  virtual bool visitFunctionType_params(SObjList<Variable> &params);
  virtual void postvisitFunctionType_params(SObjList<Variable> &params);
  virtual bool visitFunctionType_params_item(Variable *param);
  virtual void postvisitFunctionType_params_item(Variable *param);

  virtual bool visitVariable(Variable *var);
  virtual void postvisitVariable(Variable *var);

  virtual bool visitAtomicType(AtomicType *obj);
  virtual void postvisitAtomicType(AtomicType *obj);

  // I don't know where to say this, so I'll say it here: there is no
  // call in EnumType::traverse() that will take you here; visitors
  // call it manually.  However, the existance of these methods allows
  // you to organize your code in the same way as you would for the
  // other classes.
  virtual bool visitEnumType_Value(EnumType::Value *obj);
  virtual void postvisitEnumType_Value(EnumType::Value *obj);

  virtual bool visitScope(Scope *obj);
  virtual void postvisitScope(Scope *obj);

  virtual bool visitScope_variables(StringRefMap<Variable> &variables);
  virtual void postvisitScope_variables(StringRefMap<Variable> &variables);
  virtual bool visitScope_variables_entry(StringRef name, Variable *var);
  virtual void postvisitScope_variables_entry(StringRef name, Variable *var);

  virtual bool visitScope_typeTags(StringRefMap<Variable> &typeTags);
  virtual void postvisitScope_typeTags(StringRefMap<Variable> &typeTags);
  virtual bool visitScope_typeTags_entry(StringRef name, Variable *var);
  virtual void postvisitScope_typeTags_entry(StringRef name, Variable *var);

  virtual bool visitScope_templateParams(SObjList<Variable> &templateParams);
  virtual void postvisitScope_templateParams(SObjList<Variable> &templateParams);
  virtual bool visitScope_templateParams_item(Variable *var);
  virtual void postvisitScope_templateParams_item(Variable *var);

  virtual bool visitBaseClass(BaseClass *bc);
  virtual void postvisitBaseClass(BaseClass *bc);

  virtual bool visitBaseClassSubobj(BaseClassSubobj *bc);
  virtual void postvisitBaseClassSubobj(BaseClassSubobj *bc);

  virtual bool visitBaseClassSubobj_parents(SObjList<BaseClassSubobj> &parents);
  virtual void postvisitBaseClassSubobj_parents(SObjList<BaseClassSubobj> &parents);
  virtual bool visitBaseClassSubobj_parents_item(BaseClassSubobj *parent);
  virtual void postvisitBaseClassSubobj_parents_item(BaseClassSubobj *parent);

  virtual bool visitSTemplateArgument(STemplateArgument *obj);
  virtual void postvisitSTemplateArgument(STemplateArgument *obj);

  virtual bool visitPseudoInstantiation_args(ObjList<STemplateArgument> &args);
  virtual void postvisitPseudoInstantiation_args(ObjList<STemplateArgument> &args);
  virtual bool visitPseudoInstantiation_args_item(STemplateArgument *arg);
  virtual void postvisitPseudoInstantiation_args_item(STemplateArgument *arg);

  virtual bool visitDependentQTypePQTArgsList(ObjList<STemplateArgument> &list);
  virtual void postvisitDependentQTypePQTArgsList(ObjList<STemplateArgument> &list);
  virtual bool visitDependentQTypePQTArgsList_item(STemplateArgument *sta);
  virtual void postvisitDependentQTypePQTArgsList_item(STemplateArgument *sta);

  // note that this call is always a leaf in the traversal; the type
  // visitor does *not* dig into Expressions (though of course you can
  // write a 'visitExpression' method that does)
  virtual bool visitExpression(Expression *obj);
  virtual void postvisitExpression(Expression *obj);
};


#endif // ELSA_CC_TYPE_VISITOR_H
