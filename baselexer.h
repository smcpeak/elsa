// baselexer.h            see license.txt for copyright and terms of use
// flex-lexer base class; used by C/C++ lexer, among others.
//
// The key property that differentiates baselexer from lexer is
// that the latter knows the details of what tokens exist and
// what their semantic values mean, whereas the former does not.
// Therefore, I can re-use baselexer for other languages, and
// let lexer be the C++-specific one.

#ifndef BASELEXER_H
#define BASELEXER_H

#include "lexer.yy.h"       // yyFlexLexer

#include "sm-iostream.h"    // istream
#include "lexerint.h"       // LexerInterface
#include "strtable.h"       // StringRef, StringTable


// lexer object
class BaseLexer : public yyFlexLexer, public LexerInterface {
protected:  // data
  istream *inputStream;            // (owner) file from which we're reading
  SourceLocManager::File *srcFile; // (serf) contains the hash map we update

  SourceLoc nextLoc;               // location of *next* token
  int curLine;                     // current line number; needed for #line directives

public:     // data
  StringTable &strtable;           // string table
  int errors;                      // count of errors encountered
  int warnings;                    // same for warnings

private:    // funcs
  BaseLexer(BaseLexer&);           // disallowed

protected:  // funcs
  // advance source location
  void updLoc() {
    loc = nextLoc;                 // location of *this* token
    nextLoc = advText(nextLoc, yym_text(), yym_leng());
  }

  // adds a string with only the specified # of chars
  StringRef addString(char const *str, int len);

  // updLoc(), then for every newline found in
  // [yym_text(),yym_text()+yym_leng()-1], increment 'curLine'
  void whitespace();

  // return the given token code, after updLoc'ing and setting
  // 'sval' to NULL; suitable for tokens with one spelling (or
  // otherwise have no semantic value)
  int tok(int t);

  // report an error
  void err(char const *msg);
  void warning(char const *msg);

  // part of the constructor
  istream *openFile(char const *fname);
  istream *openString(char const *buf, int len);

  // read the next token and return its code; returns 0 for end of file;
  // this function is defined by the derived class, in flex's output
  // source code
  virtual int yym_lex() = 0;

public:     // funcs
  // make a lexer to scan the given file
  BaseLexer(StringTable &strtable, char const *fname);

  // make a lexer to scan an in-memory string; 'initLoc' is the
  // location that the first character should be regarded as being at;
  // the buffer must remain allocated as long as this BaseLexer is
  BaseLexer(StringTable &strtable, SourceLoc initLoc,
            char const *buf, int len);

  ~BaseLexer();

  static void tokenFunc(LexerInterface *lex);

  // LexerInterface funcs
  virtual NextTokenFunc getTokenFunc() const;
};


#endif // BASELEXER_H
