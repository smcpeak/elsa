// clang-import-stub.cc
// Non-functional stub replacement for clang-import.cc.

#include "clang-import.h"              // this module

#include "exc.h"                       // xfatal


void clangParseTranslationUnit(
  ElsaParse &elsaParse,
  std::vector<std::string> const &gccOptions)
{
  xfatal("The Clang import component is not enabled.  To enable it, "
         "set USE_CLANG=1 in personal.mk and rebuild Elsa.");
}


// EOF
