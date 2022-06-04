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
#include "cc-lang.h"                   // CCLang
#include "type-printer.h"              // TypePrinter, CTypePrinter

// smbase
#include "sm-macros.h"                 // NULLABLE
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

  // Other language options that affect printing.  At the moment, the
  // only one I plan to use is CCLang::isCplusplus.
  CCLang const &m_lang;

  // If true, print comments with some additional AST details.  The
  // default is true.
  bool m_printComments;

  // If true, print implicit standard conversions as invocations of a
  // hypothetical "ISC" macro.  The default is false.
  bool m_printISC;

  // If true, print the "digested" semantic initializers (when
  // available) rather than the original syntax.  Default is true.
  bool m_printSemanticInitializers;

public:      // methods
  PrintEnv(TypePrinter &typePrinter, CCLang const &lang)
    : m_typePrinter(typePrinter),
      m_lang(lang),
      m_printComments(true),
      m_printISC(false),
      m_printSemanticInitializers(true)
  {}

  // Print the tree to a string.
  string getResult();

  // If 'm_printComments', then:
  //   * strip any existing comments out of 'info',
  //   * enclose the result in "/*...*/",
  //   * append 'after',
  //   * and return that.
  // Otherwise return the empty string.
  string commentary(string const &info, char const *after="") const;

  // If 'name' is not NULL, return it.  Otherwise, if 'm_printComments',
  // return "/*anon*/".  Otherwise, return "".
  char const *possiblyAnonName(StringRef name) const;

  // Return the spelling of the 'asm' keyword for 'm_lang'.
  char const *asmKeywordSpelling() const;

  // Print 'type' using 'm_typePrinter'.
  virtual void ptype(TypeLike const *type, char const *name = "");

  // Nominally, call 'stmt->iprint(*this, context)'.  This is exposed as
  // a possible point of customization for clients.
  virtual void iprintStatement(
    Statement const *stmt, StatementContext context);

  // Nominally, call 'expr->iprint(*this)'.
  virtual void iprintExpression(Expression const *expr);

  // If 'm_printSemanticInitializers' and 'semanticInit!=NULL', return
  // that.  Otherwise return 'syntacticInit'.
  Initializer const * NULLABLE selectInitializer(
    Initializer const * NULLABLE syntacticInit,
    SemanticInitializer const * NULLABLE semanticInit);
};

// Print 'type' to a string using the rules of 'lang'.
string printTypeToString(CCLang const &lang, Type const *type);

void printSTemplateArgument(PrintEnv &env, STemplateArgument const *sta);


// Usual case printer.
template <class T>
void printASTNodeToEnv(PrintEnv &env, T const *astNode)
{
  astNode->print(env);
}

// Overloads for specific nodes that require additional arguments, but
// we can supply reasonable values for printing "in general".
inline void printASTNodeToEnv(PrintEnv &env, Initializer const *init)
{
  init->print(env, true /*outermost*/);
}


// Print 'astNode' as a string.
template <class T>
string printASTNodeToString(CCLang const &lang, T const *astNode)
{
  CTypePrinter typePrinter(lang);
  PrintEnv env(typePrinter, lang);

  printASTNodeToEnv(env, astNode);

  return env.getResult();
}


// The template above does not work for Statements because they require
// an additional parameter.
string printStatementToString(
  CCLang const &lang, Statement const *stmt, StatementContext context,
  bool printComments = true);


// Expressions do too.
string printExpressionToString(
  CCLang const &lang, Expression const *expr, OperatorPrecedence prec,
  bool printComments = true);


#endif // ELSA_CC_PRINT_H
