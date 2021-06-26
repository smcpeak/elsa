// type-printer.h
// TypePrinter class.

#ifndef ELSA_TYPE_PRINTER_H
#define ELSA_TYPE_PRINTER_H

#include "cc_ast.h"                    // C++ AST
#include "cc_lang.h"                   // CCLang
#include "cc-type.h"                   // Type, etc.


// In Oink, TypeLike is a superclass of Type, but by default it is
// synonymous with Type.
#ifndef REMOVE_DEFAULT_TYPELIKE_DEFINITION
typedef Type TypeLike;
#endif


// Interface for classes that know how to print out types
class TypePrinter {
public:
  virtual ~TypePrinter() {}

  // Print 'type'.  'name' is printed among the type syntax in a way
  // that would declare a variable of that name.  If it is NULL, then
  // "/*anon*/" is printed.
  virtual string printType(TypeLike const *type, char const *name) = 0;

  // retrieve the TypeLike to print for a Variable; in Elsa, this
  // just gets Variable::type, but Oink does something else
  virtual TypeLike const *getVariableTypeLike(Variable const *var);

  // retrieve for a Function, nominally Function::funcType
  virtual TypeLike const *getFunctionTypeLike(Function const *func);

  // and for an E_constructor, nominally Expression::type
  virtual TypeLike const *getE_constructorTypeLike(E_constructor const *c);
};

// This class knows how to print out Types in C syntax
class CTypePrinter : public TypePrinter {
public:      // instance data
  CCLang const &m_lang;

public:      // methods
  CTypePrinter(CCLang const &lang)
    : m_lang(lang)
  {}

  virtual ~CTypePrinter() {}

  // Implement TypePrinter interface.
  string printType(TypeLike const *type, char const *name) override;

protected:   // methods
  // **** AtomicType
  string print(AtomicType const *atomic);

  string print(SimpleType const *);
  string print(CompoundType const *);
  string print(EnumType const *);
  string print(TypeVariable const *);
  string print(PseudoInstantiation const *);
  string print(DependentQType const *);

  // **** [Compound]Type
  string print(Type const *type);
  string print(Type const *type, char const *name);
  string printRight(Type const *type, bool innerParen = true);
  string printLeft(Type const *type, bool innerParen = true);

  string printLeft(CVAtomicType const *type, bool innerParen = true);
  string printRight(CVAtomicType const *type, bool innerParen = true);
  string printLeft(PointerType const *type, bool innerParen = true);
  string printRight(PointerType const *type, bool innerParen = true);
  string printLeft(ReferenceType const *type, bool innerParen = true);
  string printRight(ReferenceType const *type, bool innerParen = true);
  string printLeft(FunctionType const *type, bool innerParen = true);
  string printRight(FunctionType const *type, bool innerParen = true);
  string printRightUpToQualifiers(FunctionType const *type, bool innerParen);
  string printRightQualifiers(FunctionType const *type, CVFlags cv);
  string printRightAfterQualifiers(FunctionType const *type);
  void   printExtraRightmostSyntax(FunctionType const *type, stringBuilder &);
  string printLeft(ArrayType const *type, bool innerParen = true);
  string printRight(ArrayType const *type, bool innerParen = true);
  string printLeft(PointerToMemberType const *type, bool innerParen = true);
  string printRight(PointerToMemberType const *type, bool innerParen = true);

  // **** Variable
  string printAsParameter(Variable const *var);
};


#endif // ELSA_TYPE_PRINTER_H
