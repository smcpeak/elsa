// interp.h
// Interpreter environment and runtime.

#ifndef INTERP_H
#define INTERP_H

// elsa
#include "cc.ast.gen.h"                // Function, Expression, etc.

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


// Interpreter environment.  This holds the run-time state of that
// program as it executes, including all of its memory and call stack
// frames.
//
// TODO: Rename to Interp.
class IEnv {
  NO_OBJECT_COPIES(IEnv);

private:     // data
  // String table, shared with the type checking phase so all of the
  // names in the AST come from this table.
  StringTable &m_stringTable;

  // Stack of frames.  The top frame is the currently active one.
  ObjStack<IFrame> m_callStack;

protected:   // methods
  // Interpret this function's body.
  //
  // The caller is responsible for creating the activation record and
  // populating it with argument values.  Then after 'interp' returns,
  // the caller is responsible for extracting the return value from
  // the activation record and deleting it.
  void interpFunction(Function const *func);

  // Interpret this statement and return the successor statemet, or NULL
  // if there is none (due to 'return' or executing the last statement
  // in a function).
  Statement const *interpStatement(Statement const *stmt);

  // Delegate to the underlying Expression.
  int interpFullExpression(FullExpression const *fullExpr);
  int interpArgExpression(ArgExpression const *argExpr);

  // Evaluate this expression and return the resulting value.
  int interpExpression(Expression const *expr);

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

  // Get a 'StringRef' for 'name' from 'm_stringTable'.
  StringRef getStringRef(char const *name);
};


#endif // INTERP_H
