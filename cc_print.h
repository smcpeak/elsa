// cc_print.h            see license.txt for copyright and terms of use

// This is a tree walk that prints out a functionally equivalent C++
// program to the original.  The AST entry points are declared in
// cc_print.ast.

// Originally adapted from cc_tcheck.cc by Daniel Wilkerson
// dsw@cs.berkeley.edu, but substantially modified afterward by Scott
// McPeak.

#ifndef ELSA_CC_PRINT_H
#define ELSA_CC_PRINT_H

// elsa
#include "cc_ast.h"                    // C++ AST
#include "type-printer.h"              // TypePrinter, CTypePrinter

// smbase
#include "boxprint.h"                  // BoxPrint


// Context for a pretty-print.
//
// When printing, we accumulate in BoxPrint a tree that describes the
// intended syntax structure.  Then, 'finish()' renders it to a string,
// which is sent to 'm_out'.
//
// Printing is somewhat delicate because it's easy to have too many or
// too few line breaks.  The general strategy used in cc_print is that
// breaks are always and only inserted by code that prints *lists*.
// That is, we put breaks exactly in those places where we are aware of
// what is being printed on both sides of the break.  Individual
// elements do *not* insert breaks on their own.
//
class PrintEnv : public BoxPrint {
public:      // data
  TypePrinter &typePrinter;
  SourceLoc loc;

public:      // methods
  PrintEnv(TypePrinter &typePrinter0)
    : typePrinter(typePrinter0),
      loc(SL_UNKNOWN)
  {}

  TypeLike const *getTypeLike(Variable const *var)
    { return typePrinter.getTypeLike(var); }

  // Render the built BoxPrint tree to a string.  This internally
  // empties the tree, so a subsequent call would return the empty
  // string if no further printing happens.
  string getResult();

  // Print 'type' using 'typePrinter'.
  void ptype(TypeLike const *type, char const *name = "");
};

// Print 'type' to a string using the rules of 'lang'.
string printTypeToString(CCLang const &lang, Type const *type);

void printSTemplateArgument(PrintEnv &env, STemplateArgument const *sta);


// Print 'astNode' as a string.
template <class T>
string printASTNodeToString(CCLang const &lang, T *astNode)
{
  CTypePrinter typePrinter(lang);
  PrintEnv env(typePrinter);
  astNode->print(env);
  return env.getResult();
}


#endif // ELSA_CC_PRINT_H
