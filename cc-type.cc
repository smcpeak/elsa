// cc-type.cc            see license.txt for copyright and terms of use
// code for cc-type.h

#include "cc-type.h"                   // this module

// elsa
#include "strip-comments.h"            // stripComments
#include "template.h"                  // TemplateInfo, etc.
#include "type-sizes.h"                // TypeSizes
#include "variable.h"                  // Variable

// This dependency is to support using MType for equality checking.
// It causes a transitive dependency on cc.ast, but I rationalize that
// an MType variant could be created, if necessary, that cut that
// dependency, especially as none of the call sites in this file
// explicitly supply MF_MATCH.
#include "mtype.h"                     // MType

// Do *not* add a dependency on cc.ast or cc-env.  cc-type is about
// the intrinsic properties of types, independent of any particular
// syntax for denoting them.  (toString() uses C's syntax, but that's
// just for debugging.)

// ast
#include "asthelp.h"                   // ind

// smbase
#include "hashtbl.h"                   // lcprngTwoSteps
#include "sm-stdint.h"                 // uintptr_t
#include "sobjset.h"                   // SObjSet
#include "strutil.h"                   // copyToStaticBuffer
#include "trace.h"                     // tracingSys

// libc++
#include <algorithm>                   // std::max

// libc
#include <stdlib.h>                    // getenv


// 2005-08-10: the anon things are kind of ugly..
bool printAnonComment = false;


// All 'traverse(TypeVisitor&)' methods are implemented in
// cc-type-visitor.cc.


// ------------------ AtomicType -----------------
ALLOC_STATS_DEFINE(AtomicType)

AtomicType::AtomicType()
{
  ALLOC_STATS_IN_CTOR
}


AtomicType::~AtomicType()
{
  ALLOC_STATS_IN_DTOR
}


DOWNCAST_IMPL(AtomicType, SimpleType)
DOWNCAST_IMPL(AtomicType, NamedAtomicType)
DOWNCAST_IMPL(AtomicType, CompoundType)
DOWNCAST_IMPL(AtomicType, EnumType)
DOWNCAST_IMPL(AtomicType, TypeVariable)
DOWNCAST_IMPL(AtomicType, PseudoInstantiation)
DOWNCAST_IMPL(AtomicType, DependentQType)


void AtomicType::gdb() const
{
  cout << toString() << endl;
}


string AtomicType::toString() const
{
  if (Type::printAsML) {
    return toMLString();
  }
  else {
    return toCString();
  }
}


bool AtomicType::isNamedAtomicType() const
{
  // default to false; NamedAtomicType overrides
  return false;
}


bool AtomicType::equals(AtomicType const *obj) const
{
  MType match;
  return match.matchAtomicType(this, obj, MF_NONE);
}


string toString(AtomicType const *at)
{
  if (at) {
    return at->toString();
  }
  else {
    return "NULL";
  }
}


// ------------------ SimpleType -----------------
SimpleType SimpleType::fixed[NUM_SIMPLE_TYPES] = {
  SimpleType(ST_CHAR),
  SimpleType(ST_UNSIGNED_CHAR),
  SimpleType(ST_SIGNED_CHAR),
  SimpleType(ST_BOOL),
  SimpleType(ST_INT),
  SimpleType(ST_UNSIGNED_INT),
  SimpleType(ST_LONG_INT),
  SimpleType(ST_UNSIGNED_LONG_INT),
  SimpleType(ST_LONG_LONG),
  SimpleType(ST_UNSIGNED_LONG_LONG),
  SimpleType(ST_SHORT_INT),
  SimpleType(ST_UNSIGNED_SHORT_INT),
  SimpleType(ST_WCHAR_T),
  SimpleType(ST_FLOAT),
  SimpleType(ST_DOUBLE),
  SimpleType(ST_LONG_DOUBLE),
  SimpleType(ST_FLOAT_COMPLEX),
  SimpleType(ST_DOUBLE_COMPLEX),
  SimpleType(ST_LONG_DOUBLE_COMPLEX),
  SimpleType(ST_FLOAT_IMAGINARY),
  SimpleType(ST_DOUBLE_IMAGINARY),
  SimpleType(ST_LONG_DOUBLE_IMAGINARY),
  SimpleType(ST_VOID),

  SimpleType(ST_ELLIPSIS),
  SimpleType(ST_CDTOR),
  SimpleType(ST_ERROR),
  SimpleType(ST_DEPENDENT),
  SimpleType(ST_IMPLINT),
  SimpleType(ST_NOTFOUND),

  SimpleType(ST_PROMOTED_INTEGRAL),
  SimpleType(ST_PROMOTED_ARITHMETIC),
  SimpleType(ST_INTEGRAL),
  SimpleType(ST_ARITHMETIC),
  SimpleType(ST_ARITHMETIC_NON_BOOL),
  SimpleType(ST_ANY_OBJ_TYPE),
  SimpleType(ST_ANY_NON_VOID),
  SimpleType(ST_ANY_TYPE),

  SimpleType(ST_PRET_STRIP_REF),
  SimpleType(ST_PRET_PTM),
  SimpleType(ST_PRET_ARITH_CONV),
  SimpleType(ST_PRET_FIRST),
  SimpleType(ST_PRET_FIRST_PTR2REF),
  SimpleType(ST_PRET_SECOND),
  SimpleType(ST_PRET_SECOND_PTR2REF),
};


/*static*/ SimpleType *SimpleType::getST(SimpleTypeId id)
{
  xassert((unsigned)id < (unsigned)NUM_SIMPLE_TYPES);
  return &SimpleType::fixed[id];
}


string SimpleType::toCString() const
{
  return simpleTypeName(type);
}


int SimpleType::reprSize(TypeSizes const &typeSizes) const
{
  return simpleTypeReprSize(typeSizes, type);
}


// ------------------ NamedAtomicType --------------------
NamedAtomicType::NamedAtomicType(StringRef n)
  : name(n),
    typedefVar(NULL),
    access(AK_PUBLIC)
{}

NamedAtomicType::~NamedAtomicType()
{
  if (typedefVar) {
    delete typedefVar;
  }
}


bool NamedAtomicType::isNamedAtomicType() const
{
  return true;
}


TypeIntr NamedAtomicType::getTypeIntr() const
{
  if (CompoundType const *ctype = this->ifCompoundTypeC()) {
    return CompoundType::toTypeIntr(ctype->keyword);
  }

  if (this->isEnumType()) {
    return TI_ENUM;
  }

  // Should not be possible since there are only two subclasses of
  // NamedAtomicType.
  xfailure("AtomicType is neither CompoundType nor EnumType");
  return TI_STRUCT;           // Not reached.
}


// ---------------- BaseClass ----------------


// ---------------- BaseClassSubobj ----------------
BaseClassSubobj::BaseClassSubobj(BaseClass const &base)
  : BaseClass(base),
    parents(),       // start with an empty list
    visited(false)   // may as well init; clients expected to clear as needed
{}


BaseClassSubobj::~BaseClassSubobj()
{
  // I don't think this code has been tested..

  // virtual parent objects are owned by the containing class'
  // 'virtualBases' list, but nonvirtual parents are owned
  // by the hierarchy nodes themselves (i.e. me)
  while (parents.isNotEmpty()) {
    BaseClassSubobj *parent = parents.removeFirst();

    if (!parent->isVirtual) {
      delete parent;
    }
  }
}


string BaseClassSubobj::canonName() const
{
  return stringc << ct->name << " (" << (void*)this << ")";
}


// ------------------ CompoundType -----------------
CompoundType::CompoundType(Keyword k, StringRef n)
  : NamedAtomicType(n),
    Scope(SK_CLASS, 0 /*changeCount*/, SL_UNKNOWN /*dummy loc*/),
    m_isForwardDeclared(true),
    m_isTransparentUnion(false),
    m_isAnonymousCompound(false),
    keyword(k),
    bases(),
    virtualBases(),
    subobj(BaseClassSubobj(this, AK_PUBLIC, false /*isVirtual*/)),
    conversionOperators(),
    instName(n),
    syntax(NULL),
    parameterizingScope(NULL),
    selfType(NULL)
{
  curCompound = this;
  curAccess = (k==K_CLASS? AK_PRIVATE : AK_PUBLIC);
}

CompoundType::~CompoundType()
{
  //bases.deleteAll();    // automatic, and I'd need a cast to do it explicitly because it's 'const' now
  if (templateInfo()) {
    delete templateInfo();
    setTemplateInfo(NULL);      // dsw: I don't like pointers to deleted objects
  }
}


STATICDEF char const *CompoundType::keywordName(Keyword k)
{
  switch (k) {
    default:          xfailure("bad keyword");
    case K_STRUCT:    return "struct";
    case K_CLASS:     return "class";
    case K_UNION:     return "union";
  }
}


// Elsewhere (cc-tcheck.cc), we rely on TypeIntr and Keyword having
// compatible numeric values.  I do not want to continue that.
STATICDEF TypeIntr CompoundType::toTypeIntr(Keyword k)
{
  switch (k) {
    default: xfailure("bad keyword");
    case K_STRUCT: return TI_STRUCT;
    case K_CLASS:  return TI_CLASS;
    case K_UNION:  return TI_UNION;
  }
}


bool CompoundType::isTemplate(bool considerInherited) const
{
  TemplateInfo *tinfo = templateInfo();
  return tinfo != NULL &&
         tinfo->hasParametersEx(considerInherited);
}


bool CompoundType::isInstantiation() const
{
  TemplateInfo *tinfo = templateInfo();
  return tinfo != NULL &&
         tinfo->isInstantiation();
}


Variable *CompoundType::getTypedefVar() const
{
  xassert(typedefVar);
  xassert(typedefVar->type);
  if (typedefVar->type->isError()) {
    // this is the error compound type
  }
  else {
    xassert(typedefVar->type->asCompoundTypeC() == this);
  }
  return typedefVar;
}


TemplateInfo *CompoundType::templateInfo() const
{
  return getTypedefVar()->templateInfo();
}

void CompoundType::setTemplateInfo(TemplateInfo *templInfo0)
{
  if (templInfo0) {
    getTypedefVar()->setTemplateInfo(templInfo0);
  }
}


bool CompoundType::hasVirtualFns() const
{
  // TODO: this fails to consider members inherited from base classes
  // ...
  // UPDATE dsw: I think the code in Declarator::mid_tcheck() is
  // supposed to push the virtuality down at least for those function
  // members that implicitly inherit their virtuality from the fact
  // that they override a virtual member in their superclass.

  for (StringRefMap<Variable>::Iter iter(getVariableIter());
       !iter.isDone(); iter.adv()) {
    Variable *var0 = iter.value();
    if (var0->hasFlag(DF_VIRTUAL)) {
      xassert(var0->getType()->asRval()->isFunctionType());
      return true;
    }
  }
  return false;
}


string CompoundType::toCString() const
{
  stringBuilder sb;

  // typedefVar might be NULL if this object is in the middle
  // of being built, but I want it to be printable at all times
  TemplateInfo *tinfo = typedefVar? templateInfo() : NULL;

  bool hasParams = tinfo && tinfo->params.isNotEmpty();
  if (hasParams) {
    sb << tinfo->paramsToCString() << " ";
  }

  if (!tinfo || hasParams) {
    // only say 'class' if this is like a class definition, or
    // if we're not a template, since template instantiations
    // usually don't include the keyword 'class' (this isn't perfect..
    // I think I need more context)
    //
    // 2021-06-13: I do not know when I need this at all in C++, and
    // when printing implicit class members I do not want the keyword,
    // so try turning it off altogether.
    //sb << keywordName(keyword) << " ";
  }

  //sb << (instName? instName : "/*anonymous*/");
  if (typedefVar) {
    sb << typedefVar->fullyQualifiedName0();
  }
  else {
    // only reachable during object construction
    sb << (name? name : "/*anon*/");
  }

  // template arguments are now in the name
  // 4/22/04: they were removed from the name a long time ago;
  //          but I'm just now uncommenting this code
  // 8/03/04: restored the original purpose of 'instName', so
  //          once again that is name+args, so this code is not needed
  //if (tinfo && tinfo->arguments.isNotEmpty()) {
  //  sb << sargsToString(tinfo->arguments);
  //}

  return sb;
}


int CompoundType::reprSize(TypeSizes const &typeSizes) const
{
  int total = 0;

  // base classes
  {
    SObjList<BaseClassSubobj const> subobjs;
    getSubobjects(subobjs);
    SFOREACH_OBJLIST(BaseClassSubobj const, subobjs, iter) {
      if (iter.data()->ct == this) {
        // skip my own subobject, as that will be accounted for below
      }
      else {
        total += iter.data()->ct->reprSize(typeSizes);
      }
    }
  }

  // This algorithm is a very crude approximation of the packing and
  // alignment behavior of some nominal compiler.  Ideally, we'd have
  // a layout algorithm for each compiler we want to emulate, or
  // perhaps even a generic algorithm with sufficient
  // parameterization, but for now it's just a best effort driven by
  // specific pieces of code that know how big their own structures
  // are supposed to be.
  //
  // Were I to try to do a better job, a good starting point would be
  // to research any published ABIs I could find for C and C++, as
  // they would have to specify a layout algorithm.  Presumably, if I
  // can emulate any published ABI then I will have the flexibility to
  // emulate any compiler also.
  //
  // One test is in/t0513.cc.

  // Maintain information about accumulated members that do not occupy
  // a complete word.
  int bits = 0;        // bitfield bits
  int bytes = 0;       // unaligned bytes
  int align = 1;       // prevailing alignment in bytes

  // data members
  SFOREACH_OBJLIST(Variable, dataMembers, iter) {
    Variable const *v = iter.data();

    if (keyword == K_UNION) {
      // representation size is max over field sizes
      total = std::max(total, v->type->reprSize(typeSizes));
      continue;
    }

    if (v->isBitfield()) {
      // consolidate bytes as bits
      bits += bytes*8;
      bytes = 0;

      int membBits = v->getBitfieldSize();
      bits += membBits;

      // increase alignment to accomodate this member
      while (membBits > align*8 && align < 4) {
        align *= 2;
      }

      continue;
    }

    // 'v' is not a bitfield, so pack the bits seen so far into
    // 'align' units
    if (bits > 0) {
      total += ((bits + (align*8-1)) / (align*8)) * align;
      bits = 0;
    }

    if (v->type->isArrayTypeWithUnspecifiedSize()) {
      // "open array"; there are checks elsewhere that control whether
      // Elsa allows this, this at point in the code I can just assume
      // that they are allowed; they are treated as having size 0, so
      // just skip it (in/c/dC0032.c)
      continue;
    }

    int membSize = v->type->reprSize(typeSizes);

    if (membSize >= align) {
      // increase alignment if necessary, up to 4 bytes;
      // this is wrong because you can't tell the alignment
      // of a structure just from its size
      while (membSize > align && align < 4) {
        align *= 2;
      }

      // consolidate any remaining bytes into 'align' units
      if (bytes > 0) {
        total += ((bytes + align-1) / align) * align;
        bytes = 0;
      }

      // add 'membSize'
      total += (membSize / align) * align;
      bytes += membSize % align;
    }
    else {
      // less than one alignment, just stuff it into 'bytes'; this
      // is wrong because it doesn't take account of alignment
      // less than 'align', e.g. 2-byte alignment of a 16-bit qty..
      // oh well
      bytes += membSize;
    }
  }

  // pad out to the next 'align' boundary
  bits += bytes*8;
  total += ((bits + (align*8-1)) / (align*8)) * align;

  return total;
}


string CompoundType::keywordAndName() const
{
  if (TemplateInfo const *ti = templateInfo()) {
    return stringb(::toString(keyword) << ' ' << ti->templateName());
  }
  else {
    // TODO: It would be nice for 'name' to be fully qualified here.
    return stringb(::toString(keyword) << " " <<
                   (name? name : "(anon)"));
  }
}


int CompoundType::numFields() const
{
  int ct = 0;
  for (StringRefMap<Variable>::Iter iter(getVariableIter());
       !iter.isDone(); iter.adv()) {
    Variable *v = iter.value();

    // count nonstatic data members
    if (!v->type->isFunctionType() &&
        !v->hasFlag(DF_TYPEDEF) &&
        !v->hasFlag(DF_STATIC)) {
      ct++;
    }
  }

  return ct;
}


void CompoundType::addField(Variable *v)
{
  if (v->name) {
    addVariable(v);
  }

  else {
    // Associate 'v' with the scope, setting its containing scope.
    registerVariable(v);

    // Add it to the data members as well so that Elsa clients can easily
    // see that there is a nested member here.
    dataMembers.append(v);
    TRACE("anon-comp", stringb(
      "Added anonymous member declared at " << toLCString(v->loc) <<
      " to container '" << toString() << "'."));
  }
}


void CompoundType::afterAddVariable(Variable *v)
{
  // if is a data member, not a method, static data, or a typedef
  if (!v->type->isFunctionType() &&
      !(v->flags & (DF_STATIC | DF_TYPEDEF | DF_ENUMERATOR | DF_USING_ALIAS))) {
    dataMembers.append(v);
  }
}


Variable const *CompoundType::getDataMemberByPositionC(int index) const
{
  return dataMembers.nthC(index);
}


int CompoundType::getDataMemberPosition(StringRef name) const
{
  Type const *dummy;
  return getDataMemberPositionAndType(dummy, name);
}

int CompoundType::getDataMemberPositionAndType(Type const * /*OUT*/ &type,
                                               StringRef name) const
{
  int index=0;
  SFOREACH_OBJLIST(Variable, dataMembers, iter) {
    if (iter.data()->name == name) {
      type = iter.data()->type;
      return index;
    }
    index++;
  }

  type = NULL;
  return -1;     // not found
}


string CompoundType::dataMembersToString() const
{
  stringBuilder sb;
  sb << "{ ";

  SFOREACH_OBJLIST(Variable, dataMembers, iter) {
    Variable const *member = iter.data();

    sb << member->type->toString() << ' ';
    if (member->name) {
      sb << member->name;
    }
    else {
      sb << "/*anon at " << toLCString(member->loc) << "*/";
    }
    sb << "; ";
  }

  sb << '}';
  return sb.str();
}


// TODO: Does this handle members of base classes correctly?  What
// about virtual inheritance?
int CompoundType::getDataMemberOffset(
  TypeSizes const &typeSizes, Variable const *dataMember) const
{
  int offset = 0;
  SFOREACH_OBJLIST(Variable, dataMembers, iter) {
    if (iter.data() == dataMember) {
      return offset;
    }

    // TODO: This does not take padding for alignment into account.
    offset += iter.data()->type->reprSize(typeSizes);
  }

  xfailure_stringbc("getDataMemberOffset: no such member: "
                    << dataMember->name);
  return 0;  // silence warning
}


void CompoundType::addBaseClass(BaseClass * /*owner*/ newBase)
{
  xassert(newBase->access != AK_UNSPECIFIED);

  // add the new base; override 'const' so we can modify the list
  // (this is the one function allowed to do this)
  const_cast<ObjList<BaseClass>&>(bases).append(newBase);

  // replicate 'newBase's inheritance hierarchy in the subobject
  // hierarchy, representing virtual inheritance with explicit sharing
  makeSubobjHierarchy(&subobj, newBase);
}

// all of this is done in the context of one class, the one whose
// tree we're building, so we can access its 'virtualBases' list
void CompoundType::makeSubobjHierarchy(
  // the root of the subobject hierarchy we're adding to
  BaseClassSubobj *subobj,

  // the base class to add to subobj's hierarchy
  BaseClass const *newBase)
{
  if (newBase->isVirtual) {
    // does 'newBase' correspond to an already-existing virtual base?
    BaseClassSubobj *vbase = findVirtualSubobject(newBase->ct);
    if (vbase) {
      // yes it's already a virtual base; just point 'subobj'
      // at the existing instance
      subobj->parents.append(vbase);

      // we're incorporating 'newBase's properties into 'vbase';
      // naturally both should already be marked as virtual
      xassert(vbase->isVirtual);

      // grant access according to the least-restrictive link
      if (newBase->access < vbase->access) {
        vbase->access = newBase->access;    // make it less restrictive
      }

      // no need to recursively examine 'newBase' since the presence
      // of 'vbase' in 'virtualBases' means we already incorporated
      // its inheritance structure
      return;
    }
  }

  // represent newBase in the subobj hierarchy
  BaseClassSubobj *newBaseObj = new BaseClassSubobj(*newBase);
  subobj->parents.append(newBaseObj);

  // if we just added a new virtual base, remember it
  if (newBaseObj->isVirtual) {
    virtualBases.append(newBaseObj);
  }

  // put all of newBase's base classes into newBaseObj
  FOREACH_OBJLIST(BaseClass, newBase->ct->bases, iter) {
    makeSubobjHierarchy(newBaseObj, iter.data());
  }
}


// simple scan of 'virtualBases'
BaseClassSubobj const *CompoundType::findVirtualSubobjectC
  (CompoundType const *ct) const
{
  FOREACH_OBJLIST(BaseClassSubobj, virtualBases, iter) {
    if (iter.data()->ct == ct) {
      return iter.data();
    }
  }
  return NULL;   // not found
}


// fundamentally, this takes advantage of the ownership scheme,
// where nonvirtual bases form a tree, and the 'virtualBases' list
// gives us additional trees of internally nonvirtual bases
void CompoundType::clearSubobjVisited() const
{
  // clear the 'visited' flags in the nonvirtual bases
  clearVisited_helper(&subobj);

  // clear them in the virtual bases
  FOREACH_OBJLIST(BaseClassSubobj, virtualBases, iter) {
    clearVisited_helper(iter.data());
  }
}

STATICDEF void CompoundType::clearVisited_helper
  (BaseClassSubobj const *subobj)
{
  subobj->visited = false;

  // recursively clear flags in the *nonvirtual* bases
  SFOREACH_OBJLIST(BaseClassSubobj, subobj->parents, iter) {
    if (!iter.data()->isVirtual) {
      clearVisited_helper(iter.data());
    }
  }
}


// interpreting the subobject hierarchy recursively is sometimes a bit
// of a pain, especially when the client doesn't care about the access
// paths, so this allows a more natural iteration by collecting them
// all into a list; this function provides a prototypical example of
// how to interpret the structure recursively, when that is necessary
void CompoundType::getSubobjects(SObjList<BaseClassSubobj const> &dest) const
{
  // reverse before also, in case there happens to be elements
  // already on the list, so those won't change order
  dest.reverse();

  clearSubobjVisited();
  getSubobjects_helper(dest, &subobj);

  // reverse the list since it was constructed in reverse order
  dest.reverse();
}

STATICDEF void CompoundType::getSubobjects_helper
  (SObjList<BaseClassSubobj const> &dest, BaseClassSubobj const *subobj)
{
  if (subobj->visited) return;
  subobj->visited = true;

  dest.prepend(subobj);

  SFOREACH_OBJLIST(BaseClassSubobj, subobj->parents, iter) {
    getSubobjects_helper(dest, iter.data());
  }
}


string CompoundType::renderSubobjHierarchy() const
{
  stringBuilder sb;
  sb << "// subobject hierarchy for " << name << "\n"
     << "digraph \"Subobjects\" {\n"
     ;

  SObjList<BaseClassSubobj const> objs;
  getSubobjects(objs);

  // look at all the subobjects
  SFOREACH_OBJLIST(BaseClassSubobj const, objs, iter) {
    BaseClassSubobj const *obj = iter.data();

    sb << "  \"" << obj->canonName() << "\" [\n"
       << "    label = \"" << obj->ct->name
                  << "\\n" << ::toString(obj->access) << "\"\n"
       << "  ]\n"
       ;

    // render 'obj's base class links
    SFOREACH_OBJLIST(BaseClassSubobj, obj->parents, iter) {
      BaseClassSubobj const *parent = iter.data();

      sb << "  \"" << parent->canonName() << "\" -> \""
                   << obj->canonName() << "\" [\n";
      if (parent->isVirtual) {
        sb << "    style = dashed\n";    // virtual inheritance: dashed link
      }
      sb << "  ]\n";
    }
  }

  sb << "}\n\n";

  return sb;
}


string toString(CompoundType::Keyword k)
{
  xassert((unsigned)k < (unsigned)CompoundType::NUM_KEYWORDS);
  return string(typeIntrNames[k]);    // see cc-type.h
}


int CompoundType::countBaseClassSubobjects(CompoundType const *ct) const
{
  SObjList<BaseClassSubobj const> objs;
  getSubobjects(objs);

  int count = 0;
  SFOREACH_OBJLIST(BaseClassSubobj const, objs, iter) {
    if (iter.data()->ct == ct) {
      count++;
    }
  }

  // Count anonymous compound members as being subobjects for this,
  // since the goal is simply to make sure there is exactly one within
  // the type where lookup was performed.
  SFOREACH_OBJLIST(Variable, dataMembers, iter) {
    Variable const *member = iter.data();
    if (member->name == NULL) {
      if (CompoundType const *memberCT = member->type->ifCompoundTypeC()) {
        if (memberCT == ct) {
          count++;
        }
        else {
          count += memberCT->countBaseClassSubobjects(ct);
        }
      }
    }
  }

  return count;
}


// simple recursive computation
void getBaseClasses(SObjSet<CompoundType*> &bases, CompoundType *ct)
{
  bases.add(ct);

  FOREACH_OBJLIST(BaseClass, ct->bases, iter) {
    getBaseClasses(bases, iter.data()->ct);
  }
}

STATICDEF CompoundType *CompoundType::lub
  (CompoundType *t1, CompoundType *t2, bool &wasAmbig)
{
  wasAmbig = false;

  // might be a common case?
  if (t1 == t2) {
    return t1;
  }

  // compute the set of base classes for each class
  SObjSet<CompoundType*> t1Bases, t2Bases;
  getBaseClasses(t1Bases, t1);
  getBaseClasses(t2Bases, t2);

  // look for an element in the intersection that has nothing below it
  // (hmm.. this will be quadratic due to linear-time 'hasBaseClass'...)
  CompoundType *least = NULL;
  {
    for (SObjSetIter<CompoundType*> iter(t1Bases); !iter.isDone(); iter.adv()) {
      if (!t2Bases.contains(iter.data())) continue;       // filter for intersection

      if (!least ||
          iter.data()->hasBaseClass(least)) {
        // new least
        least = iter.data();
      }
    }
  }

  if (!least) {
    return NULL;        // empty intersection
  }

  // check that it's the unique least
  for (SObjSetIter<CompoundType*> iter(t1Bases); !iter.isDone(); iter.adv()) {
    if (!t2Bases.contains(iter.data())) continue;       // filter for intersection

    if (least->hasBaseClass(iter.data())) {
      // least is indeed less than (or equal to) this one
    }
    else {
      // counterexample; 'least' is not in fact the least
      wasAmbig = true;
      return NULL;
    }
  }

  // good to go
  return least;
}


void CompoundType::finishedClassDefinition(StringRef specialName)
{
  // get inherited conversions
  FOREACH_OBJLIST(BaseClass, bases, iter) {
    conversionOperators.appendAll(iter.data()->ct->conversionOperators);
  }

  // remove duplicates (in/t0483.cc); if there is an ambiguity because
  // of inheriting something multiple times, it should be detected when
  // we go to actually *use* the conversion operator
  conversionOperators.removeDuplicatesAsPointerMultiset();

  // add those declared locally; they hide any that are inherited
  Variable *localOps = rawLookupVariable(specialName);
  if (!localOps) {
    return;      // nothing declared locally
  }

  if (!localOps->overload) {
    addLocalConversionOp(localOps);
  }
  else {
    SFOREACH_OBJLIST_NC(Variable, localOps->overload->set, iter) {
      addLocalConversionOp(iter.data());
    }
  }
}

void CompoundType::addLocalConversionOp(Variable *op)
{
  Type *opRet = op->type->asFunctionTypeC()->retType;

  // remove any existing conversion operators that yield the same type
  {
    SObjListMutator<Variable> mut(conversionOperators);
    while (!mut.isDone()) {
      Type *mutRet = mut.data()->type->asFunctionTypeC()->retType;

      if (mutRet->equals(opRet)) {
        mut.remove();    // advances the iterator
      }
      else {
        mut.adv();
      }
    }
  }

  // add 'op'
  conversionOperators.append(op);
}


// return false if the presence of 'v' in a CompoundType
// prevents that compound from being "aggregate"
static bool isAggregate_one(Variable const *v)
{
  // typedef and static members are irrelevant
  if (v->isType() || v->hasFlag(DF_STATIC)) {
    return true;
  }

  if (!v->hasFlag(DF_IMPLICIT) &&
      v->type->isFunctionType() &&
      v->type->asFunctionTypeC()->isConstructor()) {
    // user-defined (non-DF_IMPLICIT) constructor
    return false;
  }

  if (v->hasFlag(DF_VIRTUAL)) {
    // virtual function
    return false;
  }

  if (v->getAccess() != AK_PUBLIC &&
      !v->type->isFunctionType()) {
    // non-public data
    return false;
  }

  // does not prevent container from being "aggregate"
  return true;
}


// conditions:
//   - no user-defined constructors
//   - no private or protected non-static data members
//   - no base classes
//   - no virtual functions
bool CompoundType::isAggregate() const
{
  StringRefMap<Variable>::Iter iter(getVariableIter());
  for (; !iter.isDone(); iter.adv()) {
    Variable const *v = iter.value();

    if (v->isOverloaded()) {
      SFOREACH_OBJLIST(Variable, v->overload->set, iter2) {
        if (!isAggregate_one(iter2.data())) {
          return false;
        }
      }
    }
    else {
      if (!isAggregate_one(v)) {
        return false;
      }
    }
  }

  if (bases.isNotEmpty()) {
    return false;
  }

  return true;
}


// C11 6.7.2.1p18 also says that there must be at least two named
// members, and only the last can be an array with missing size.  Those
// additional constraints are not enforced here.
bool CompoundType::hasFlexibleArrayMember() const
{
  if (!isUnion() && dataMembers.isNotEmpty()) {
    Variable const *last = dataMembers.lastC();
    return last->type->isArrayTypeWithUnspecifiedSize();
  }

  return false;
}


// ---------------- EnumType ------------------
EnumType::~EnumType()
{}


string EnumType::toCString() const
{
  return stringc << "enum " << (name? name : "/*anonymous*/");
}


int EnumType::reprSize(TypeSizes const &typeSizes) const
{
  // this is the usual choice
  return simpleTypeReprSize(typeSizes, ST_INT);
}


EnumType::Value *EnumType::addValue(StringRef name, int value, Variable *decl)
{
  xassert(!valueIndex.isMapped(name));

  Value *v = new Value(name, this, value, decl);
  valueIndex.add(name, v);

  // 7/22/04: At one point I was also maintaining a linked list of
  // the Value objects.  Daniel pointed out this was quadratic b/c
  // I was using 'append()'.  Since I never used the list anyway,
  // I just dropped it in favor of the dictionary (only).

  if (value < 0) {
    hasNegativeValues = true;
  }

  return v;
}


EnumType::Value const * NULLABLE EnumType::getValue(StringRef name) const
{
  Value const *v;
  if (valueIndex.queryC(name, v)) {
    return v;
  }
  else {
    return NULL;
  }
}


// ---------------- EnumType::Value --------------
EnumType::Value::Value(StringRef n, EnumType *t, int v, Variable *d)
  : name(n), type(t), value(v), decl(d)
{}

EnumType::Value::~Value()
{}


// -------------------- TypePred ----------------------
bool StatelessTypePred::operator() (Type const *t)
{
  return f(t);
}


// -------------------- BaseType ----------------------
ALLOC_STATS_DEFINE(BaseType)

bool BaseType::printAsML = false;


BaseType::BaseType()
{
  ALLOC_STATS_IN_CTOR
}

BaseType::~BaseType()
{
  ALLOC_STATS_IN_DTOR
}


DOWNCAST_IMPL(BaseType, CVAtomicType)
DOWNCAST_IMPL(BaseType, PointerType)
DOWNCAST_IMPL(BaseType, ReferenceType)
DOWNCAST_IMPL(BaseType, FunctionType)
DOWNCAST_IMPL(BaseType, ArrayType)
DOWNCAST_IMPL(BaseType, PointerToMemberType)


bool BaseType::isTypedefType() const
{
  return false;
}

DOWNCAST_IMPL(BaseType, TypedefType)


static Type const *baseTypeToType(BaseType const *bt)
{
  // The constructor of BaseType is private, with Type as its only
  // friend, therefore Type is the only possible derived class.
  return static_cast<Type const *>(bt);
}


Type const *BaseType::skipTypedefsC() const
{
  int ct=0;
  Type const *t = baseTypeToType(this);
  while (TypedefType const *tt = t->ifTypedefTypeC()) {
    t = tt->underlyingType();

    // Catch an infinite loop.
    xassert(++ct < 1000);
  }
  return t;
}


bool BaseType::equals(BaseType const *obj, MatchFlags flags) const
{
  MType mtype;
  return mtype.matchType(baseTypeToType(this),
                         baseTypeToType(obj), flags);
}

unsigned BaseType::hashValue() const
{
  unsigned h = innerHashValue();

  // 'h' now has quite a few bits of entropy, but they're mostly
  // in the high bits.  push it through a PRNG to mix it better.
  return lcprngTwoSteps_inline(h);
}


string cvToString(CVFlags cv)
{
  if (cv != CV_NONE) {
    return stringc << " " << toString(cv);
  }
  else {
    return string("");
  }
}


void BaseType::gdb() const
{
  cout << toString() << endl;
}

string BaseType::toString() const
{
  if (printAsML) {
    return toMLString();
  }
  else {
    return toCString();
  }
}


string BaseType::toCString() const
{
  if (isCVAtomicType()) {
    // special case a single atomic type, so as to avoid
    // printing an extra space
    CVAtomicType const *cvatomic = asCVAtomicTypeC();
    return stringc
      << cvatomic->atomic->toCString()
      << cvToString(cvatomic->cv);
  }
  else {
    return stringc
      << leftString()
      << rightString();
  }
}

string BaseType::toCString(char const *name) const
{
  // print the inner parentheses if the name is omitted
  bool innerParen = (name && name[0])? false : true;

  #if 0    // wrong
  // except, if this type is a pointer, then omit the parens anyway;
  // we only need parens when the type is a function or array and
  // the name is missing
  if (isPointerType()) {
    innerParen = false;
  }
  #endif // 0
  stringBuilder s;
  s << leftString(innerParen);
  if (name) {
    s << name;
  }
  else if (printAnonComment) {
    s << "/*anon*/";
  }
  s << rightString(innerParen);
  return s;
}

// this is only used by CVAtomicType.. all others override it
string BaseType::rightString(bool /*innerParen*/) const
{
  return "";
}


bool BaseType::anyCtorSatisfiesF(TypePredFunc f) const
{
  StatelessTypePred stp(f);
  return anyCtorSatisfies(stp);
}


CVFlags BaseType::getCVFlags() const
{
  return CV_NONE;
}


bool BaseType::isSimpleType() const
{
  if (isCVAtomicType()) {
    AtomicType const *at = asCVAtomicTypeC()->atomic;
    return at->isSimpleType();
  }
  else {
    return false;
  }
}

SimpleType const *BaseType::asSimpleTypeC() const
{
  return asCVAtomicTypeC()->atomic->asSimpleTypeC();
}

bool BaseType::isSimple(SimpleTypeId id) const
{
  return isSimpleType() &&
         asSimpleTypeC()->type == id;
}

bool BaseType::isSomeKindOfCharType() const
{
  return
    isSomeNarrowCharType()     ||
    isSimple(ST_WCHAR_T);
}

bool BaseType::isSomeNarrowCharType() const
{
  return
    isSimple(ST_CHAR)          ||
    isSimple(ST_UNSIGNED_CHAR) ||
    isSimple(ST_SIGNED_CHAR);
}

bool BaseType::isStringType() const
{
  return isArrayType() &&
    (asArrayTypeC()->eltType->isSimpleCharType() ||
     asArrayTypeC()->eltType->isSimpleWChar_tType());
}

bool BaseType::isIntegerType() const
{
  return isSimpleType() &&
         ::isIntegerType(asSimpleTypeC()->type);
}


bool BaseType::isEnumType() const
{
  return isCVAtomicType() &&
         asCVAtomicTypeC()->atomic->isEnumType();
}


bool BaseType::isDependent() const
{
  // 14.6.2.1: type variables (template parameters) are the base case
  // for dependent types
  return isSimple(ST_DEPENDENT) ||
         isTypeVariable();
}

bool BaseType::isGeneralizedDependent() const
{
  if (isSimple(ST_DEPENDENT) ||
      isTypeVariable() ||
      isPseudoInstantiation() ||
      isDependentQType()) {
    return true;
  }

  // 10/09/04: (in/d0113.cc) I want to regard A<T>::B, where B is an
  // inner class of template class A, as being dependent.  For that
  // matter, A itself should be regarded as dependent.  So if 'ct'
  // has template parameters (including inherited), say it is
  // dependent.
  //
  // 2005-05-06: I removed the following because I think it is wrong.
  // The name 'A' (if found in, say, the global scope) is not dependent,
  // though 'A<T>' might be (if T is an uninst template param).  I think
  // in/d0113.cc was just invalid, and I fixed it.
  //
  // 2005-08-12: Ok, I think I want to call it dependent *only* on the
  // basis of whether there are *inherited* params, as in A<T>::B.
  // A<T> itself would be a PseudoInstantiation (and therefore
  // dependent), while just A would not be dependent, despite having
  // (non-inherited) parameters.  (in/t0551.cc)
  if (isCompoundType()) {
    TemplateInfo *ti = asCompoundTypeC()->templateInfo();
    if (ti && ti->hasInheritedParameters()) {
      return true;
    }
  }

  return false;
}

static bool cGD_helper(Type const *t)
{
  return t->isGeneralizedDependent();
}

// What is the intended difference in usage between
// 'isGeneralizedDependent' and 'containsGeneralizedDependent'?
// Mostly, the latter includes types that cannot be classes even when
// template args are supplied, whereas the former includes only types
// that *might* be classes when targs are supplied.  (TODO: write up
// the entire design of dependent type representation)
bool BaseType::containsGeneralizedDependent() const
{
  return anyCtorSatisfiesF(cGD_helper);
}


bool BaseType::isCompoundTypeOf(CompoundType::Keyword keyword) const
{
  if (isCVAtomicType()) {
    CVAtomicType const *a = asCVAtomicTypeC();
    return a->atomic->isCompoundType() &&
           a->atomic->asCompoundTypeC()->keyword == keyword;
  }
  else {
    return false;
  }
}

NamedAtomicType *BaseType::ifNamedAtomicType()
{
  return isNamedAtomicType()? asNamedAtomicType() : NULL;
}

CompoundType const *BaseType::ifCompoundTypeC() const
{
  return isCompoundType()? asCompoundTypeC() : NULL;
}

NamedAtomicType const *BaseType::asNamedAtomicTypeC() const
{
  return asCVAtomicTypeC()->atomic->asNamedAtomicTypeC();
}

CompoundType const *BaseType::asCompoundTypeC() const
{
  return asCVAtomicTypeC()->atomic->asCompoundTypeC();
}

PseudoInstantiation const *BaseType::asPseudoInstantiationC() const
{
  return asCVAtomicTypeC()->atomic->asPseudoInstantiationC();
}


bool BaseType::isMethod() const
{
  return isFunctionType() &&
         asFunctionTypeC()->isMethod();
}

bool BaseType::isArrayTypeWithUnspecifiedSize() const
{
  if (ArrayType const *at = this->ifArrayTypeC()) {
    return at->getSize() == ArrayType::NO_SIZE;
  }
  return false;
}

bool BaseType::isVariableLengthArrayType() const
{
  if (ArrayType const *at = this->ifArrayTypeC()) {
    return at->getSize() == ArrayType::DYN_SIZE;
  }
  return false;
}


bool BaseType::isFloatingType() const
{
  if (CVAtomicType const *cvAtomic = this->ifCVAtomicTypeC()) {
    if (SimpleType const *simple = cvAtomic->atomic->ifSimpleTypeC()) {
      return isFloatType(simple->type);
    }
  }
  return false;
}


bool BaseType::isReferenceToConst() const {
  return isReferenceType() && asReferenceTypeC()->atType->isConst();
}

bool BaseType::isPointerOrArrayRValueType() const {
  return asRvalC()->isPointerType() || asRvalC()->isArrayType();
}

// FIX: this is silly; it should be made into a virtual dispatch
Type *BaseType::getAtType() const
{
  if (isPointerType()) {
    return asPointerTypeC()->atType;
  }
  else if (isReferenceType()) {
    return asReferenceTypeC()->atType;
  }
  else if (isArrayType()) {
    return asArrayTypeC()->eltType;
  }
  else if (isPointerToMemberType()) {
    return asPointerToMemberTypeC()->atType;
  }
  else {
    xfailure("illegal call to getAtType");
    return NULL;       // silence warning
  }
}

BaseType const *BaseType::asRvalC() const
{
  if (isReference()) {
    // note that due to the restriction about stacking reference
    // types, unrolling more than once is never necessary
    return asReferenceTypeC()->atType;
  }
  else {
    return this;       // this possibility is one reason to not make 'asRval' return 'Type*' in the first place
  }
}


bool BaseType::isCVAtomicType(AtomicType::Tag tag) const
{
  return isCVAtomicType() &&
         asCVAtomicTypeC()->atomic->getTag() == tag;
}


TypeVariable const *BaseType::asTypeVariableC() const
{
  return asCVAtomicTypeC()->atomic->asTypeVariableC();
}


bool BaseType::isNamedAtomicType() const {
  return isCVAtomicType() && asCVAtomicTypeC()->atomic->isNamedAtomicType();
}


bool typeIsError(Type const *t)
{
  return t->isError();
}

bool BaseType::containsErrors() const
{
  return anyCtorSatisfiesF(typeIsError);
}


bool typeHasTypeVariable(Type const *t)
{
  return t->isTypeVariable() ||
         (t->isCompoundType() &&
          t->asCompoundTypeC()->isTemplate(true /*considerInherited*/)) ||
         t->isPseudoInstantiation();
}

bool BaseType::containsTypeVariables() const
{
  return anyCtorSatisfiesF(typeHasTypeVariable);
}


// TODO: sm: I think it's wrong to have both 'hasTypeVariable' and
// 'hasVariable', since I think any use of the former actually wants
// to be a use of the latter.


// ------------------ ContainsVariablesPred ------------------
class ContainsVariablesPred : public TypePred {
public:      // data
  MType *map;        // nullable serf

private:     // funcs
  bool isUnbound(StringRef name);

public:      // funcs
  ContainsVariablesPred(MType *m) : map(m) {}

  virtual bool operator() (Type const *t) override;
  bool atomicTypeHasVariable(AtomicType const *t);
  bool nameContainsVariables(PQName const *name);
};


bool ContainsVariablesPred::isUnbound(StringRef name)
{
  return !map || !map->isBound(name);
}


bool ContainsVariablesPred::atomicTypeHasVariable(AtomicType const *t)
{
  if (t->isTypeVariable()) {
    return isUnbound(t->asTypeVariableC()->name);
  }

  if (t->isPseudoInstantiation()) {
    PseudoInstantiation const *pi = t->asPseudoInstantiationC();
    return atomicTypeHasVariable(pi->primary) ||
           containsVariables(pi->args, map);
  }

  if (t->isDependentQType()) {
    DependentQType const *dqt = t->asDependentQTypeC();
    return atomicTypeHasVariable(dqt->first) ||
           nameContainsVariables(dqt->rest);
  }

  if (t->isCompoundType() &&
      t->asCompoundTypeC()->isTemplate(true /*considerInherited*/)) {
    return true;

    // 2005-08-12: this was here, but I think it is wrong
    //return containsVariables(t->asCompoundTypeC()->templateInfo()->arguments, map);
  }

  return false;
}


bool ContainsVariablesPred::nameContainsVariables(PQName const *name)
{
  ASTSWITCHC(PQName, name) {
    ASTCASEC(PQ_template, t) {
      return containsVariables(t->sargs, map);
    }
    ASTNEXTC(PQ_qualifier, q) {
      return containsVariables(q->sargs, map) ||
             nameContainsVariables(q->rest);
    }
    ASTDEFAULTC {
      return false;
    }
    ASTENDCASEC
  }
}


bool ContainsVariablesPred::operator() (Type const *t)
{
  if (t->isCVAtomicType()) {
    return atomicTypeHasVariable(t->asCVAtomicTypeC()->atomic);
  }
  else if (t->isPointerToMemberType()) {
    return atomicTypeHasVariable(t->asPointerToMemberTypeC()->inClassNAT);
  }

  return false;
}


bool BaseType::containsVariables(MType *map) const
{
  ContainsVariablesPred cvp(map);
  return anyCtorSatisfies(cvp);
}


string toString(Type const *t)
{
  return t->toString();
}


// ----------------- CVAtomicType ----------------
// map cv down to {0,1,2,3}
inline unsigned cvHash(CVFlags cv)
{
  return cv >> CV_SHIFT_AMOUNT;
}

unsigned CVAtomicType::innerHashValue() const
{
  // underlying atomic is pointer-based equality
  return (unsigned)(uintptr_t)atomic +
         cvHash(cv);
         // T_ATOMIC is zero anyway
}


string CVAtomicType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atomic->toCString();
  s << cvToString(cv);

  // this is the only mandatory space in the entire syntax
  // for declarations; it separates the type specifier from
  // the declarator(s)
  s << " ";

  return s;
}


int CVAtomicType::reprSize(TypeSizes const &typeSizes) const
{
  return atomic->reprSize(typeSizes);
}


bool CVAtomicType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this);
}


CVFlags CVAtomicType::getCVFlags() const
{
  return cv;
}


// ------------------- PointerType ---------------
PointerType::PointerType(CVFlags c, Type *a)
  : cv(c), atType(a)
{
  // it makes no sense to stack reference operators underneath
  // other indirections (i.e. no ptr-to-ref, nor ref-to-ref)
  xassert(!a->isReference());
}


enum {
  // enough to leave room for a composed type to describe itself
  // (essentially 5 bits)
  HASH_KICK = 33,

  // put the tag in the upper part of those 5 bits
  TAG_KICK = 7
};

unsigned PointerType::innerHashValue() const
{
  // The essential strategy for composed types is to multiply the
  // underlying type's hash by HASH_KICK, pushing the existing bits up
  // (but with mixing), and then add our local info, including the
  // tag.
  //
  // I don't claim to be an expert at designing hash functions.  There
  // are probably better ones; if you think you know one, let me know.
  // My plan is to watch the hash outputs produced during operator
  // overload resolution, and tweak the function if I see an obvious
  // avenue for improvement.

  return atType->innerHashValue() * HASH_KICK +
         cvHash(cv) +
         T_POINTER * TAG_KICK;
}


string PointerType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atType->leftString(false /*innerParen*/);
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << "(";
  }
  s << "*";
  if (cv) {
    // 1/03/03: added this space so "Foo * const arf" prints right (t0012.cc)
    s << cvToString(cv) << " ";
  }
  return s;
}

string PointerType::rightString(bool /*innerParen*/) const
{
  stringBuilder s;
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << ")";
  }
  s << atType->rightString(false /*innerParen*/);
  return s;
}


int PointerType::reprSize(TypeSizes const &typeSizes) const
{
  return typeSizes.getSize(STS_POINTER);
}


bool PointerType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


CVFlags PointerType::getCVFlags() const
{
  return cv;
}


// ------------------- ReferenceType ---------------
ReferenceType::ReferenceType(Type *a)
  : atType(a)
{
  // it makes no sense to stack reference operators underneath
  // other indirections (i.e. no ptr-to-ref, nor ref-to-ref)
  xassert(!a->isReference());
}


unsigned ReferenceType::innerHashValue() const
{
  return atType->innerHashValue() * HASH_KICK +
         T_REFERENCE * TAG_KICK;
}


string ReferenceType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atType->leftString(false /*innerParen*/);
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << "(";
  }
  s << "&";
  return s;
}

string ReferenceType::rightString(bool /*innerParen*/) const
{
  stringBuilder s;
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << ")";
  }
  s << atType->rightString(false /*innerParen*/);
  return s;
}

int ReferenceType::reprSize(TypeSizes const &typeSizes) const
{
  return typeSizes.getSize(STS_POINTER);
}


bool ReferenceType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


CVFlags ReferenceType::getCVFlags() const
{
  return CV_NONE;
}


// -------------------- FunctionType::ExnSpec --------------
FunctionType::ExnSpec::ExnSpec(ExnSpec const &obj)
{
  // copy list contents
  types = obj.types;
}


FunctionType::ExnSpec::~ExnSpec()
{
  types.removeAll();
}


bool FunctionType::ExnSpec::anyCtorSatisfies(TypePred &pred) const
{
  SFOREACH_OBJLIST(Type, types, iter) {
    if (iter.data()->anyCtorSatisfies(pred)) {
      return true;
    }
  }
  return false;
}


// -------------------- FunctionType -----------------
FunctionType::FunctionType(Type *r)
  : flags(FF_NONE),
    retType(r),
    params(),
    exnSpec(NULL)
{}


FunctionType::~FunctionType()
{
  if (exnSpec) {
    delete exnSpec;
  }
}


// sm: I moved 'isCopyConstructorFor' and 'isCopyAssignOpFor' out into
// cc-tcheck.cc since the rules are fairly specific to the analysis
// being performed there


bool FunctionType::paramsHaveDefaultsPast(int startParam) const
{
  SObjListIter<Variable> iter(params);

  // count off 'startParams' parameters that are not tested
  while (!iter.isDone() && startParam>0) {
    iter.adv();
    startParam--;
  }

  // all remaining parameters must accept defaults
  for (; !iter.isDone(); iter.adv()) {
    if (!iter.data()->value) {
      return false;     // doesn't have a default value
    }
  }

  return true;          // all params after 'startParam' have defaults
}


unsigned FunctionType::innerHashValue() const
{
  // process return type similarly to how other constructed types
  // handle their underlying type
  unsigned val = retType->innerHashValue() * HASH_KICK +
                 params.count() +
                 T_FUNCTION * TAG_KICK;

  // now factor in the parameter types
  int ct = 1;
  SFOREACH_OBJLIST(Variable, params, iter) {
    // similar to return value
    unsigned p = iter.data()->type->innerHashValue() * HASH_KICK +
                 (ct + T_FUNCTION) * TAG_KICK;

    // multiply with existing hash, sort of like each parameter
    // is another dimension and we're constructing a tuple in
    // some space
    val *= p;
  }

  // don't consider exnSpec or templateInfo; I don't think I'll be
  // encountering situations where I want the hash to be sensitive to
  // those

  // the 'flags' information is mostly redundant with the parameter
  // list, so don't bother folding that in

  return val;
}


void FunctionType::addParam(Variable *param)
{
  xassert(param->hasFlag(DF_PARAMETER));
  params.append(param);
}

void FunctionType::addReceiver(Variable *param)
{
  xassert(param->type->isReference());
  xassert(param->hasFlag(DF_PARAMETER));
  xassert(!isConstructor());    // ctors don't get a __receiver param
  xassert(!isMethod());         // this is about to change below
  params.prepend(param);
  setFlag(FF_METHOD);
}


Variable const *FunctionType::getReceiverC() const
{
  xassert(isMethod());
  Variable const *ret = params.firstC();
  xassert(ret->getTypeC()->isReference());
  return ret;
}

CVFlags FunctionType::getReceiverCV() const
{
  if (isMethod()) {
    // expect 'this' to be of type 'SomeClass cv &', and
    // dig down to get that 'cv'
    return getReceiverC()->type->asReferenceType()->atType->asCVAtomicType()->cv;
  }
  else {
    return CV_NONE;
  }
}

CompoundType *FunctionType::getClassOfMember()
{
  return getReceiver()->type->asReferenceType()->atType->asCompoundType();
}

NamedAtomicType *FunctionType::getNATOfMember()
{
  return getReceiver()->type->asReferenceType()->atType->asNamedAtomicType();
}


SObjListIter<Variable> FunctionType::nonReceiverParamIterC() const
{
  SObjListIter<Variable> ret(this->params);
  if (this->isMethod()) {
    xassert(!ret.isDone());
    ret.adv();
  }
  return ret;
}


SObjListIterNC<Variable> FunctionType::nonReceiverParamIterNC()
{
  SObjListIterNC<Variable> ret(this->params);
  if (this->isMethod()) {
    xassert(!ret.isDone());
    ret.adv();
  }
  return ret;
}


string FunctionType::leftString(bool innerParen) const
{
  stringBuilder sb;

  // FIX: FUNC TEMPLATE LOSS
  // template parameters
//    if (templateInfo) {
//      sb << templateInfo->paramsToCString() << " ";
//    }

  // return type and start of enclosing type's description
  if (flags & (/*FF_CONVERSION |*/ FF_CTOR | FF_DTOR)) {
    // don't print the return type, it's implicit

    // 7/18/03: changed so we print ret type for FF_CONVERSION,
    // since otherwise I can't tell what it converts to!
  }
  else {
    sb << retType->leftString();
  }

  // NOTE: we do *not* propagate 'innerParen'!
  if (innerParen) {
    sb << "(";
  }

  return sb;
}

string FunctionType::rightString(bool innerParen) const
{
  // I split this into two pieces because the Cqual++ concrete
  // syntax puts $tainted into the middle of my rightString,
  // since it's following the placement of 'const' and 'volatile'
  return stringc
    << rightStringUpToQualifiers(innerParen)
    << rightStringQualifiers(getReceiverCV())
    << rightStringAfterQualifiers();
}

string FunctionType::rightStringUpToQualifiers(bool innerParen) const
{
  // finish enclosing type
  stringBuilder sb;
  if (innerParen) {
    sb << ")";
  }

  // arguments
  sb << "(";
  int ct=0;
  SFOREACH_OBJLIST(Variable, params, iter) {
    ct++;
    if (isMethod() && ct==1) {
      // don't actually print the first parameter;
      // the 'm' stands for nonstatic member function
      sb << "/""*m: " << iter.data()->type->toCString() << " *""/ ";
      continue;
    }
    if (ct >= 3 || (!isMethod() && ct>=2)) {
      sb << ", ";
    }
    sb << iter.data()->toCStringAsParameter();
  }

  if (acceptsVarargs()) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << "...";
  }

  sb << ")";

  return sb;
}

STATICDEF string FunctionType::rightStringQualifiers(CVFlags cv)
{
  if (cv) {
    return stringc << " " << ::toString(cv);
  }
  else {
    return "";
  }
}

string FunctionType::rightStringAfterQualifiers() const
{
  stringBuilder sb;

  // exception specs
  if (exnSpec) {
    sb << " throw(";
    int ct=0;
    SFOREACH_OBJLIST(Type, exnSpec->types, iter) {
      if (ct++ > 0) {
        sb << ", ";
      }
      sb << iter.data()->toCString();
    }
    sb << ")";
  }

  // hook for verifier syntax
  extraRightmostSyntax(sb);

  // finish up the return type
  sb << retType->rightString();

  return sb;
}

void FunctionType::extraRightmostSyntax(stringBuilder &) const
{
}


string FunctionType::toString_withCV(CVFlags cv) const
{
  return stringc
    << leftString(true /*innerParen*/)
    << rightStringUpToQualifiers(true /*innerParen*/)
    << rightStringQualifiers(cv)
    << rightStringAfterQualifiers();
}


int FunctionType::reprSize(TypeSizes const &typeSizes) const
{
  // thinking here about how this works when we're summing
  // the fields of a class with member functions ..
  return 0;
}


bool parameterListCtorSatisfies(TypePred &pred,
                                SObjList<Variable> const &params)
{
  SFOREACH_OBJLIST(Variable, params, iter) {
    if (iter.data()->type->anyCtorSatisfies(pred)) {
      return true;
    }
  }
  return false;
}

bool FunctionType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         retType->anyCtorSatisfies(pred) ||
         parameterListCtorSatisfies(pred, params) ||
         (exnSpec && exnSpec->anyCtorSatisfies(pred));
  // FIX: FUNC TEMPLATE LOSS
  //
  // UPDATE: this is actually symmetric with the way that compound
  // type templates are dealt with, which is to say we do not recurse
  // on the parameters
//      || (templateInfo && templateInfo->anyParamCtorSatisfies(pred));
}


// -------------------- ArrayType ------------------
void ArrayType::checkWellFormedness() const
{
  xassert(eltType);
  xassert(!eltType->isReference());
}


unsigned ArrayType::innerHashValue() const
{
  // similar to PointerType
  return eltType->innerHashValue() * HASH_KICK +
         size +
         T_ARRAY * TAG_KICK;
}


string ArrayType::leftString(bool /*innerParen*/) const
{
  return eltType->leftString();
}

string ArrayType::rightString(bool /*innerParen*/) const
{
  stringBuilder sb;

  if (hasSize()) {
    sb << "[" << size << "]";
  }
  else {
    sb << "[]";
  }

  sb << eltType->rightString();

  return sb;
}


int ArrayType::reprSize(TypeSizes const &typeSizes) const
{
  if (!hasSize()) {
    throw_XReprSize(size == DYN_SIZE /*isDynamic*/);
  }

  // TODO: This does not take alignment into account.
  return eltType->reprSize(typeSizes) * size;
}


bool ArrayType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         eltType->anyCtorSatisfies(pred);
}



// ---------------- PointerToMemberType ---------------
PointerToMemberType::PointerToMemberType(NamedAtomicType *inClassNAT0, CVFlags c, Type *a)
  : inClassNAT(inClassNAT0), cv(c), atType(a)
{
  // 'inClassNAT' should always be a compound or something dependent
  // on a type variable; basically, it should just not be an enum,
  // given that it is a named atomic type
  xassert(!inClassNAT->isEnumType());

  // cannot have pointer to reference type
  xassert(!a->isReference());

  // there are some other semantic restrictions, but I let the
  // type checker enforce them
}


CompoundType *PointerToMemberType::inClass() const {
  if (inClassNAT->isCompoundType()) {
    return inClassNAT->asCompoundType();
  }
  xfailure("Request for the inClass member of a PointerToMemberType "
           "as if it were a CompoundType which it is not in this case.");
}


unsigned PointerToMemberType::innerHashValue() const
{
  return atType->innerHashValue() * HASH_KICK +
         cvHash(cv) +
         T_POINTERTOMEMBER * TAG_KICK +
         (unsigned)(uintptr_t)inClass();   // we'll see...
}


string PointerToMemberType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atType->leftString(false /*innerParen*/);
  s << " ";
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << "(";
  }
  s << inClassNAT->name << "::*";
  s << cvToString(cv);
  return s;
}

string PointerToMemberType::rightString(bool /*innerParen*/) const
{
  stringBuilder s;
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << ")";
  }
  s << atType->rightString(false /*innerParen*/);
  return s;
}


int PointerToMemberType::reprSize(TypeSizes const &typeSizes) const
{
  return typeSizes.getSize(STS_POINTER_TO_MEMBER);
}


bool PointerToMemberType::anyCtorSatisfies(TypePred &pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


CVFlags PointerToMemberType::getCVFlags() const
{
  return cv;
}


// --------------------------- TypedefType -----------------------------
bool TypedefType::s_printTypedefComments = true;


TypedefType::TypedefType(Variable *typedefVar, CVFlags cv, Type *type)
  : m_typedefVar(typedefVar),
    m_cv(cv),
    m_type(type)
{
  xassert(typedefVar->isType());
  xassert(type);
}


Type *TypedefType::underlyingType() const
{
  return m_type;
}


Type::Tag TypedefType::getTag() const
{
  return underlyingType()->getTag();
}


#define TYPEDEFTYPE_DOWNCAST_IMPL(destType)            \
  destType const *TypedefType::as##destType##C() const \
  {                                                    \
    return underlyingType()->as##destType##C();        \
  }                                                    \
  destType const *TypedefType::if##destType##C() const \
  {                                                    \
    return underlyingType()->if##destType##C();        \
  }

TYPEDEFTYPE_DOWNCAST_IMPL(CVAtomicType)
TYPEDEFTYPE_DOWNCAST_IMPL(PointerType)
TYPEDEFTYPE_DOWNCAST_IMPL(ReferenceType)
TYPEDEFTYPE_DOWNCAST_IMPL(FunctionType)
TYPEDEFTYPE_DOWNCAST_IMPL(ArrayType)
TYPEDEFTYPE_DOWNCAST_IMPL(PointerToMemberType)

#undef TYPEDEFTYPE_DOWNCAST_IMPL


bool TypedefType::isTypedefType() const
{
  return true;
}

TypedefType const *TypedefType::asTypedefTypeC() const
{
  return static_cast<TypedefType const*>(this);
}

TypedefType const *TypedefType::ifTypedefTypeC() const
{
  return static_cast<TypedefType const*>(this);
}


unsigned      TypedefType::innerHashValue() const
{
  return underlyingType()->innerHashValue();
}

string        TypedefType::toMLString() const
{
  return underlyingType()->toMLString();
}

string TypedefType::leftString(bool innerParen) const
{
  if (s_printTypedefComments) {
    return stringb("/*" << stripComments(m_typedefVar->name) << "*/" <<
                   underlyingType()->leftString(innerParen));
  }
  else {
    return underlyingType()->leftString(innerParen);
  }
}

string        TypedefType::rightString(bool innerParen) const
{
  return underlyingType()->rightString(innerParen);
}

int           TypedefType::reprSize(TypeSizes const &typeSizes) const
{
  return underlyingType()->reprSize(typeSizes);
}

bool          TypedefType::anyCtorSatisfies(TypePred &pred) const
{
  return underlyingType()->anyCtorSatisfies(pred);
}

CVFlags       TypedefType::getCVFlags() const
{
  return underlyingType()->getCVFlags();
}

Type *        TypedefType::getAtType() const
{
  return underlyingType()->getAtType();
}


// ---------------------- toMLString ---------------------
// print out a type as an ML-style string

//  Atomic
string SimpleType::toMLString() const
{
  return simpleTypeName(type);
}

string CompoundType::toMLString() const
{
  stringBuilder sb;

//    bool hasParams = templateInfo && templateInfo->params.isNotEmpty();
//    if (hasParams) {
  TemplateInfo *tinfo = templateInfo();
  if (tinfo) {
    sb << tinfo->paramsToMLString();
  }

//    if (!templateInfo || hasParams) {
    // only say 'class' if this is like a class definition, or
    // if we're not a template, since template instantiations
    // usually don't include the keyword 'class' (this isn't perfect..
    // I think I need more context)
  sb << keywordName(keyword) << "-";
//    }

  if (name) {
    sb << "'" << name << "'";
  } else {
    sb << "*anonymous*";
  }

  // template arguments are now in the name
  //if (templateInfo && templateInfo->specialArguments) {
  //  sb << "<" << templateInfo->specialArgumentsRepr << ">";
  //}

  return sb;
}

string EnumType::toMLString() const
{
  stringBuilder sb;
  sb << "enum-";
  if (name) {
    sb << "'" << name << "'";
  } else {
    sb << "*anonymous*";
  }
  return sb;
}

string TypeVariable::toMLString() const
{
  return stringc << "typevar-'" << string(name) << "'";
}

string PseudoInstantiation::toMLString() const
{
  return stringc << "psuedoinstantiation-'" << primary->name << "'"
                 << sargsToString(args);      // sm: ?
}

//  Constructed

// I don't like #if-s everywhere, and Steve Fink agrees with me.
void BaseType::putSerialNo(stringBuilder &sb) const
{
  #if USE_SERIAL_NUMBERS
    printSerialNo(sb, "t", serialNumber, "-");
  #endif
}

string CVAtomicType::toMLString() const
{
  stringBuilder sb;
  sb << cvToString(cv) << " ";
  putSerialNo(sb);
  sb << atomic->toMLString();
  return sb;
}

string PointerType::toMLString() const
{
  stringBuilder sb;
  if (cv) {
    sb << cvToString(cv) << " ";
  }
  putSerialNo(sb);
  sb << "ptr(" << atType->toMLString() << ")";
  return sb;
}

string ReferenceType::toMLString() const
{
  stringBuilder sb;
  putSerialNo(sb);
  sb << "ref(" << atType->toMLString() << ")";
  return sb;
}

string FunctionType::toMLString() const
{
  stringBuilder sb;
  // FIX: FUNC TEMPLATE LOSS
//    if (templateInfo) {
//      sb << templateInfo->paramsToMLString();
//    }
  putSerialNo(sb);
  if (flags & FF_CTOR) {
    sb << "ctor-";
  }
  sb << "fun";

  sb << "(";

  sb << "retType: " << retType->toMLString();
  // arg, you can't do this due to the way Scott handles retVar
//    if (retVar) {
//      sb << ", " << retVar->toMLString();
//    }

  int ct=0;
//    bool firstTime = true;
  SFOREACH_OBJLIST(Variable, params, iter) {
    Variable const *v = iter.data();
    ct++;
//      if (firstTime) {
//        firstTime = false;
//      } else {
    sb << ", ";                 // retType is first
//      }
//      if (isMethod() && ct==1) {
//        // print "this" param
//        sb << "this: " << v->type->toMLString();
//        continue;
//      }
    sb << v->toMLString();
//      sb << "'" << v->name << "'->" << v->type->toMLString();
  }
  sb << ")";
  return sb;
}

string ArrayType::toMLString() const
{
  stringBuilder sb;
  putSerialNo(sb);
  sb << "array(";
  sb << "size:";
  if (hasSize()) {
    sb << size;
  } else {
    sb << "unspecified";
  }
  sb << ", ";
  sb << eltType->toMLString();
  sb << ")";
  return sb;
}

string PointerToMemberType::toMLString() const
{
  stringBuilder sb;
  if (cv) {
    sb << cvToString(cv) << " ";
  }
  putSerialNo(sb);
  sb << "ptm(";
  sb << inClassNAT->name;
  sb << ", " << atType->toMLString();
  sb << ")";
  return sb;
}

string TemplateParams::paramsToMLString() const
{
  stringBuilder sb;
  sb << "template <";
#if 0
  int ct=0;
  // FIX: incomplete
  SFOREACH_OBJLIST(Variable, params, iter) {
    Variable const *p = iter.data();
    if (ct++ > 0) {
      sb << ", ";
    }

    // FIX: is there any difference here?
    // NOTE: use var->toMLString()
    if (p->type->isTypeVariable()) {
//        sb << "var(name:" << p->name << ", " << v->type->toMLString() << ")";
    }
    else {
      // non-type parameter
//        sb << p->toStringAsParameter();
    }
  }
#endif
  sb << ">";
  return sb;
}


// ---------------------- TypeFactory ---------------------
CompoundType *TypeFactory::makeCompoundType
  (CompoundType::Keyword keyword, StringRef name)
{
  CompoundType *ct = new CompoundType(keyword, name);
  TRACE("makeCompoundType", "ct=" << (void*)ct << " name=" << name);
  return ct;
}


EnumType *TypeFactory::makeEnumType(StringRef name)
{
  return new EnumType(name);
}


Type *TypeFactory::shallowCloneType(Type *baseType)
{
  switch (baseType->getTag()) {
    default:
      xfailure("bad tag");
      break;

    case Type::T_ATOMIC: {
      CVAtomicType *atomic = baseType->asCVAtomicType();

      // make a new CVAtomicType with the same AtomicType as 'baseType',
      // but with the new flags
      return makeCVAtomicType(atomic->atomic, atomic->cv);
    }

    case Type::T_POINTER: {
      PointerType *ptr = baseType->asPointerType();
      return makePointerType(ptr->cv, ptr->atType);
    }

    case Type::T_REFERENCE: {
      ReferenceType *ref = baseType->asReferenceType();
      return makeReferenceType(ref->atType);
    }

    case Type::T_FUNCTION: {
      FunctionType *ft = baseType->asFunctionType();
      FunctionType *ret = makeFunctionType(ft->retType);
      ret->flags = ft->flags;
      SFOREACH_OBJLIST_NC(Variable, ft->params, paramiter) {
        ret->params.append(paramiter.data());
      }
      ret->exnSpec = ft->exnSpec;
      return ret;
    }

    case Type::T_ARRAY: {
      ArrayType *arr = baseType->asArrayType();
      return makeArrayType(arr->eltType, arr->size);
    }

    case Type::T_POINTERTOMEMBER: {
      PointerToMemberType *ptm = baseType->asPointerToMemberType();
      return makePointerToMemberType(ptm->inClassNAT, ptm->cv, ptm->atType);
    }
  }
}


// the idea is we're trying to apply 'cv' to 'baseType'; for
// example, we could have gotten baseType like
//   typedef unsigned char byte;     // baseType == unsigned char
// and want to apply const:
//   byte const b;                   // cv = CV_CONST
// yielding final type
//   unsigned char const             // return value from this fn
Type *TypeFactory::setQualifiers(SourceLoc loc, CVFlags cv, Type *baseType,
                                 TypeSpecifier *)
{
  if (baseType->isError()) {
    return baseType;
  }

  if (cv == baseType->getCVFlags()) {
    // keep what we've got
    return baseType;
  }

  Type *ret = NULL;

  if (TypedefType *tt = baseType->ifTypedefType()) {
    return makeTypedefType(tt->m_typedefVar, cv);
  }
  else if (baseType->isCVAtomicType()) {
    ret = shallowCloneType(baseType);
    ret->asCVAtomicType()->cv = cv;
  }
  else if (baseType->isPointerType()) {
    ret = shallowCloneType(baseType);
    ret->asPointerType()->cv = cv;
  }
  else if (baseType->isPointerToMemberType()) {
    ret = shallowCloneType(baseType);
    ret->asPointerToMemberType()->cv = cv;
  }
  else {
    // anything else cannot have a cv applied to it anyway; the NULL
    // will result in an error message in the client
    //
    // 2021-06-27: At the moment, getting here means we crash.  I might
    // have broken some error reporting.
    ret = NULL;
  }

  return ret;
}


Type *TypeFactory::applyCVToType(SourceLoc loc, CVFlags cv, Type *baseType,
                                 TypeSpecifier *syntax)
{
  if (baseType->isReferenceType()) {
    // 8.3.2p1: this is fine; 'cv' is ignored
    return baseType;
  }

  if (baseType->isFunctionType()) {
    // This is also ignored per C++14 8.3.5/7.
    return baseType;
  }

  CVFlags now = baseType->getCVFlags();
  if (wantsQualifiedTypeReuseOptimization() &&
      (now | cv) == now) {
    // no change, 'cv' already contained in the existing flags
    return baseType;
  }
  else if (baseType->isArrayType()) {
    // 8.3.4 para 1: apply cv to the element type
    //
    // Note: This clones the ArrayType object every time, if someone
    // is adding const or volatile to an array (possible via typedef).
    // This is probably a bug.
    ArrayType *at = baseType->asArrayType();
    return makeArrayType(applyCVToType(loc, cv, at->eltType, NULL /*syntax*/),
                         at->size);
  }
  else {
    // change to the union; setQualifiers will take care of catching
    // inappropriate application (e.g. 'const' to a reference)
    return setQualifiers(loc, now | cv, baseType, syntax);
  }
}


bool TypeFactory::wantsQualifiedTypeReuseOptimization()
{
  return true;
}


Type *TypeFactory::syntaxPointerType(SourceLoc loc,
  CVFlags cv, Type *type, D_pointer *)
{
  return makePointerType(cv, type);
}

Type *TypeFactory::syntaxReferenceType(SourceLoc loc,
  Type *type, D_reference *)
{
  return makeReferenceType(type);
}


FunctionType *TypeFactory::syntaxFunctionType(SourceLoc loc,
  Type *retType, D_func *syntax)
{
  return makeFunctionType(retType);
}


PointerToMemberType *TypeFactory::syntaxPointerToMemberType(SourceLoc loc,
  NamedAtomicType *inClassNAT, CVFlags cv, Type *atType, D_ptrToMember *syntax)
{
  return makePointerToMemberType(inClassNAT, cv, atType);
}


Type *TypeFactory::makeTypeOf_receiver(SourceLoc loc,
  NamedAtomicType *classType, CVFlags cv, D_func *syntax)
{
  if (classType->typedefVar) {
    // Wrap the atomic in a TypedefType so when this gets printed it
    // will use the TS_name rather than TS_elaborated form.
    TypedefType *tt = makeTypedefType(classType->typedefVar, cv);

    return makeReferenceType(tt);
  }
  else {
    // One way this happens is while processing templates, where
    // 'classType' could be a PseudoInstantiation.  Since there is no
    // 'typedefVar', we cannot create a TypedefType.
    CVAtomicType *at = makeCVAtomicType(classType, cv);
    return makeReferenceType(at);
  }
}


FunctionType *TypeFactory::makeSimilarFunctionType(SourceLoc loc,
  Type *retType, FunctionType *similar)
{
  FunctionType *ret =
    makeFunctionType(retType);
  ret->flags = similar->flags & ~FF_METHOD;     // isMethod is like a parameter
  if (similar->exnSpec) {
    ret->exnSpec = new FunctionType::ExnSpec(*similar->exnSpec);
  }
  return ret;
}


CVAtomicType *TypeFactory::getSimpleType(SimpleTypeId st, CVFlags cv)
{
  return makeCVAtomicType(SimpleType::getST(st), cv);
}


ArrayType *TypeFactory::setArraySize(SourceLoc loc, ArrayType *type, int size)
{
  return makeArrayType(type->eltType, size);
}


// -------------------- BasicTypeFactory ----------------------
// this is for when I split Type from Type_Q
CVAtomicType BasicTypeFactory::unqualifiedSimple[NUM_SIMPLE_TYPES] = {
  #define CVAT(id) \
    CVAtomicType(&SimpleType::fixed[id], CV_NONE),
  CVAT(ST_CHAR)
  CVAT(ST_UNSIGNED_CHAR)
  CVAT(ST_SIGNED_CHAR)
  CVAT(ST_BOOL)
  CVAT(ST_INT)
  CVAT(ST_UNSIGNED_INT)
  CVAT(ST_LONG_INT)
  CVAT(ST_UNSIGNED_LONG_INT)
  CVAT(ST_LONG_LONG)
  CVAT(ST_UNSIGNED_LONG_LONG)
  CVAT(ST_SHORT_INT)
  CVAT(ST_UNSIGNED_SHORT_INT)
  CVAT(ST_WCHAR_T)
  CVAT(ST_FLOAT)
  CVAT(ST_DOUBLE)
  CVAT(ST_LONG_DOUBLE)
  CVAT(ST_FLOAT_COMPLEX)
  CVAT(ST_DOUBLE_COMPLEX)
  CVAT(ST_LONG_DOUBLE_COMPLEX)
  CVAT(ST_FLOAT_IMAGINARY)
  CVAT(ST_DOUBLE_IMAGINARY)
  CVAT(ST_LONG_DOUBLE_IMAGINARY)
  CVAT(ST_VOID)

  CVAT(ST_ELLIPSIS)
  CVAT(ST_CDTOR)
  CVAT(ST_ERROR)
  CVAT(ST_DEPENDENT)
  CVAT(ST_IMPLINT)
  CVAT(ST_NOTFOUND)

  CVAT(ST_PROMOTED_INTEGRAL)
  CVAT(ST_PROMOTED_ARITHMETIC)
  CVAT(ST_INTEGRAL)
  CVAT(ST_ARITHMETIC)
  CVAT(ST_ARITHMETIC_NON_BOOL)
  CVAT(ST_ANY_OBJ_TYPE)
  CVAT(ST_ANY_NON_VOID)
  CVAT(ST_ANY_TYPE)

  CVAT(ST_PRET_STRIP_REF)
  CVAT(ST_PRET_PTM)
  CVAT(ST_PRET_ARITH_CONV)
  CVAT(ST_PRET_FIRST)
  CVAT(ST_PRET_FIRST_PTR2REF)
  CVAT(ST_PRET_SECOND)
  CVAT(ST_PRET_SECOND_PTR2REF)
  #undef CVAT
};


CVAtomicType *BasicTypeFactory::makeCVAtomicType(AtomicType *atomic, CVFlags cv)
{
  // dsw: we now need to avoid this altogether since in
  // setQualifiers() I mutate the cv value on a type after doing the
  // shallowClone(); NOTE: the #ifndef OINK is no longer needed as
  // Oink no longer delegates to this method.
//  #ifndef OINK
//    // FIX: We need to avoid doing this in Oink
//    if (cv==CV_NONE && atomic->isSimpleType()) {
//      // since these are very common, and ordinary Types are immutable,
//      // share them
//      SimpleType *st = atomic->asSimpleType();
//      xassert((unsigned)(st->type) < (unsigned)NUM_SIMPLE_TYPES);
//      return &(unqualifiedSimple[st->type]);
//    }
//  #endif

  return new CVAtomicType(atomic, cv);
}


PointerType *BasicTypeFactory::makePointerType(CVFlags cv, Type *atType)
{
  return new PointerType(cv, atType);
}


Type *BasicTypeFactory::makeReferenceType(Type *atType)
{
  return new ReferenceType(atType);
}


FunctionType *BasicTypeFactory::makeFunctionType(Type *retType)
{
  return new FunctionType(retType);
}

void BasicTypeFactory::doneParams(FunctionType *)
{}


ArrayType *BasicTypeFactory::makeArrayType(Type *eltType, int size)
{
  return new ArrayType(eltType, size);
}


PointerToMemberType *BasicTypeFactory::makePointerToMemberType
  (NamedAtomicType *inClassNAT, CVFlags cv, Type *atType)
{
  return new PointerToMemberType(inClassNAT, cv, atType);
}


TypedefType *BasicTypeFactory::makeTypedefType
  (Variable *typedefVar, CVFlags cv)
{
  xassert(typedefVar);
  xassert(typedefVar->isType());

  Type *orig = typedefVar->type;
  xassert(orig);

  Type *withCV = applyCVToType(SL_UNKNOWN, cv, orig, NULL /*syntax*/);

  return new TypedefType(typedefVar, cv, withCV);
}


Variable *BasicTypeFactory::makeVariable(
  SourceLoc L, StringRef n, Type *t, DeclFlags f)
{
  // I will turn this on from time to time as a way to check that
  // Types are always capable of printing themselves.  It should never
  // be checked in when in the "on" state.
  #if 0
    if (t) {
      t->toString();
    }
  #endif // 0

  // the TranslationUnit parameter is ignored by default; it is passed
  // only for the possible benefit of an extension analysis
  Variable *var = new Variable(L, n, t, f);
  return var;
}


// -------------------- XReprSize -------------------
XReprSize::XReprSize(bool d)
  : XMessage(stringc << "reprSize of a " << (d ? "dynamically-sized" : "sizeless")
                     << " array"),
    isDynamic(d)
{}

XReprSize::XReprSize(XReprSize const &obj)
  : XMessage(obj),
    isDynamic(obj.isDynamic)
{}

XReprSize::~XReprSize()
{}


void throw_XReprSize(bool d)
{
  XReprSize x(d);
  THROW(x);
}


// --------------- debugging ---------------
char *type_toString(Type const *t)
{
  // defined in smbase/strutil.cc
  return copyToStaticBuffer(t->toString().c_str());
}


// EOF
