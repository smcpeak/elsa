// cc-scope.h            see license.txt for copyright and terms of use
// a C++ scope, which is used by the Env parsing environment
// and also by CompoundType to store members
//
// see Diagram 1 of doc/cpp_er.html

#ifndef CC_SCOPE_H
#define CC_SCOPE_H

#include "cc-scope-fwd.h"              // forwards for this module

// elsa
#include "cc-ast-fwd.h"                // Function, PQName, TranslationUnit
#include "cc-env-fwd.h"                // Env
#include "cc-flags.h"                  // AccessKeyword
#include "cc-type-fwd.h"               // CompoundType, etc.
#include "cc-type-visitor-fwd.h"       // TypeVisitor
#include "lookupset.h"                 // LookupSet
#include "serialno.h"                  // INHERIT_SERIAL_BASE
#include "strmap.h"                    // StringRefMap
#include "template-fwd.h"              // TemplateParams
#include "variable-fwd.h"              // Variable

// smbase
#include "array.h"                     // ArrayStack
#include "sm-macros.h"                 // CMEMB, DMEMB
#include "sobjlist.h"                  // SObjList
#include "srcloc.h"                    // SourceLoc
#include "strtable.h"                  // StringRef


// information about a single scope: the names defined in it,
// any "current" things being built (class, function, etc.)
class Scope INHERIT_SERIAL_BASE {
private:     // types
  // for recording information about "active using" edges that
  // need to be cancelled at scope exit
  class ActiveEdgeRecord {
  public:
    Scope *source, *target;     // edge exists from source to target

  public:
    ActiveEdgeRecord()
      : source(NULL), target(NULL) {}
    ActiveEdgeRecord(ActiveEdgeRecord const &obj)
      : DMEMB(source), DMEMB(target) {}
    ActiveEdgeRecord(Scope *s, Scope *t)
      : source(s), target(t) {}

    ActiveEdgeRecord& operator= (ActiveEdgeRecord const &obj)
      { CMEMB(source); CMEMB(target); return *this; }
  };

private:     // data
  // variables: name -> Variable
  // note: this includes typedefs (DF_TYPEDEF is set), and it also
  // includes enumerators (DF_ENUMERATOR is set)
  StringRefMap<Variable> variables;

  // 2005-02-24: Rather than keeping separate maps for compounds
  // and enums, I am now going to keep them in a unified map of
  // type tags to typedef variables.
  StringRefMap<Variable> typeTags;

  // per-scope change count
  int changeCount;

  // true if the scope is currently on the scope stack, meaning
  // lookups can find it; this is used to ensure that no scope is
  // ever on the scope stack twice
  bool onScopeStack;

public:      // data
  // when this is set to false, the environment knows it should not
  // put new names into this scope, but rather go further down into
  // the scope stack to insert the name (used for scopes of template
  // parameters, after the names have been added)
  bool canAcceptNames;

  // (serf) This is the parent (enclosing) scope, but only if that
  // scope 'isPermanentScope()'.
  //
  // For classes, this field is only set to non-NULL after the inner
  // class has been fully constructed, since we can rely on the
  // Environment's scope stack to look up things in containing classes
  // while building the inner class for the first time (why did I do
  // that?).  For namespaces, it's set as soon as the namespace is
  // created.
  Scope *parentScope;

  // what kind of scope is this?
  ScopeKind scopeKind;

  // if this is a namespace, this points to the variable used to
  // find the namespace during lookups
  Variable *namespaceVar;

  // If this is a template (SK_TEMPLATE_PARAMS) scope, these are the
  // template parameters.  We will attach them to functions and
  // classes contained in this scope, as those functions and classes
  // are parameterized by these variables.
  SObjList<Variable> templateParams;

  // If this is SK_TEMPLATE_PARAMS, then the parameters correspond to
  // some specific template entity (primary, specialization, or
  // instantiation, whichever is most specific to the situation).
  // This pointer names that entity.  It is initially NULL, as we
  // don't immediately know which is being parameterized, but is set
  // to non-NULL as soon as we know.
  //
  // Once this is set to point at a class template entity, this
  // scope is no longer used for lookups!  Instead, the parameterized
  // class will delegate lookups directly at the proper time.
  Variable *parameterizedEntity;          // (nullable serf)

  // --------------- for using-directives ----------------
  // possible optim:  For most scopes, these three arrays waste 13
  // words of storage.  I could collect them into a separate structure
  // and just keep a pointer here, making it non-NULL only when
  // something gets put into an array.

  // set of "using" edges; these affect lookups transitively when
  // other scopes have active edges to this one
  ArrayStack<Scope*> usingEdges;

  // this is the in-degree of the usingEdges network; it is used to
  // tell when a scope has someone using it, because that means we
  // may need to recompute the active-using edges
  int usingEdgesRefct;

  // set of "active using" edges; these directly influence lookups
  // in this scope
  ArrayStack<Scope*> activeUsingEdges;

  // set of "active using" edges in other scopes that need to be
  // retracted once this scope exits
  ArrayStack<ActiveEdgeRecord> outstandingActiveEdges;

  // ------------- "current" entities -------------------
  // these are set to allow the typechecking code to know about
  // the context we're in
  CompoundType *curCompound;          // (serf) CompoundType we're building
  AccessKeyword curAccess;            // access disposition in effect
  Function *curFunction;              // (serf) Function we're analyzing
  SourceLoc curLoc;                   // latest AST location marker seen

private:     // funcs
  Variable *lookupVariable_inner
    (LookupSet &candidates, StringRef name, Env &env, LookupFlags flags);
  void lookupVariable_considerBase
    (StringRef name, Env &env, LookupFlags flags,
     Variable *&v1,
     BaseClassSubobj const *&v1Subobj,
     BaseClassSubobj const *v2Subobj);

  // more using-directive stuff
  void addActiveUsingEdge(Scope *target);
  void removeActiveUsingEdge(Scope *target);
  void scheduleActiveUsingEdge(Env &env, Scope *target);
  bool foundViaUsingEdge(LookupSet &candidates, Env &env, LookupFlags flags,
                         Variable *v, Variable *&vfound);
  Variable *searchActiveUsingEdges
    (LookupSet &candidates, StringRef name,
     Env &env, LookupFlags flags, Variable *vfound);
  Variable *searchUsingEdges
    (LookupSet &candidates, StringRef name, Env &env, LookupFlags flags);
  void getUsingClosure(ArrayStack<Scope*> &dest);

  // variant of 'lookup' that does not expand overload sets
  Variable *lookupSingleVariable(StringRef name, LookupFlags flags);

protected:   // funcs
  // this function is called at the end of addVariable, after the
  // Variable has been added to the 'variables' map; it's intended
  // that CompoundType will override this, for the purpose of
  // maintaining 'dataMembers'
  virtual void afterAddVariable(Variable *v);

public:      // funcs
  Scope(ScopeKind sk, int changeCount, SourceLoc initLoc);
  virtual ~Scope();     // virtual to silence warning; destructor is not part of virtualized interface

  int getChangeCount() const { return changeCount; }

  // this is actually for debugging only ....
  int getNumVariables() const       { return variables.getNumEntries(); }

  // some syntactic sugar on the scope kind
  bool isGlobalScope() const        { return scopeKind == SK_GLOBAL; }
  bool isParameterScope() const     { return scopeKind == SK_PARAMETER; }
  bool isFunctionScope() const      { return scopeKind == SK_FUNCTION; }
  bool isClassScope() const         { return scopeKind == SK_CLASS; }
  bool isTemplateParamScope() const { return scopeKind == SK_TEMPLATE_PARAMS; }
  bool isTemplateArgScope() const   { return scopeKind == SK_TEMPLATE_ARGS; }
  bool isNamespace() const          { return scopeKind == SK_NAMESPACE; }

  // template params or args ...
  bool isTemplateScope() const      { return isTemplateParamScope() || isTemplateArgScope(); }

  // True of the global scope, namespace scopes, and class scopes.
  //
  // It excludes function/block scopes on the premise that we could
  // (although currently do not) deallocate them after the corresponding
  // function has been processed.
  bool isPermanentScope() const;

  // are we in a template scope that is in a global scope?
  bool isGlobalTemplateScope() const;
  // are we in a scope that at some point above is an uninstantiated
  // templatized scope?
  bool isWithinUninstTemplate() const;

  // True if this scope has extant template parameters.  It is not
  // enough to be a template scope, it must have parameters beyond
  // an empty "<>".
  bool hasTemplateParams() const;

  // true if this scope is only accessed via delegation from
  // a CompoundType
  bool isDelegated() const;

  // true if this scope is CompoundType and delegates to another
  bool hasDelegationPointer() const
    { return !!getDelegationPointer(); }
  Scope *getDelegationPointer() const;
  Scope *getAndNullifyDelegationPointer();
  void setDelegationPointer(Scope *s);

  // insertion; these return false if the corresponding map already
  // has a binding (unless 'forceReplace' is true)
  bool addVariable(Variable *v, bool forceReplace=false);
  bool addCompound(CompoundType *ct);
  bool addEnum(EnumType *et);
  bool addTypeTag(Variable *tag);

  // mark 'v' as being a member of this scope, by setting its 'scope'
  // and 'scopeKind' members (this is not done by 'addVariable')
  void registerVariable(Variable *v);

  // somewhat common sequence: register, add, assert that the add worked
  void addUniqueVariable(Variable *v);

  // Add 'v' to 'variables' so it is found by lookup, but do *not* call
  // 'afterAddVariable', nor 'registerVariable'.
  //
  // If there is a collision with an existing entry, return the existing
  // one (meaning failure), otherwise NULL (meaning success).
  Variable * NULLABLE addVariableForLookupOnly(Variable *v);

  // 2005-02-24: new and improved lookup
  void lookup(LookupSet &set, StringRef name, Env &env, LookupFlags flags);

  // 2005-08-03: when 'env' is NULL, errors are not reported
  void lookup(LookupSet &set, StringRef name, Env /*nullable*/ *env,
              LookupFlags flags);

  // like Env::lookupPQ_one
  Variable *lookup_one(StringRef name, Env &env, LookupFlags flags);
  Variable *lookup_one(StringRef name, Env *env, LookupFlags flags);

  // lookup; these return NULL if the name isn't found; 'env' is
  // passed for the purpose of reporting ambiguity errors
  Variable *lookupVariable(StringRef name, Env &env, LookupFlags f=LF_NONE);
  CompoundType const *lookupCompoundC(StringRef name, Env &env, LookupFlags f=LF_NONE) const;
  EnumType const *lookupEnumC(StringRef name, Env &env, LookupFlags f=LF_NONE) const;

  // compounds/enums
  Variable *lookupTypeTag(StringRef name, Env &env, LookupFlags f=LF_NONE) const;

  // non-const versions..
  CompoundType *lookupCompound(StringRef name, Env &env, LookupFlags f=LF_NONE)
    { return const_cast<CompoundType*>(lookupCompoundC(name, env, f)); }
  EnumType *lookupEnum(StringRef name, Env &env, LookupFlags f=LF_NONE)
    { return const_cast<EnumType*>(lookupEnumC(name, env, f)); }

  // for iterating over the variables
  StringRefMap<Variable>::Iter getVariableIter() const
    { return StringRefMap<Variable>::Iter(variables); }

  // and the type tags
  StringRefMap<Variable>::Iter getTypeTagIter() const
    { return StringRefMap<Variable>::Iter(typeTags); }

  // lookup within the 'variables' map, without consulting base
  // classes, etc.; returns NULL if not found
  Variable *rawLookupVariable(StringRef name)
    { return variables.get(name); }

  // extended interface for returning sets
  Variable *lookupVariable_set
    (LookupSet &candidates, StringRef name, Env &env, LookupFlags flags);


  // if this scope has a name, return the typedef variable that
  // names it; otherwise, return NULL
  Variable const *getTypedefNameC() const;
  Variable *getTypedefName() { return const_cast<Variable*>(getTypedefNameC()); }
  bool hasName() const { return scopeKind==SK_CLASS || scopeKind==SK_NAMESPACE; }

  // true if this scope encloses (has as a nested scope) 's'; this
  // is proper enclosure: it is not the case that s->encloses(s)
  bool encloses(Scope const *s) const;

  // non-proper enclosure
  bool enclosesOrEq(Scope const *s) const;

  // stuff for using-directives
  void addUsingEdge(Scope *target);
  void addUsingEdgeTransitively(Env &env, Scope *target);

  // indication of scope open/close so we can maintain the
  // connection between "using" and "active using" edges
  void openedScope(Env &env);
  void closedScope();

  // dsw: needed this and this was a natural place to put it
  bool immediateGlobalScopeChild();
  bool linkerVisible();

  // This is just a unique and possibly human readable string; it is
  // used in the Oink linker imitator.
  //
  // sm: TODO: Change the name so it reflects the mangling activity;
  // I want "fullyQualifiedName" to do what "fullyQualifiedCName"
  // does now.
  //
  // dsw: I made your way the only way; that is, I just inlined mangle
  // as set to false
  string fullyQualifiedName();

  // more C-like notation for a fully qualified name
  string fullyQualifiedCName() {
    // dsw: see the note above; these functions are now identical.
    return fullyQualifiedName();
  }

  // set 'parameterizedEntity', checking a few things in the process
  void setParameterizedEntity(Variable *entity);

  // this scope must be a CompoundType or a namespace; get the
  // Variable 'v' such that v.getDenotedScope() == this
  Variable *getScopeVariable() const;

  // dsw: I know this is *almost* the only virtual method, but
  // traverse() is virtual everywhere else; change it if you like
  virtual void traverse(TypeVisitor &vis);
  // this is factored out so that subclasses can call it
  void traverse_internal(TypeVisitor &vis);

  // name of this scope for use in error messages and such
  string scopeName() const;

  // for debugging, a quick description of this scope
  string desc() const;
  void gdb() const;
};


#endif // CC_SCOPE_H
