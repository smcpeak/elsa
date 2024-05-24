/* iptparse.lex */
/* lexer for iptree parser */

%smflex 101

/* ------------ C prelude ------------ */
%{
#include "iptparse.h"       // token definitions
#include "exc.h"            // xfatal
#include "xassert.h"        // xassert

int lexerSval = 0;
%}


/* ------------ flex options ----------- */
/* don't use the default-echo rules */
%option nodefault


/* -------------- token rules ------------ */
%%

  /* punctuation */
","              { return TOK_COMMA; }
";"              { return TOK_SEMICOLON; }

  /* decimal integer literal */
[0-9]+ {
  lexerSval = atoi(YY_TEXT);
  return TOK_INTLIT;
}

  /* whitespace; ignore */
[ \t\n\f\v\r]+  {
}

  /* C++ comment */
"//".*    {
}

.  {
  xfatal("illegal character: `" << YY_TEXT[0] << "'");
}

<<EOF>> {
  YY_TERMINATE();
}


%%
/* ----------------- C epilogue --------------- */

TokenType getNextToken(yy_lexer_t *lexer)
{
  int code = yy_lex(lexer);
  xassert(0 <= code && code < NUM_TOKENTYPES);
  return (TokenType)code;
}

  /* EOF */
