// gnu-attr-in-declaration.c
// Exercise cases of GNU __attribute__ in declarations.

typedef int Integer;
enum E { reeee };
int x;

// nonterm DeclSpecifier:

// -> n:PQTypeName m2:UberModifierSeqOpt uma:AttributeSpecifier_UMAASLOpt
Integer        __attribute__((mode(byte)))                                b1;
Integer extern __attribute__((mode(byte)))                                b2;
Integer        __attribute__((mode(byte)))        __attribute__((unused)) b3;
Integer        __attribute__((mode(byte))) extern                         b4;
Integer        __attribute__((mode(byte))) static __attribute__((unused)) b5;

// -> m1:UberModifierSeq uma1:AttributeSpecifier_UMAASLOpt n:PQTypeName uma2:UMAASLOpt
// Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
static /**/  __attribute__((mode(byte))) /**/   /**/                    Integer /**/                    /**/   d1;
static const __attribute__((mode(byte))) /**/   /**/                    Integer /**/                    /**/   d2;
static const __attribute__((mode(byte))) /**/   __attribute__((unused)) Integer /**/                    /**/   d3;
/**/   const __attribute__((mode(byte))) extern /**/                    Integer /**/                    /**/   d4;
/**/   const __attribute__((mode(byte))) /**/   /**/                    Integer /**/                    extern d5;
static /**/  __attribute__((mode(byte))) /**/   /**/                    Integer __attribute__((unused)) /**/   d6;
/**/   const __attribute__((mode(byte))) /**/   /**/                    Integer __attribute__((unused)) extern d7;
/**/   const __attribute__((mode(byte))) extern __attribute__((unused)) Integer /**/                    /**/   d8;

// -> m1:UberModifierSeq n:PQTypeName m2:UberModifierSeqOpt uma:AttributeSpecifier_UMAASLOpt
static          Integer        __attribute__((mode(byte))) const                          c1;
volatile        Integer extern __attribute__((mode(byte)))                                c2;
volatile static Integer        __attribute__((mode(byte)))        __attribute__((unused)) c3;
static   const  Integer        __attribute__((mode(byte)))                                c4;
volatile extern Integer        __attribute__((mode(byte))) const  __attribute__((unused)) c5;

// -> m1:UberModifierSeq uma1:AttributeSpecifier_UMAASLOpt e:ElaboratedOrSpecifier uma2:UMAASLOpt
// Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
extern __attribute__((unused)) /**/  enum E /**/                        e1;
extern __attribute__((unused)) const enum E /**/                        e2;
extern __attribute__((unused)) /**/  enum E volatile                    e3;
extern __attribute__((unused)) /**/  enum E __attribute__((mode(byte))) e4;

// -> m1:UberModifierSeq e:ElaboratedOrSpecifier m2:UberModifierSeqOpt uma:AttributeSpecifier_UMAASLOpt
// Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
extern enum E __attribute__((unused)) /**/  /**/                        f1;
extern enum E __attribute__((unused)) const /**/                        f2;
extern enum E __attribute__((unused)) /**/  __attribute__((mode(byte))) f3;

// -> te:TypeofTypeSpecifier m2:UberModifierSeqOpt uma:AttributeSpecifier_UMAASLOpt
// Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
__typeof__(x) /**/   __attribute__((unused)) /**/                        /**/  g1;
__typeof__(x) static __attribute__((unused)) /**/                        /**/  g2;
__typeof__(x) static __attribute__((unused)) __attribute__((mode(byte))) /**/  g3;
__typeof__(x) /**/   __attribute__((unused)) /**/                        const g4;
__typeof__(x) /**/   __attribute__((unused)) __attribute__((mode(byte))) const g5;

// -> m1:UberModifierSeq uma1:AttributeSpecifier_UMAASLOpt te:TypeofTypeSpecifier uma2:UMAASLOpt
// Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
static __attribute__((unused)) /**/  /**/                        __typeof__(x) /**/                        /**/     h1;
static __attribute__((unused)) const /**/                        __typeof__(x) /**/                        /**/     h2;
static __attribute__((unused)) /**/  __attribute__((mode(byte))) __typeof__(x) /**/                        /**/     h3;
static __attribute__((unused)) const /**/                        __typeof__(x) __attribute__((mode(byte))) /**/     h4;
static __attribute__((unused)) const /**/                        __typeof__(x) __attribute__((mode(byte))) volatile h5;

// -> m1:UberModifierSeq te:TypeofTypeSpecifier m2:UberModifierSeqOpt uma:AttributeSpecifier_UMAASLOpt
// Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
extern __typeof__(x) __attribute__((unused)) /**/  /**/                        k1;
extern __typeof__(x) __attribute__((unused)) const /**/                        k2;
extern __typeof__(x) __attribute__((unused)) const __attribute__((mode(byte))) k3;


// EOF
