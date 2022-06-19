// import-clang.h
// Read the Clang AST and create Elsa AST.

#ifndef ELSA_IMPORT_CLANG_H
#define ELSA_IMPORT_CLANG_H

#include "cc-ast-fwd.h"                // TranslationUnit
#include "elsaparse-fwd.h"             // ElsaParse

#include <vector>                      // std::vector
#include <string>                      // std::string


// Run the clang parser on command line 'gccOptions', which does *not*
// include the name of the compiler.  For example: ["foo.cc"].
//
// Populates 'elsaParse' in a manner similar to calling its 'parse'
// method.  It does not matter how 'elsaParse' has been configured, such
// as language options.
//
// Throws an exception on error.
void clangParseTranslationUnit(
  ElsaParse &elsaParse,
  std::vector<std::string> const &gccOptions);


#endif // ELSA_IMPORT_CLANG_H
