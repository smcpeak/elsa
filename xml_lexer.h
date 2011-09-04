// xml_lexer.h           see license.txt for copyright and terms of use

#ifndef XML_LEXER_H
#define XML_LEXER_H

#include <stdio.h>
#include "fstream.h"            // ifstream

#include "str.h"                // string
#include "sm_flexlexer.h"       // yyFlexLexer
#include "baselexer.h"          // FLEX_OUTPUT_METHOD_DECLS
#include "xml_enum.h"           // XTOK_*

class XmlLexer : private yyFlexLexer {
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

  FLEX_OUTPUT_METHOD_DECLS
};

#endif // XML_LEXER_H
