// interp.cc
// Code for interp.h.

#include "interp.h"                    // this module

// smbase
#include "trace.h"                     // TRACE


// ---------------------------- IFrame ---------------------------------
IFrame::IFrame()
  : m_returnValue(0)
{}


IFrame::~IFrame()
{}


// ----------------------------- IEnv ----------------------------------
IEnv::IEnv(StringTable &stringTable,
           TranslationUnit const *translationUnit)
  : m_stringTable(stringTable),
    m_translationUnit(translationUnit),
    m_callStack()
{}


IEnv::~IEnv()
{
  m_callStack.clear();
}


Function const *IEnv::findMain()
{
  // TODO: This should look in the static environment's global scope
  // rather than iterating over all forms.
  StringRef mainName = m_stringTable.add("main");
  FOREACH_ASTLIST(TopForm, m_translationUnit->topForms, iter) {
    TopForm const *tf = iter.data();

    TF_func const *tfunc = tf->ifTF_funcC();
    if (tfunc) {
      PQName const *name = tfunc->f->nameAndParams->decl->getDeclaratorIdC();
      if (name) {
        // Here, we require that the name as declared be unqualified.
        // I think in C++ it would be valid to define "::main", in which
        // case this code would need to be generalized slightly.
        PQ_name const *pqn = name->ifPQ_nameC();
        if (pqn && pqn->name == mainName) {
          return tfunc->f;
        }
      }
    }
  }

  return NULL;
}


int IEnv::interpMain()
{
  TRACE("interp", "start of interpMain");

  Function const *mainFunc = findMain();
  if (!mainFunc) {
    cerr << "Program has no 'main' function.\n";
    return 22;
  }

  IFrame *frame = pushNewFrame();
  mainFunc->interp(*this);

  int ret = frame->m_returnValue;
  popFrame(frame);

  return ret;
}


IFrame *IEnv::pushNewFrame()
{
  m_callStack.push(new IFrame);
  return m_callStack.top();
}


void IEnv::popFrame(IFrame *top)
{
  xassert(m_callStack.top() == top);
  delete m_callStack.pop();
}


IFrame *IEnv::topFrame()
{
  return m_callStack.top();
}


// --------------------------- Function --------------------------------
void Function::interp(IEnv &ienv) const
{
  TRACE("interp", "Beginning execution of '" <<
    nameAndParams->var->toQualifiedString() << "' at " <<
    toString(nameAndParams->var->loc) << ".");

  Statement const *curStatement = this->body;
  while (curStatement) {
    // Interpret the statement and get the next one.
    curStatement = curStatement->interp(ienv);
  }

  TRACE("interp", "Finished execution of '" <<
    nameAndParams->var->toQualifiedString() << "'.");
}


// --------------------------- Statement -------------------------------
Statement const *Statement::interp(IEnv &ienv) const
{
  ASTSWITCHC(Statement, this) {
    ASTCASEC(S_return, r) {
      if (r->expr) {
        int val = r->expr->interp(ienv);
        ienv.topFrame()->m_returnValue = val;
        TRACE("interp", "Return value is " << val << ".");
      }
      return NULL;
    }
    ASTNEXTC1(S_compound) {
      // Use generic handling.
    }
    ASTDEFAULTC {
      xunimp(stringb("Statement " << kindName()));
      return NULL;
    }
    ASTENDCASEC
  }

  // Generic successor handling uses the computed CFG, but requires that
  // the successor be unambiguous.
  NextPtrList successors;
  this->getSuccessors(successors, false /*isContinue*/);
  if (successors.isEmpty()) {
    return NULL;
  }
  else {
    xassert(successors.length() == 1);
    return successors[0].stmt();
  }
}


// ------------------------- FullExpression ----------------------------
int FullExpression::interp(IEnv &ienv) const
{
  return expr->interp(ienv);
}


// --------------------------- Expression ------------------------------
int Expression::interp(IEnv &ienv) const
{
  ASTSWITCHC(Expression, this) {
    ASTCASEC(E_intLit, lit) {
      return lit->i;
    }
    ASTDEFAULTC {
      xunimp(stringb("Expression " << kindName()));
    }
    ASTENDCASEC
  }

  return 0;
}


// EOF
