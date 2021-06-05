// interp.h
// Interpreter environment and runtime.

#ifndef INTERP_H
#define INTERP_H

#include "cc.ast.gen.h"                // TranslationUnit, etc.


// Interpreter enviroment.  This holds the AST of the program to execute
// and the run-time state of that program as it executes, including all
// of its memory and call stack frames.
class IEnv {
private:     // data
  // String table, shared with the type checking phase so all of the
  // names in the AST come from this table.
  StringTable &m_stringTable;

  // For the moment, we can only interpret within a single TU.
  TranslationUnit const *m_translationUnit;

private:     // methods
  // Search for the 'main' function in 'm_translationUnit'.  Return NULL
  // if none found.
  Function const *findMain();

public:      // methods
  IEnv(StringTable &stringTable,
       TranslationUnit const *translationUnit);
  ~IEnv();

  // Run the 'main' function and return its exit code.
  int interpMain();
};


#endif // INTERP_H
