// interp.h
// Interpreter environment and runtime.

#ifndef INTERP_H
#define INTERP_H

// elsa
#include "cc.ast.gen.h"                // TranslationUnit, etc.

// smbase
#include "objstack.h"                  // ObjStack


// Interpreter call stack frame.
class IFrame {
  NO_OBJECT_COPIES(IFrame);

public:      // data
  int m_returnValue;

public:      // methods
  IFrame();
  ~IFrame();
};


// Interpreter enviroment.  This holds the AST of the program to execute
// and the run-time state of that program as it executes, including all
// of its memory and call stack frames.
class IEnv {
  NO_OBJECT_COPIES(IEnv);

private:     // data
  // String table, shared with the type checking phase so all of the
  // names in the AST come from this table.
  StringTable &m_stringTable;

  // Stack of frames.  The top frame is the currently active one.
  ObjStack<IFrame> m_callStack;

public:      // methods
  IEnv(StringTable &stringTable);
  ~IEnv();

  // Run the 'main' function and return its exit code.
  int interpMain(Function const *mainFunction);

  // Create a new frame and push it onto the call stack.
  IFrame *pushNewFrame();

  // Pop the stack, which must have 'top' on top, and delete 'top'.
  void popFrame(IFrame *top);

  // Get currently active frame.
  IFrame *topFrame();
};


#endif // INTERP_H
