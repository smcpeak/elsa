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
  StringTable &strTable;

  // Way to make types.
  BasicTypeFactory m_typeFactory;

  // Language options.
  CCLang &lang;

  // If true, print the counts of errors and warnings at the end.  This
  // is initially false.
  bool printErrorCount;

  // If true, pretty-print the parsed AST as C/C++ syntax after parsing.
  // Initially false.
  bool prettyPrint;

  // If true, print decoded string literals after parsing.  Initially
  // false.
  bool printStringLiterals;

  // The parsed TU.
  TranslationUnit *unit;

  // The 'main' function definition; NULL if none.
  Function *mainFunction;

  // Time in milliseconds for various phases.
  long parseTime;
  long tcheckTime;
  long integrityTime;
  long elaborationTime;

public:      // methods
  ElsaParse(StringTable &strTable, CCLang &lang);
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

  // Same for variables.
  Variable *getGlobalVar(char const *varName) const;
};


#endif // ELSAPARSE_H
