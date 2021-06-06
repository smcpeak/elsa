// elsaparse.h
// Main entry point for Elsa as a library.

#ifndef ELSAPARSE_H
#define ELSAPARSE_H

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

  // Language options.
  CCLang &lang;

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
};


#endif // ELSAPARSE_H
