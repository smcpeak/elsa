// cc-print.h            see license.txt for copyright and terms of use

// This is a tree walk that prints out a functionally equivalent C++
// program to the original.  The AST entry points are declared in
// cc-print.ast.

// Originally adapted from cc-tcheck.cc by Daniel Wilkerson
// dsw@cs.berkeley.edu, but substantially modified afterward by Scott
// McPeak.

#ifndef ELSA_CC_PRINT_H
#define ELSA_CC_PRINT_H

// elsa
#include "cc-ast.h"                    // C++ AST
#include "type-printer.h"              // TypePrinter, CTypePrinter

// smbase
#include "tree-print.h"                // TreePrint


// Context for a pretty-print.
//
// When printing, we accumulate in TreePrint a tree that describes the
// intended syntax structure.  Then, 'print()' renders it to an
// ostream.
//
// Printing is somewhat delicate because it's easy to have too many or
// too few line breaks.  The general strategy used in cc-print is that
// breaks are always and only inserted by code that prints *lists*.
// That is, we put breaks exactly in those places where we are aware of
// what is being printed on both sides of the break.  Individual
// elements do *not* insert breaks on their own.
//
class PrintEnv : public TreePrint {
public:      // data
  // How to print types.
  TypePrinter &m_typePrinter;

public:      // methods
  PrintEnv(TypePrinter &typePrinter)
    : m_typePrinter(typePrinter)
  {}

  // Print the tree to a string.
  string getResult();

  // Print 'type' using 'm_typePrinter'.
  virtual void ptype(TypeLike const *type, char const *name = "");

  // Nominally, call 'stmt->iprint(*this)'.  This is exposed as a
  // possible point of customization for clients.
  virtual void iprintStatement(Statement const *stmt);

  // Nominally, call 'expr->iprint(*this)'.
  virtual void iprintExpression(Expression const *expr);
};

// Print 'type' to a string using the rules of 'lang'.
string printTypeToString(CCLang const &lang, Type const *type);

void printSTemplateArgument(PrintEnv &env, STemplateArgument const *sta);


// Print 'astNode' as a string.
template <class T>
string printASTNodeToString(CCLang const &lang, T const *astNode)
{
  CTypePrinter typePrinter(lang);
  PrintEnv env(typePrinter);

  astNode->print(env);

  return env.getResult();
}


#endif // ELSA_CC_PRINT_H
