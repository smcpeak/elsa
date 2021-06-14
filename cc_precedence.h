// cc_precedence.h
// OperatorPrecedence enum.

#ifndef ELSA_CC_PRECEDENCE_H
#define ELSA_CC_PRECEDENCE_H

// Operator precedence levels, where a lower numeric value means higher
// precedence.
//
// This is used by the pretty printer to decide where to put
// parentheses.
//
// Based on https://en.cppreference.com/w/cpp/language/operator_precedence.
enum OperatorPrecedence {
  // Artificial entry with higher precedence than any other.  This is
  // for expressions such as literals that never need parentheses.
  OPREC_HIGHEST         =  0,

  // Entries from the table, numbered accordingly.
  OPREC_SCOPE_QUALIFIER =  1,     // ::
  OPREC_POSTFIX         =  2,     // a++ a-- t() t{} f() a[] . ->
  OPREC_PREFIX          =  3,     // ++a --a +a -a ! ~ (t) *a &a sizeof
                                  // co_await new new[] delete delete[]
  OPREC_PTR_TO_MEMB     =  4,     // .* ->*
  OPREC_MULTIPLY        =  5,     // * / %
  OPREC_ADD             =  6,     // + -
  OPREC_SHIFT           =  7,     // << >>
  OPREC_SPACESHIP       =  8,     // <=>
  OPREC_RELATIONAL      =  9,     // < <= > >=
  OPREC_EQUALITY        = 10,     // == !=
  OPREC_BIT_AND         = 11,     // &
  OPREC_BIT_XOR         = 12,     // ^
  OPREC_BIT_OR          = 13,     // |
  OPREC_LOGICAL_AND     = 14,     // &&
  OPREC_LOGICAL_OR      = 15,     // ||
  OPREC_ASSIGN          = 16,     // ?: throw co_yield = += -= ...
  OPREC_COMMA           = 17,     // ,

  // Artificial entry for precedence lower than any real operator.  This
  // is used when printing, e.g., a statement expression, and indicates
  // that it never needs parentheses.
  OPREC_LOWEST          = 18,

  NUM_OPERATOR_PRECEDENCES
};


#endif // ELSA_CC_PRECEDENCE_H
