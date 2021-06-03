// iptparse.h
// interface to the interval partition tree parser

#ifndef IPTPARSE_H
#define IPTPARSE_H

#include <stdio.h>                     // FILE
#include "str.h"                       // rostring
#include "iptparse.yy.h"               // yy_lexer_t

class IPTree;                          // iptree.h

// the lexer stashes lexed integer codes here
extern int lexerSval;

// token codes
enum TokenType {
  TOK_EOF=0,          // end of file
  TOK_INTLIT,         // integer literal
  TOK_COMMA,          // ","
  TOK_SEMICOLON,      // ";"

  NUM_TOKENTYPES
};

// call this to get the next token (defined in iptparse.yy.cc)
TokenType getNextToken(yy_lexer_t *lexer);

// parse a file into a given tree
IPTree *parseFile(rostring fname);


#endif // IPTPARSE_H
