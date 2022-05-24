// ubermods-attrspec.cc
// Code for ubermods-attrspec.h.

#include "ubermods-attrspec.h"         // this module

#include "ccparse.h"                   // ParseEnv


// Decrement the count of 'existing' and make a new object with 'mods'
// and 'list'.  Except, if the ref count of 'existing' is 1, then just
// reuse it.
static UberModifiersAndASL * /*inc*/ umaaslMakeOrReuse(
  UberModifiersAndASL * /*dec*/ existing,
  UberModifiers mods,
  AttributeSpecifierList *list)
{
  xassert(existing);
  if (existing->getRefCount() == 1) {
    // Reuse it.
    existing->m_modifiers = mods;
    existing->m_attrSpecList = list;
    return existing;
  }
  else {
    decRefCount(existing);
    return incRefCount(new UberModifiersAndASL(mods, list));
  }
}


UberModifiersAndASL * /*inc*/ umaaslCombineUM(ParseEnv &env, SourceLoc loc,
  UberModifiersAndASL * /*nullable dec*/ uma, UberModifiers mods)
{
  if (!uma) {
    if (mods == UM_NONE) {
      // A NULL pointer implicitly represents UM_NONE.
      return NULL_UMA;
    }
    else {
      return incRefCount(new UberModifiersAndASL(mods, NULL_ASL /*list*/));
    }
  }
  else {
    UberModifiers newMods = env.uberCombine(loc, uma->m_modifiers, mods);

    if (newMods == uma->m_modifiers) {
      // The existing object already has the desired flags.
      return uma;
    }
    else {
      return umaaslMakeOrReuse(uma, newMods, uma->m_attrSpecList);
    }
  }
}


UberModifiersAndASL * /*inc*/ umaaslAppendAS(
  UberModifiersAndASL * /*nullable dec*/ uma, AttributeSpecifier *as)
{
  if (!uma) {
    return incRefCount(new UberModifiersAndASL(
      UM_NONE,
      aslSingleton(as)));
  }
  else {
    return umaaslMakeOrReuse(uma, uma->m_modifiers,
      aslAppendAS(uma->m_attrSpecList, as));
  }
}


UberModifiersAndASL * /*inc*/ umaaslPrependAS(
  AttributeSpecifier *as, UberModifiersAndASL * /*nullable dec*/ uma)
{
  if (!uma) {
    return incRefCount(new UberModifiersAndASL(
      UM_NONE,
      aslSingleton(as)));
  }
  else {
    return umaaslMakeOrReuse(uma, uma->m_modifiers,
      aslPrependAS(as, uma->m_attrSpecList));
  }
}


UberModifiers umaaslUM(UberModifiersAndASL * /*nullable*/ uma)
{
  if (!uma) {
    return UM_NONE;
  }
  else {
    return uma->m_modifiers;
  }
}


AttributeSpecifierList *umaaslASL(UberModifiersAndASL * /*nullable*/ uma)
{
  if (!uma) {
    return NULL_ASL;
  }
  else {
    return uma->m_attrSpecList;
  }
}


// --------------------- AttributeSpecifierList ------------------------
AttributeSpecifierList *aslSingleton(AttributeSpecifier *as)
{
  return new AttributeSpecifierList(as, NULL_ASL /*next*/);
}


AttributeSpecifierList *aslAppendAS(
  AttributeSpecifierList * /*nullable*/ list,
  AttributeSpecifier *as)
{
  if (!list) {
    return aslSingleton(as);
  }
  else {
    return aslPrependAS(list->spec, aslAppendAS(list->next, as));
  }
}


AttributeSpecifierList *aslAppendASL(
  AttributeSpecifierList * /*nullable*/ list1,
  AttributeSpecifierList * /*nullable*/ list2)
{
  if (!list1) {
    return list2;
  }
  else if (!list2) {
    return list1;
  }
  else {
    return aslPrependAS(list1->spec, aslAppendASL(list1->next, list2));
  }
}


AttributeSpecifierList *aslPrependAS(
  AttributeSpecifier *as,
  AttributeSpecifierList * /*nullable*/ list)
{
  return new AttributeSpecifierList(as, list);
}


// EOF
