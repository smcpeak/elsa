// ubermods-attrspec.cc
// Code for ubermods-attrspec.h.

#include "ubermods-attrspec.h"         // this module

#include "ccparse.h"                   // ParseEnv


UberModifiersAndASL *umaaslCombineUM(ParseEnv &env, SourceLoc loc,
  UberModifiersAndASL * /*nullable*/ uma, UberModifiers mods)
{
  if (!uma) {
    if (mods == UM_NONE) {
      // A NULL pointer implicitly represents UM_NONE.
      return NULL_UMA;
    }
    else {
      return new UberModifiersAndASL(mods, NULL_ASL /*list*/);
    }
  }
  else {
    UberModifiers newMods = env.uberCombine(loc, uma->m_modifiers, mods);

    if (newMods == uma->m_modifiers) {
      // The existing object already has the desired flags.
      return uma;
    }
    else {
      return new UberModifiersAndASL(newMods, uma->m_attrSpecList);
    }
  }
}


UberModifiersAndASL *umaaslAppendAS(
  UberModifiersAndASL * /*nullable*/ uma, AttributeSpecifier *as)
{
  if (!uma) {
    return new UberModifiersAndASL(
      UM_NONE,
      aslSingleton(as));
  }
  else {
    return new UberModifiersAndASL(
      uma->m_modifiers,
      aslAppendAS(uma->m_attrSpecList, as));
  }
}


UberModifiersAndASL *umaaslPrependAS(
  AttributeSpecifier *as, UberModifiersAndASL * /*nullable*/ uma)
{
  if (!uma) {
    return new UberModifiersAndASL(
      UM_NONE,
      aslSingleton(as));
  }
  else {
    return new UberModifiersAndASL(
      uma->m_modifiers,
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
