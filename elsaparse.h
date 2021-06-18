// elsaparse.h
// Main entry point for Elsa as a library.

#ifndef ELSAPARSE_H
#define ELSAPARSE_H

#include "elsaparse-fwd.h"             // forwards for this module

// elsa
#include "cc.gr.gen.h"                 // TranslationUnit, Function
#include "cc_lang.h"                   // CCLang

// smbase
#include "strtable.h"                  // StringTable


// Class to contain the parameters to and results of parsing.
//
// TODO: This interface is not very well designed.  I made it by
// directly extracting the parts I needed for the interpreter, but it
// could use some cleanup.
class ElsaParse {
  NO_OBJECT_COPIES(ElsaParse);

public:      // data
  // String table for identifiers.
  StringTable &m_stringTable;

  // Way to make types.
  BasicTypeFactory m_typeFactory;

  // Language options.
  CCLang &m_lang;

  // If true, print the counts of errors and warnings at the end.  This
  // is initially false.
  bool m_printErrorCount;

  // If true, pretty-print the parsed AST as C/C++ syntax after parsing.
  // Initially false.
  bool m_prettyPrint;

  // If true, print decoded string literals after parsing.  Initially
  // false.
  bool m_printStringLiterals;

  // The parsed TU.
  TranslationUnit *m_translationUnit;

  // The 'main' function definition; NULL if none.
  Function *m_mainFunction;

  // Time in milliseconds for various phases.
  long m_parseTime;
  long m_tcheckTime;
  long m_integrityTime;
  long m_elaborationTime;

public:      // methods
  ElsaParse(StringTable &stringTable, CCLang &lang);
  ~ElsaParse();

  // Parse 'inputFname' according to the language preferences in 'lang'.
  //
  // In the case of errors, for now, this just prints them and exits
  // without returning.
  void parse(char const *inputFname);

  // Print the phase times.
  void printTimes();

  // Search the global scope for a type with the given name.  Throw if
  // it is not found.
  Type *getGlobalType(char const *typeName) const;

  // Same for variables, and also for typedefs.
  Variable *getGlobalVar(char const *varName) const;
};


#endif // ELSAPARSE_H
