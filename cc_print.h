// cc_print.h            see license.txt for copyright and terms of use

// This is a tree walk that prints out a functionally equivalent C++
// program to the original.  The AST entry points are declared in
// cc.ast

// Adapted from cc_tcheck.cc by Daniel Wilkerson dsw@cs.berkeley.edu

#ifndef CC_PRINT_H
#define CC_PRINT_H

#include "cc_ast.h"             // C++ AST; this module
#include "str.h"                // stringBuilder

#include "boxprint.h"           // BoxPrint
#include "sm-iostream.h"        // ostream


// Forward in this file.
class PrintEnv;


// this virtual semi-abstract class is intended to act as a
// "superclass" for ostream, stringBuilder, and any other "output
// stream" classes
class OutStream {
  public:
  virtual ~OutStream() {}

  // special-case methods
  virtual OutStream & operator << (ostream& (*manipfunc)(ostream& outs)) = 0;
  virtual void flush() = 0;

  // special method to support rostring
  virtual OutStream & operator << (rostring message) = 0;

  // generic methods
  #define MAKE_INSERTER(type) \
    virtual OutStream &operator << (type message) = 0;
  MAKE_INSERTER(char const *)
  MAKE_INSERTER(char)
  MAKE_INSERTER(bool)
  MAKE_INSERTER(int)
  MAKE_INSERTER(unsigned int)
  MAKE_INSERTER(long)
  MAKE_INSERTER(unsigned long)
  MAKE_INSERTER(double)
  #undef MAKE_INSERTER
};

class StringBuilderOutStream : public OutStream {
public:      // data
  stringBuilder &buffer;

public:      // methods
  StringBuilderOutStream(stringBuilder &buffer0) : buffer(buffer0) {}

  // special-case methods
  virtual StringBuilderOutStream & operator << (ostream& (*manipfunc)(ostream& outs)) {
    buffer << "\n";             // assume that it is endl
    return *this;
  }
  virtual void flush() {}       // no op

  // special method to support rostring
  virtual OutStream & operator << (rostring message) {return operator<< (message.c_str());}

  // generic methods
  #define MAKE_INSERTER(type)        \
    virtual StringBuilderOutStream &operator << (type message) \
    {                                \
      buffer << message;             \
      return *this;                  \
    }
  MAKE_INSERTER(char const *)
  MAKE_INSERTER(char)
  MAKE_INSERTER(bool)
  MAKE_INSERTER(int)
  MAKE_INSERTER(unsigned int)
  MAKE_INSERTER(long)
  MAKE_INSERTER(unsigned long)
  MAKE_INSERTER(double)
  #undef MAKE_INSERTER
};

class OStreamOutStream : public OutStream {
  ostream &out;

  public:
  OStreamOutStream(ostream &out0) : out(out0) {}

  // special-case methods
  virtual OStreamOutStream & operator << (ostream& (*manipfunc)(ostream& outs)) {
    out << manipfunc;
    return *this;
  }
  virtual void flush() { out.flush(); }

  // special method to support rostring
  virtual OutStream & operator << (rostring message) {return operator<< (message.c_str());}

  // generic methods
  #define MAKE_INSERTER(type)        \
    virtual OStreamOutStream &operator << (type message) \
    {                                \
      out << message;                \
      return *this;                  \
    }
  MAKE_INSERTER(char const *)
  MAKE_INSERTER(char)
  MAKE_INSERTER(bool)
  MAKE_INSERTER(int)
  MAKE_INSERTER(unsigned int)
  MAKE_INSERTER(long)
  MAKE_INSERTER(unsigned long)
  MAKE_INSERTER(double)
  #undef MAKE_INSERTER
};

// In Oink, TypeLike is a superclass of Type but here we will just
// make it synonymous with Type.  oink/cc_print.h.cpatch comments-out
// this declaration.
typedef Type TypeLike;

// Interface for classes that know how to print out types
class TypePrinter {
public:
  virtual ~TypePrinter() {}

  // Print 'type'.  'name' is printed among the type syntax in a way
  // that would declare a variable of that name.  If it is NULL, then
  // "/*anon*/" is printed.
  virtual void printType(PrintEnv &env, TypeLike const *type, char const *name) = 0;

  // retrieve the TypeLike to print for a Variable; in Elsa, this
  // just gets Variable::type, but Oink does something else
  virtual TypeLike const *getTypeLike(Variable const *var);

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

  // satisfy the interface to TypePrinter
  void printType(PrintEnv &env, TypeLike const *type, char const *name) override;

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


#endif // CC_PRINT_H
