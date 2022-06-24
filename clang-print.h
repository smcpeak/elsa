// clang-print.h
// Routines for printing elements of the libclang AST.

#ifndef ELSA_CLANG_PRINT_H
#define ELSA_CLANG_PRINT_H

// clang
#include "clang-c/Index.h"             // CXCursorKind, etc.

// libc++
#include <string>                      // std::string


// Consume (dispose) a CXString, returning its contents as a
// std::string.
std::string toString(CXString cxString);

// Return things like "DeclRefExpr".
std::string toString(CXCursorKind cursorKind);

// Return a string of letters indicating the classification of the kind,
// for example including 'd' for 'clang_isDeclaration'.
std::string cursorKindClassificationsString(CXCursorKind cursorKind);

// Return things like "Pointer".
std::string toString(CXTypeKind typeKind);


#endif // ELSA_CLANG_PRINT_H
