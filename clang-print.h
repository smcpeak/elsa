// clang-print.h
// Routines for printing elements of the libclang AST.

#ifndef ELSA_CLANG_PRINT_H
#define ELSA_CLANG_PRINT_H

// clang
#include "clang-c/Index.h"             // CXCursorKind, etc.

// libc++
#include <iosfwd>                      // std::ostream
#include <string>                      // std::string
#include <vector>                      // std::vector


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

// Return the location as "<line>:<col>".
std::string toLCString(CXSourceLocation loc);

// Return cursor location as "<line>:<col>".
std::string cursorLocLC(CXCursor cursor);

// Return a storage class name like "Static".
char const *toString(CX_StorageClass storageClass);

// Return the spelling of 'type'.
std::string typeSpelling(CXType type);

// Return all children of 'cursor'.
std::vector<CXCursor> getChildren(CXCursor cursor);


// Class to print Clang AST.
class ClangPrint {
public:      // data
  // Stream to write to.
  std::ostream &m_os;

  // If true, print type details too.
  bool m_printTypes;

  // If non-zero print up to this many tokens from each node.
  int m_maxTokensToPrint;

private:     // methods
  bool maybePrintType(char const *label, CXType cxType);

public:      // methods
  ClangPrint(std::ostream &os)
    : m_os(os),
      m_printTypes(true),
      m_maxTokensToPrint(0)
  {}

  // Print the subtree rooted at 'cursor'.
  void printCursor(CXCursor cursor, int indent);

  // Print up to 'm_maxTokensToPrint' tokens of 'cursor'.
  void printTokens(CXCursor cursor, int indent);

  // Print the type tree rooted at 'cxType'.
  void printTypeTree(CXType cxType, int indent);
};



#endif // ELSA_CLANG_PRINT_H
