// gnu-attr-on-conversion-id.cc
// GNU __attribute__ on a conversion-id, which has a possible problem
// with grammatical ambiguity.

/*
Interpretation 1:

    operator char __attribute__((...)) __attribute__((...))
       |      |            |                    |
       |     UTK   AttributeSpecifier   AttributeSpecifier
       |      |            |                    |
       |      |       AttributeSpecifier_UTACVAASLOpt
       |      |        |
       |     TypeSpecifier
       |          |
     ConversionFunctionId
              |
       DirectDeclarator
              |
          Declarator

Intepretation 2:

    operator char __attribute__((...)) __attribute__((...))
       |      |            |               |
       |     UTK    AS_UTACVAASLOpt        |
       |      |       |                    |
       |     TypeSpecifier                 |
       |          |                        |
     ConversionFunctionId                  |
              |                            |
       DirectDeclarator    AttributeSpecifierList
                     |      |
                    Declarator
*/

struct S {
  // This is the declaration diagrammed above.
  operator char __attribute__(()) __attribute__(()) ();

  // Variations on attribute placement.
  operator __attribute__(()) __attribute__(()) int ();
  operator __attribute__(()) float __attribute__(()) ();

  // TODO: More variations.
};

// EOF
