// variable.h                       see license.txt for copyright and terms of use
// Class Variable.

#ifndef VARIABLE_H
#define VARIABLE_H

#include "variable-fwd.h"              // forwards for this module

// elsa
#include "cc-ast-fwd.h"                // Expression, Function
#include "cc-flags.h"                  // DeclFlags, ScopeKind
#include "cc-scope-fwd.h"              // Scope
#include "cc-type-fwd.h"               // Type, etc.
#include "cc-type-visitor-fwd.h"       // TypeVisitor
#include "packedword.h"                // PackedWord
#include "template-fwd.h"              // TemplateInfo

// smbase
#include "serialno.h"                  // INHERIT_SERIAL_BASE
#include "sobjlist.h"                  // SObjList
#include "sobjset.h"                   // SObjSet
#include "srcloc.h"                    // SourceLoc
#include "strtable.h"                  // StringRef


// This (after going through the string table) is used as Variable::name
// for conversion operators.
extern char const * const specialName_conversionOperator;

// This is used as Variable::name for constructors.
extern char const * const specialName_constructor;


// information about a name
// see Diagram 1 of doc/cpp_er.html
//
// Every binding introduction (e.g. declaration) of a name will own
// one of these to describe the introduced name; every reference to
// that name will be annotated with a pointer to the Variable hanging
// off the introduction.
//
// The name 'variable' is a slight misnomer; it's used for naming:
//   - local and global variables
//   - logic variables (in the verifier)
//   - functions
//   - function parameters
//   - structure fields
//   - enumeration values
//   - typedefs
//   - namespaces
// All of these things can appear in syntactically similar contexts,
// and the lookup rules generally treat them as all being in the
// same lookup space (as opposed to enum and class tag names).
//
// I've decided that, rather than AST nodes trying to own Variables,
// Variables will live in a separate pool (like types) so the AST
// nodes can share them at will.
//
// In fact, my current idea is that the cc-type and variable modules
// are so intertwined they might as well be one.  They share
// dependencies extensively, including the TypeFactory.  So while
// they are (and will remain) physically separate files, they
// should be treated as their own subsystem.
class Variable INHERIT_SERIAL_BASE {
public:    // data
  // for now, there's only one location, and it's the definition
  // location if that exists, else the declaration location; there
  // are significant advantages to storing *two* locations (first
  // declaration, and definition), but I haven't done that yet
  SourceLoc loc;          // location of the name in the source text

  // name introduced (possibly NULL for abstract declarators)
  StringRef name;

  // type of the variable (NULL iff flags has DF_NAMESPACE)
  Type *type;

  // various flags; 'const' to force modifications to go through
  // the 'setFlagsTo' method
  const DeclFlags flags;

  // associated value for constant literals, e.g. "const int five = 5;",
  // or default value for function parameters
  //
  // if this Variable is a parameter of a template function, then this
  // 'value' might not have been tchecked; you have to look at the
  // associated TemplateInfo::uninstantiatedDefaultArgs to find out
  //
  // this is const to encourage use of use setValue()
  Expression * const value;     // (nullable serf)

  // default value for template parameters; see TODO at end
  // of this file
  Type *defaultParamType; // (nullable serf)

  // associated function definition; if NULL, either this thing isn't
  // a function or we never saw a definition
  Function *funcDefn;     // (nullable serf)

  // if this name has been overloaded, then this will be a pointer
  // to the set of overloaded names; otherwise it's NULL
  OverloadSet *overload;  // (nullable serf)

  // if we are a virtual method, the set of variables of other
  // viritual methods that we immediately override; a NULL pointer
  // here just means the empty list;
  //
  // NOTE: Scott: I wanted to do it in full correctness and not omit
  // anything in the case of multiple-inheritance; however it is
  // really hard to know what functions to omit, so I therefore do the
  // quadratic thing and include them all; given that you do something
  // similar with the BaseClassSubobj heriarchy for classes, I don't
  // think this is so bad; feel free to change it; please change the
  // name to directlyVirtuallyOverride if you do.
  SObjSet<Variable*> *virtuallyOverride;

  // Named scope in which the variable appears.  This is only non-NULL
  // if the scope is a namespace, a class, or is the global scope,
  // meaning this Variable is a member of one of those.
  //
  // Actually, it looks like this is only set when the containing scope
  // is a namespace or the global scope, *not* for members of classes.
  // That seems like a bug, but for the moment I don't really need it
  // fixed (I think).
  Scope *m_containingScope;            // (nullable serf)

  // If this Variable 'isNamespace()', then this is the Scope named by
  // this Variable.  Otherwise it is NULL.
  //
  // Note that multiple Variables can have the same 'm_denotedScope' due
  // to namespace aliases.
  Scope *m_denotedScope;               // (nullable serf)

  // total number of Variables created
  static size_t numVariables;

private:      // data
  // The next two fields are used to store conceptually different
  // things in a single word in order to save space.  I am concerned
  // about the space used by Variable because they are ubiquitous.  I
  // would like to move to a model where Variable is a superclass and
  // there are subclasses for various roles, as that would minimize
  // wasted storage, but that is a fairly big change, and for the
  // moment these localized hacks will suffice.
  //
  // dsw: see the use of the PACKEDWORD_DEF_GS macro below for the
  // partition into bits
  PackedWord intData;

  // for most kinds of Variables, this is 'getUsingAlias()'; for
  // template parameters (isTemplateParam()), this is
  // 'getParameterizedEntity()'; if 'getIsGNUAlias()', this is
  // 'getGNUAliasTarget()'.
  Variable *usingAlias_or_parameterizedEntity;   // (nullable serf)

  // for templates, this is the list of template parameters and other
  // template stuff; for a primary it includes a list of
  // already-instantiated versions
  //
  // this is private to force clients to go through templateInfo(),
  // which skips aliases, as aliases now share templateInfos with
  // the things they are aliases of
  TemplateInfo *templInfo;      // (owner)

protected:    // funcs
  friend class BasicTypeFactory;
  Variable(SourceLoc L, StringRef n, Type *t, DeclFlags f);

public:
  virtual ~Variable();

  // Throw if some invariant does not hold of this Variable.
  virtual void checkInvariants();

  bool hasFlag(DeclFlags f) const { return (flags & f) != 0; }
  bool hasAnyFlags(DeclFlags /*union*/ f) const { return (flags & f) != 0; }
  bool hasAllFlags(DeclFlags /*union*/ f) const { return (flags & f) == f; }

  void setFlag(DeclFlags f) { setFlagsTo(flags | f); }
  void addFlags(DeclFlags f) { setFlag(f); }
  void clearFlag(DeclFlags f) { setFlagsTo(flags & ~f); }

  // change the value of 'flags'; this is virtual so that annotation
  // systems can monitor flag modifications
  virtual void setFlagsTo(DeclFlags f);

  // True if 'm_containingScope' is the global scope.  This is true of
  // entities (including anonymous, such as a struct with no tag)
  // declared in the global scope, as opposed to local, prototype,
  // class, or namespace scope.  Templates declared at global scope are
  // global, but their instantiations are *not*, since they cannot be
  // found by name lookup.
  bool inGlobalScope() const;

  // True if the containing scope exists and is either the global scope
  // or a namespace scope.  Like with 'inGlobalScope()', it includes
  // templates but not their instantiations.
  bool inGlobalOrNamespaceScope() const;

  // Usually, this is 'm_containingScope->curCompound'.  But if that is
  // an anonymous compound contained within another compound, go up the
  // scope tree to find a CompoundType that is not an anonymous
  // compound, which is where this Variable will have been inserted for
  // lookup purposes.  Returns NULL if such a compound is not found.
  CompoundType const * NULLABLE containingCompoundForLookup() const;

  bool isStaticLinkage() const {
    // quarl 2006-07-11
    //    See Declarator::mid_tcheck() for why this checks for DF_INLINE && isClassMember().
    //    Note that these are not exclusive; a variable can have both static
    //    linkage (by being inline) and be a static member.
    return ( ( (hasFlag(DF_STATIC) &&
                inGlobalOrNamespaceScope()) ||
               (hasFlag(DF_INLINE) && isClassMember())
             ) /*&& !hasFlag(DF_GNU_EXTERN_INLINE)*/);
  }

  // True of variables declared in and local to a function.  False for
  // members of local classes.
  bool isLocalVariable() const {
    return !( inGlobalOrNamespaceScope() || isClassMember() );
  }

  bool isStaticLocal() const {
    return hasFlag(DF_STATIC) && isLocalVariable();
  }

  bool isStaticClassMember() const
    { return isClassMember() && hasFlag(DF_STATIC); }
  bool isNonStaticClassMember() const
    { return isClassMember() && !hasFlag(DF_STATIC); }

  // bool isStatic() const { return hasFlag(DF_STATIC); }

  // True if this is a member of a class (CompoundType).
  //
  // Except: If this is a member of an anonymous union that is declared
  // at block or file or namespace scope (i.e., not class scope), then
  // this is false.
  bool isClassMember() const;

  // True if this Variable represents a namespace.  For the purpose of
  // this function, the global scope is considered a namespace.
  bool isNamespace() const { return hasFlag(DF_NAMESPACE); }

  bool isImplicitTypedef() const { return hasAllFlags(DF_IMPLICIT | DF_TYPEDEF); }
  bool isImplicitMemberFunc() const { return hasFlag(DF_IMPLICIT) && !hasFlag(DF_TYPEDEF); }
  bool isEnumerator() const { return hasFlag(DF_ENUMERATOR); }
  bool isType() const { return hasFlag(DF_TYPEDEF); }

  bool linkerVisibleName() const;
  bool linkerVisibleName(bool evenIfStaticLinkage) const;

  // true if this name refers to a class or struct or union
  bool isClass() const;

  // refers to a user-provided typedef
  bool isExplicitTypedef() const
    { return hasFlag(DF_TYPEDEF) && !hasFlag(DF_IMPLICIT); }

  // break intData apart into various typed sub-fields
  PACKEDWORD_DEF_GS(intData, Access,           AccessKeyword,  0,  4)
  PACKEDWORD_DEF_GS(intData, Real,             bool,           4,  5)
  PACKEDWORD_DEF_GS(intData, MaybeUsedAsAlias, bool,           5,  6)
  PACKEDWORD_DEF_GS(intData, User1,            bool,           6,  7)
  PACKEDWORD_DEF_GS(intData, User2,            bool,           7,  8)
  PACKEDWORD_DEF_GS(intData, ScopeKind,        ScopeKind,      8, 11)
  PACKEDWORD_DEF_GS(intData, HasValue,         bool,          11, 12)

  // If true, this symbol has the GNU '__alias__' attribute, and
  // 'usingAlias_or_parameterizedEntity' points to the Variable this is
  // an alias of.
  PACKEDWORD_DEF_GS(intData, IsGNUAlias,       bool,          12, 13)

  PACKEDWORD_DEF_GS(intData, ParameterOrdinal, int,           16, 32)
  // ParameterOrdinal and BitfieldSize overlap, but
  // set/getBitfieldSize check an assertion before delegating to
  // ParameterOrdinal
  // PACKEDWORD_DEF_GS(intData, BitfieldSize, int, 16, 32)

  void setValue(Expression *e) { const_cast<Expression *&>(value)=e; setHasValue(e!=NULL); }

  // true if this name refers to a template function, or is
  // the typedef-name of a template class (or partial specialization)
  //
  // if 'considerInherited' is false, then the template-ness
  // is only as w.r.t. its containing class
  bool isTemplate(bool considerInherited = true) const;
  bool isTemplateFunction(bool considerInherited = true) const;
  bool isTemplateClass(bool considerInherited = true) const;

  // true if this is an instantiation of a template
  bool isInstantiation() const;

  // Templates (and specializations) and instantiations have
  // TemplateInfo.  For other entities, this is NULL.
  TemplateInfo *templateInfo() const;
  void setTemplateInfo(TemplateInfo *templInfo0);

  // Are there templatized variables (such as type variables) that are
  // not in the template info template parameters?
  bool notQuantifiedOut();

  // are we an uninstantiated template or a member of one?
  bool isUninstTemplateMember() const;

  // dsw: need a way to tell if a Variable is a method on a
  // templatized class that was never instantiated because it was
  // never used
  bool isUninstClassTemplMethod() const;

  // variable's type.. same as the public 'type' field..
  Type *getType() { return type; }
  Type const *getTypeC() const { return type; }

  // create an overload set if it doesn't exist, and return it (do not
  // do this unless you actually need a set; if you just want to treat
  // overloaded and non-overloaded variables uniformly, use
  // 'getOverloadList' instead)
  OverloadSet *getOrCreateOverloadSet();

  // return the set of overloaded entities; this might just
  // be the singleton 'this', if it isn't overloaded
  void getOverloadList(SObjList<Variable> &set);

  // true if this name is overloaded
  bool isOverloaded() const { return !!overload; }

  // number of elements in the overload set, or 1 if there is no
  // overload set
  int overloadSetSize() const;

  // true if this a pure virtual non-static member function (method)
  bool isPureVirtualMethod() const;

  // true if this is a member of a template (uninstantiated template)
  bool isMemberOfTemplate() const;

  // true if this is a template parameter or bound arg or both
  bool isAbstractTemplateParam() const
    { return hasFlag(DF_TEMPL_PARAM) && !hasFlag(DF_BOUND_TPARAM); }
  bool isBoundTemplateParam() const
    { return hasAllFlags(DF_TEMPL_PARAM | DF_BOUND_TPARAM); }
  bool isTemplateParam() const
    { return hasFlag(DF_TEMPL_PARAM); }

  // true if this is a template type parameter (unbound/abstract)
  bool isTemplateTypeParam() const;

  // generic print (C or ML depending on Type::printAsML)
  string toString() const;

  // C declaration syntax
  string toCString() const;

  // syntax when used in a parameter list
  string toCStringAsParameter() const;

  // ML-style
  string toMLString() const;

  // toString+newline to cout
  void gdb() const;

  // fully qualified but not mangled name
  string fullyQualifiedName0() const;
  void appendMangledness(stringBuilder &mgldName);
  string mangledName0(); 	// no scope
  string fullyQualifiedMangledName0(); // scope+mangling

  // like toString but with the fully qualified name
  string toQualifiedString() const;

  // hook for verifier: text to be printed after the variable's name
  // in declarator syntax
  virtual string namePrintSuffix() const;    // default: ""

  // if this is variable is actually an alias for another one, via a
  // "using declaration" (C++98 7.3.3), then this points to the one
  // it is an alias of; otherwise NULL; see comments near
  // implementation of skipAliasC
  Variable *getUsingAlias() const;
  void setUsingAlias(Variable *target);

  // if this variable is a template parameter, then this says which
  // template entity is paramterized by it; otherwise NULL
  Variable *getParameterizedEntity() const;
  void setParameterizedEntity(Variable *templ);

  // true if 'this' and 'other' are the same ordinal parameter of
  // the same template entity
  bool sameTemplateParameter(Variable const *other) const;

  // follow the 'usingAlias' field if non-NULL; otherwise return this
  Variable const *skipAliasC() const;
  Variable *skipAlias() { return const_cast<Variable*>(skipAliasC()); }

  // Get/set the GNU '__alias__' target, or NULL for none.
  Variable *getGNUAliasTarget() const;
  void setGNUAliasTarget(Variable *target);

  // true if this name refers to a template (function) or an overload
  // set that includes one
  bool namesTemplateFunction() const;

  // this must be an enumerator; get the integer value it denotes
  int getEnumeratorValue() const;

  // bitfield access
  bool isBitfield() const { return hasFlag(DF_BITFIELD); }
  void setBitfieldSize(int bits);      // must be bitfield
  int getBitfieldSize() const;         // must be bitfield

  // This variable refers to a named scope (a namespace, the global
  // scope, or a class); get it.
  Scope *getDenotedScope() const;

  // dsw: Variables are part of the type system at least for purposes
  // of traversal
  void traverse(TypeVisitor &vis);
};


// Return 'v->toString()' or, if 'v' is NULL, "NULL".
string toString(Variable const *v);

// true if 'v1' and 'v2' refer to the same run-time entity
bool sameEntity(Variable const *v1, Variable const *v2);


class OverloadSet {
public:
  // list-as-set
  SObjList<Variable> set;

public:
  OverloadSet();
  ~OverloadSet();

  void addMember(Variable *v);
  int count() const { return set.count(); }

  // These are obsolete; see Env::findInOverloadSet.
  //
  // Update: But Oink wants to use them for linker imitation.. and
  // I don't see much harm in that, since non-concrete types should
  // not be linker visible anyway.
  Variable *findByType(FunctionType const *ft, CVFlags receiverCV);
  Variable *findByType(FunctionType const *ft);
};


// This function renders an Expression as a string, if it knows how
// to.  This function is here to cut the dependency between Types and
// the AST.  If the AST-aware modules are compiled into this program,
// then this function just calls into them, prepending the prefix; but
// if not, then this always returns "".
string renderExpressionAsString(char const *prefix, Expression const *e);


/*
TODO: More efficient storage for Variable.

First, I want to make a class hierarchy like this:

  - Variable
    - TypeVariable: things that currently have DF_TYPEDEF
      - TypeParamVariable: template type paramters; make up
        a new DeclFlag to distinguish them (DF_TEMPL_PARAM?)
        [has defaultTypeArg]
      - ClassVariable: DF_TYPEDEF where 'type->isClassType()'
        [has templInfo]
    - ObjectVariable: no DF_TYPEDEF
      [has value]
      - FunctionVariable: objects where 'type->isFunctionType()'
        [has templInfo]
        [has funcDefn]
        [has overload]
      - Enumerator (obviates EnumType::Value)
    - AliasVariable (DF_USING_ALIAS?)
      [has usingAlias]
    - NamespaceVariable (?? how to namespaces name their space?)

Second, I want to collapse 'access' and 'scopeKind', since
these fields waste most of their bits.  Perhaps when DeclFlags
gets split I can arrange a storage sharing strategy among the
(then four) fields that are bit-sets.

*/

extern bool variablesLinkerVisibleEvenIfNonStaticDataMember;

#endif // VARIABLE_H
