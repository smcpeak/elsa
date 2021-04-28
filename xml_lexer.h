// xml_lexer.h           see license.txt for copyright and terms of use

#ifndef XML_LEXER_H
#define XML_LEXER_H

#include <stdio.h>
#include "sm-fstream.h"         // ifstream

#include "str.h"                // string
#include "xml_enum.h"           // XTOK_*

// Get the definition of 'xmlBaseFlexLexer' by #including FlexLexer.h
// with a customized definition of 'yyFlexLexer'.
#ifndef XML_BASE_LEXER_DEFINED
#define XML_BASE_LEXER_DEFINED
#undef yyFlexLexer
#define yyFlexLexer xmlBaseFlexLexer
#include "FlexLexer.h"          // xmlBaseFlexLexer
#endif // XML_BASE_LEXER_DEFINED

class XmlLexer : private xmlBaseFlexLexer {
private:
  inline int yyunderflow(); // helper for read_xml_string
  bool read_xml_string();   // used by lexer when reading strings

public:
  char const *inputFname;       // just for error messages
  int linenumber;
  bool sawEof;

  XmlLexer()
    : inputFname(NULL)
    , linenumber(1)             // file line counting traditionally starts at 1
    , sawEof(false)
  {}

  // this is yylex() but does what I want it to with EOF
  int getToken();
  // have we seen the EOF?
  bool haveSeenEof() { return sawEof; }

  // This is yytext.  Strings are already dequoted+unescaped.
  char const *currentText() { return this->YYText(); }

  // this is yyrestart.  For starting and restarting.
  void restart(istream *in) { this->yyrestart(in); sawEof = false; }

  int tok(XmlToken kind);
  int svalTok(XmlToken t);
  void err(char const *msg);

  string tokenKindDesc(int kind) const;
  string tokenKindDescV(int kind) const;

  // Generated lexer implementation.
  virtual int yylex();
};

#endif // XML_LEXER_H
