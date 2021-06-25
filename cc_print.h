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
#include "sm-iostream.h"               // ostream
#include "str.h"                       // stringBuilder


// Forward in this file.
class PrintEnv;


// this virtual semi-abstract class is intended to act as a
// "superclass" for ostream, stringBuilder, and any other "output
// stream" classes
class OutStream {
public:      // methods
  virtual ~OutStream() {}

  // Insert the given NUL-terminated string.
  virtual void insert(char const *s) = 0;
};

class StringBuilderOutStream : public OutStream {
public:      // data
  stringBuilder &buffer;

public:      // methods
  StringBuilderOutStream(stringBuilder &buffer0) : buffer(buffer0) {}

  void insert(char const *s) override
    { buffer << s; }
};

class OStreamOutStream : public OutStream {
public:      // data
  ostream &out;

public:      // methods
  OStreamOutStream(ostream &out0) : out(out0) {}

  void insert(char const *s) override
    { out << s; }
};

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
  OutStream &m_out;
  SourceLoc loc;

public:      // methods
  PrintEnv(TypePrinter &typePrinter0, OutStream &out0)
    : typePrinter(typePrinter0)
    , m_out(out0)
    , loc(SL_UNKNOWN)
  {}

  TypeLike const *getTypeLike(Variable const *var)
    { return typePrinter.getTypeLike(var); }

  // Render the built BoxPrint tree to a string and write it to 'm_out'.
  void finish();

  // Print 'type' using 'typePrinter'.
  void ptype(TypeLike const *type, char const *name = "");
};

// Version of PrintEnv that prints to a string.
class StringPrintEnv : public PrintEnv {
private:      // data
  stringBuilder m_sb;
  StringBuilderOutStream m_sbos;
  CTypePrinter m_typePrinter;

public:      // code
  StringPrintEnv(CCLang const &lang)
    : PrintEnv(m_typePrinter, m_sbos),
      m_sbos(m_sb),
      m_typePrinter(lang)
  {}

  // Get the string.  This calls 'finish()'.
  string getResult();
};

// Print 'type' to a string using the rules of 'lang'.
string printTypeToString(CCLang const &lang, Type const *type);

void printSTemplateArgument(PrintEnv &env, STemplateArgument const *sta);


// Print 'astNode' as a string.
template <class T>
string printASTNodeToString(CCLang const &lang, T *astNode)
{
  CTypePrinter typePrinter(lang);
  stringBuilder sb;
  StringBuilderOutStream sbos(sb);
  PrintEnv env(typePrinter, sbos);

  astNode->print(env);
  env.finish();

  return sb.str();
}


#endif // ELSA_CC_PRINT_H
