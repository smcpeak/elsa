// interp.cc
// Code for interp.h.

#include "interp.h"                    // this module

// smbase
#include "trace.h"                     // TRACE

// libc
#include <stdio.h>                     // putchar


// ---------------------------- IFrame ---------------------------------
IFrame::IFrame()
  : m_returnValue(0)
{}


IFrame::~IFrame()
{}


// ---------------------------- Interp ---------------------------------
Interp::Interp(StringTable &stringTable)
  : m_stringTable(stringTable),
    m_callStack()
{}


Interp::~Interp()
{
  m_callStack.clear();
}


int Interp::interpMain(Function const *mainFunction)
{
  TRACE("interp", "start of interpMain");

  IFrame *frame = pushNewFrame();
  interpFunction(mainFunction);

  int ret = frame->m_returnValue;
  popFrame(frame);

  return ret;
}


IFrame *Interp::pushNewFrame()
{
  m_callStack.push(new IFrame);
  return m_callStack.top();
}


void Interp::popFrame(IFrame *top)
{
  xassert(m_callStack.top() == top);
  delete m_callStack.pop();
}


IFrame *Interp::topFrame()
{
  return m_callStack.top();
}


StringRef Interp::getStringRef(char const *name)
{
  return m_stringTable.add(name);
}


// --------------------------- Function --------------------------------
void Interp::interpFunction(Function const *function)
{
  Variable *funcVar = function->nameAndParams->var;
  TRACE("interp", "Beginning execution of '" <<
    funcVar->toQualifiedString() << "' at " <<
    toString(funcVar->loc) << ".");

  Statement const *curStatement = function->body;
  while (curStatement) {
    // Interpret the statement and get the next one.
    curStatement = interpStatement(curStatement);
  }

  TRACE("interp", "Finished execution of '" <<
    funcVar->toQualifiedString() << "'.");
}


// --------------------------- Statement -------------------------------
Statement const *Interp::interpStatement(Statement const *stmt)
{
  ASTSWITCHC(Statement, stmt) {
    ASTCASEC(S_return, r) {
      if (r->expr) {
        int val = interpFullExpression(r->expr);
        topFrame()->m_returnValue = val;
        TRACE("interp", "Return value is " << val << ".");
      }
      return NULL;
    }
    ASTNEXTC1(S_compound) {
      // Use generic handling.
    }
    ASTNEXTC(S_expr, s) {
      interpFullExpression(s->expr);
    }
    ASTDEFAULTC {
      xunimp(stringb("Statement " << stmt->kindName()));
      return NULL;
    }
    ASTENDCASEC
  }

  // Generic successor handling uses the computed CFG, but requires that
  // the successor be unambiguous.
  NextPtrList successors;
  stmt->getSuccessors(successors, false /*isContinue*/);
  if (successors.isEmpty()) {
    return NULL;
  }
  else {
    xassert(successors.length() == 1);
    return successors[0].stmt();
  }
}


// ------------------------- FullExpression ----------------------------
int Interp::interpFullExpression(FullExpression const *fullExpr)
{
  return interpExpression(fullExpr->expr);
}


// ------------------------- ArgExpression -----------------------------
int Interp::interpArgExpression(ArgExpression const *argExpr)
{
  return interpExpression(argExpr->expr);
}


// --------------------------- built-ins -------------------------------
int callBuiltin_getchar(ObjList<int> const &args)
{
  if (args.count() != 1) {
    xfatal("'getchar' requires exactly one argument.");
  }
  int ch = *(args.nthC(0));

  return putchar(ch);
}


// --------------------------- Expression ------------------------------
// Evaluate 'expr', expecting it to name a variable.
static Variable *evaluateToVariable(Expression const *expr)
{
  ASTSWITCHC(Expression, expr) {
    ASTCASEC(E_variable, ev) {
      xassert(ev->var);
      return ev->var;
    }
    ASTDEFAULTC {
      xunimp(stringb("Expression " << expr->kindName()));
    }
    ASTENDCASEC
  }

  return NULL;     // Not reached.
}


int Interp::interpExpression(Expression const *expr)
{
  ASTSWITCHC(Expression, expr) {
    ASTCASEC(E_intLit, lit) {
      return lit->i;
    }

    ASTNEXTC(E_funCall, fc) {
      // Evaluate arguments.
      ObjList<int> arguments;
      FAKELIST_FOREACH(ArgExpression, fc->args, iter) {
        int a = interpArgExpression(iter);
        arguments.append(new int(a));
      }

      // Evaluate callee.
      Variable *calleeVar = evaluateToVariable(fc->func);
      if (calleeVar->scope->isGlobalScope()) {
        if (calleeVar->name == getStringRef("putchar")) {
          return callBuiltin_getchar(arguments);
        }
      }

      xfatal("Call to non-builtin function '" <<
             calleeVar->toString() << "'.");
    }

    ASTDEFAULTC {
      xunimp(stringb("Expression " << expr->kindName()));
    }
    ASTENDCASEC
  }

  return 0;
}


// EOF
