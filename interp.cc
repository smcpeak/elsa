// interp.cc
// Code for interp.h.

#include "interp.h"                    // this module

// smbase
#include "trace.h"                     // TRACE


IEnv::IEnv(StringTable &stringTable,
           TranslationUnit const *translationUnit)
  : m_stringTable(stringTable),
    m_translationUnit(translationUnit)
{}


IEnv::~IEnv()
{}


Function const *IEnv::findMain()
{
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

  cout << "Found 'main' at " << toString(mainFunc->getLoc()) << ".\n";
  return 0;
}


// EOF
