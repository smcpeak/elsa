// ubermods-attrspec.h
// UberModifiersAndASL class, 'umaaslXXX' functions to manipulate it,
// as well as 'aslXXX' functions to manipulate AttributeSpecifierLists.

#ifndef ELSA_UBERMODS_ATTRSPEC_H
#define ELSA_UBERMODS_ATTRSPEC_H

// smbase
#include "srcloc.h"                    // SourceLoc

// elsa
#include "cc-flags.h"                  // UberModifiers
#include "ccparse-fwd.h"               // ParseEnv

class AttributeSpecifier;              // gnu.ast.gen.h
class AttributeSpecifierList;          // gnu.ast.gen.h


// A pair of an UberModifiers set and an AttributeSpecifierList.
//
// The main reason this class exists is I have places in gnu.gr where I
// need to carry both of these (because modifiers and attributes can be
// freely intermixed), but Elkhound parser semantic values are limited
// to what can be stored in a single 'uintptr_t'.  Consequently, I use a
// heap-allocated pair.
//
// Like AttributeSpecifierList, this class is considered part of the
// AST familiy of classes (although it only exists during parsing
// proper, having been consumed and discarded by the time we get to the
// type-check phase), and its objects are therefore never freed.
//
// TODO: Perhaps I could use reference counting for these?
//
class UberModifiersAndASL {
public:      // data
  // The set of UberModifiers.
  UberModifiers m_modifiers;

  // A nullable list of '__attribute__' specifiers.
  //
  // This would be an owner pointer, except that AttributeSpecifierList
  // is an AST node, and there are generally issues with freeing them
  // due to pointer duplication as part of GLR parsing, so for now this
  // is never freed.
  AttributeSpecifierList *m_attrSpecList;

public:      // methods
  UberModifiersAndASL(UberModifiers modifiers, AttributeSpecifierList *asl)
    : m_modifiers(modifiers),
      m_attrSpecList(asl)
  {}

  // Compiler-supplied copy constructor and assignment are fine.
};


// The 'umaaslXXX' functions accept and return nullable pointers to
// 'UberModifiersAndASL', where NULL_UMA represents the pair of UM_NONE
// and an empty attribute specifier list.
//
// These operate functionally, in the sense that they do not modify
// the passed object, instead returning a new one if the new value is
// different from the old.

// An empty UberModifiersAndASL.
#define NULL_UMA ((UberModifiersAndASL*)NULL)

// Return the union of 'uma' and 'mods'.  The result could be NULL,
// and/or equal to 'uma', if either properly represents the union.
UberModifiersAndASL *umaaslCombineUM(ParseEnv &env, SourceLoc loc,
  UberModifiersAndASL * /*nullable*/ uma, UberModifiers mods);

// Return the result of appending 'as' to 'uma'.
UberModifiersAndASL *umaaslAppendAS(
  UberModifiersAndASL * /*nullable*/ uma, AttributeSpecifier *as);

// Return the result of prepending 'as' to 'uma'.
UberModifiersAndASL *umaaslPrependAS(
  AttributeSpecifier *as, UberModifiersAndASL * /*nullable*/ uma);

// Extract the modifiers from 'uma'.
UberModifiers umaaslUM(UberModifiersAndASL * /*nullable*/ uma);

// Extract the attribute specifier list from 'uma'.  This can return
// NULL, meaning the list is empty.
AttributeSpecifierList *umaaslASL(UberModifiersAndASL * /*nullable*/ uma);


// -------------- AttributeSpecifierList construction ------------------
// These functions treat 'AttributeSpecifierList*' as a functional list.

// An empty AttributeSpecifierList.
#define NULL_ASL ((AttributeSpecifierList*)NULL)

// Return a list with just one element, 'as'.
AttributeSpecifierList *aslSingleton(AttributeSpecifier *as);

// Return 'list' followed by 'as'.
AttributeSpecifierList *aslAppendAS(
  AttributeSpecifierList * /*nullable*/ list,
  AttributeSpecifier *as);

// Return 'list1 followed by 'list2'.
AttributeSpecifierList *aslAppendASL(
  AttributeSpecifierList * /*nullable*/ list1,
  AttributeSpecifierList * /*nullable*/ list2);

// Return list of 'as' then 'list'.
AttributeSpecifierList *aslPrependAS(
  AttributeSpecifier *as,
  AttributeSpecifierList * /*nullable*/ list);


#endif // ELSA_UBERMODS_ATTRSPEC_H
