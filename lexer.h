// lexer.h            see license.txt for copyright and terms of use
// lexer for C and C++ source files

#ifndef LEXER_H
#define LEXER_H

#include "baselexer.h"      // BaseLexer
#include "cc-lang-fwd.h"    // CCLang
#include "cc-tokens.h"      // TokenType


// bounds-checking functional interfaces to tables declared in cc-tokens.h
char const *toString(TokenType type);
TokenFlag tokenFlags(TokenType type);


// lexer object
class Lexer : public BaseLexer {
private:    // data
  bool prevIsNonsep;               // true if last-yielded token was nonseparating
  StringRef prevHashLineFile;      // previously-seen #line directive filename

public:     // data
  CCLang &lang;                    // language options

protected:  // funcs
  // see comments at top of lexer.cc
  void checkForNonsep(TokenType t) {
    if (tokenFlags(t) & TF_NONSEPARATOR) {
      if (prevIsNonsep) {
        err("two adjacent nonseparating tokens");
      }
      prevIsNonsep = true;
    }
    else {
      prevIsNonsep = false;
    }
  }

  // consume whitespace
  void whitespace();

  // do everything for a single-spelling token
  int tok(TokenType t);

  // do everything for a multi-spelling token
  int svalTok(TokenType t);

  // C++ "alternate keyword" token
  int alternateKeyword_tok(TokenType t);

  // handle a #line directive
  void parseHashLine(char const *directive, int len);

  // report an error in a preprocessing task
  void pp_err(char const *msg);

  // The core scanner method.
  int yym_lex() override;

public:     // funcs
  // make a lexer to scan the given file
  Lexer(StringTable &strtable, CCLang &lang, char const *fname);
  Lexer(StringTable &strtable, CCLang &lang, SourceLoc initLoc,
        char const *buf, int len);
  ~Lexer();

  static void tokenFunc(LexerInterface *lex);
  static void c_tokenFunc(LexerInterface *lex);

  // LexerInterface funcs
  virtual NextTokenFunc getTokenFunc() const override;
  virtual string tokenDesc() const override;
  virtual string tokenKindDesc(int kind) const override;

  string tokenKindDescV(int kind) const;
};


#endif // LEXER_H
