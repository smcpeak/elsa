// gnu-attr-in-type-specifier.c
// GNU attributes in TypeSpecifiers.

int f(__attribute__((mode(byte))) int b1,
      int __attribute__((mode(byte))) b2);

int f(__attribute__((mode(byte))) int b1,
      int __attribute__((mode(byte))) b2)
{
  return b1+b2;
}


typedef int Integer;
enum E { reeee };
int x;


// nonterm TypeSpecifier:

int a1(
  // -> n:PQTypeName cv2:UberCVQualifierSeqOpt uma:AttributeSpecifier_UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  Integer /**/  __attribute__((unused)) /**/                        /**/     /**/                        /**/  p1,
  Integer const __attribute__((unused)) /**/                        /**/     /**/                        /**/  p2,
  Integer const __attribute__((unused)) /**/                        volatile /**/                        /**/  p3,
  Integer const __attribute__((unused)) /**/                        volatile __attribute__((mode(byte))) /**/  p4,
  Integer /**/  __attribute__((unused)) /**/                        volatile __attribute__((mode(byte))) const p5,
  Integer /**/  __attribute__((unused)) __attribute__((mode(byte))) volatile /**/                        const p6,
  int z);

int a2(
  // -> cv1:UberCVQualifierSeq uma1:AttributeSpecifier_UCVAASLOpt n:PQTypeName uma2:UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  const __attribute__((unused)) /**/     Integer /**/     /**/                        p1,
  const __attribute__((unused)) volatile Integer /**/     /**/                        p2,
  const __attribute__((unused)) /**/     Integer volatile /**/                        p3,
  const __attribute__((unused)) /**/     Integer volatile __attribute__((mode(byte))) p4,
  int z);

int a3(
  // -> cv1:UberCVQualifierSeq n:PQTypeName cv2:UberCVQualifierSeqOpt uma:AttributeSpecifier_UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  volatile Integer /**/  __attribute__((unused)) /**/                        /**/  p1,
  volatile Integer const __attribute__((unused)) /**/                        /**/  p2,
  volatile Integer /**/  __attribute__((unused)) __attribute__((mode(byte))) /**/  p3,
  volatile Integer /**/  __attribute__((unused)) __attribute__((mode(byte))) const p4,
  int z);

int a4(
  // -> k1:UberTypeKeyword m2:UberTypeAndCVQualifierSeqOpt uma:AttributeSpecifier_UTACVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  int /**/  __attribute__((unused)) /**/                        p1,
  int const __attribute__((unused)) /**/                        p2,
  int /**/  __attribute__((unused)) __attribute__((mode(byte))) p3,
  int const __attribute__((unused)) __attribute__((mode(byte))) p4,
  int z);

int a5(
  // -> m1:UberCVQualifierSeq uma1:AttributeSpecifier_UCVAASLOpt k1:UberTypeKeyword uma2:UTACVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  const __attribute__((unused)) /**/     /**/                        int /**/     /**/                        /**/ p1,
  const __attribute__((unused)) volatile /**/                        int /**/     /**/                        /**/ p2,
  const __attribute__((unused)) volatile __attribute__((mode(byte))) int /**/     /**/                        /**/ p3,
  const __attribute__((unused)) /**/     __attribute__((mode(byte))) int /**/     /**/                        /**/ p4,
  const __attribute__((unused)) /**/     /**/                        int volatile /**/                        /**/ p5,
  const __attribute__((unused)) /**/     /**/                        int volatile __attribute__((mode(byte))) /**/ p6,
  const __attribute__((unused)) /**/     /**/                        int long     __attribute__((mode(byte))) /**/ p7,
  const __attribute__((unused)) /**/     /**/                        int volatile __attribute__((mode(byte))) long p8,
  int z);

int a6(
  // -> m1:UberCVQualifierSeq k1:UberTypeKeyword m2:UberTypeAndCVQualifierSeqOpt uma:AttributeSpecifier_UTACVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  const int /**/     __attribute__((unused)) /**/ /**/                        p1,
  const int volatile __attribute__((unused)) /**/ /**/                        p2,
  const int /**/     __attribute__((unused)) long /**/                        p3,
  const int /**/     __attribute__((unused)) long __attribute__((mode(byte))) p4,
  int z);

int a7(
  // -> e:ElaboratedOrSpecifier m2:UberCVQualifierSeqOpt uma:AttributeSpecifier_UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  enum E /**/  __attribute__((unused)) /**/  /**/                      p1,
  enum E const __attribute__((unused)) /**/  /**/                      p2,
  enum E /**/  __attribute__((unused)) const /**/                      p3,
  enum E /**/  __attribute__((unused)) const __attribute((mode(byte))) p4,
  int z);

int a8(
  // -> m1:UberCVQualifierSeq uma1:AttributeSpecifier_UCVAASLOpt e:ElaboratedOrSpecifier uma2:UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  const __attribute__((unused)) /**/     enum E /**/     /**/                      p1,
  const __attribute__((unused)) volatile enum E /**/     /**/                      p2,
  const __attribute__((unused)) /**/     enum E volatile /**/                      p3,
  const __attribute__((unused)) /**/     enum E volatile __attribute((mode(byte))) p4,
  int z);

int a9(
  // -> m1:UberCVQualifierSeq e:ElaboratedOrSpecifier m2:UberCVQualifierSeqOpt uma:AttributeSpecifier_UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  const enum E /**/     __attribute__((unused)) /**/                      /**/     p1,
  const enum E volatile __attribute__((unused)) /**/                      /**/     p2,
  const enum E /**/     __attribute__((unused)) /**/                      volatile p3,
  const enum E /**/     __attribute__((unused)) __attribute((mode(byte))) volatile p5,
  int z);

int a10(
  // -> te:TypeofTypeSpecifier cv2:UberCVQualifierSeqOpt uma:AttributeSpecifier_UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  __typeof__(x) /**/  __attribute__((unused)) /**/  /**/                        p1,
  __typeof__(x) const __attribute__((unused)) /**/  /**/                        p2,
  __typeof__(x) /**/  __attribute__((unused)) const /**/                        p3,
  __typeof__(x) /**/  __attribute__((unused)) const __attribute__((mode(byte))) p4,
  int z);

int a11(
  // -> cv1:UberCVQualifierSeq uma1:AttributeSpecifier_UCVAASLOpt te:TypeofTypeSpecifier uma2:UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  const __attribute__((unused)) /**/     /**/                        __typeof__(x) p1,
  const __attribute__((unused)) volatile /**/                        __typeof__(x) p2,
  const __attribute__((unused)) volatile __attribute__((mode(byte))) __typeof__(x) p3,

  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  const __attribute__((unused)) __typeof__(x) /**/                        volatile p4,
  const __attribute__((unused)) __typeof__(x) __attribute__((mode(byte))) volatile p5,

  int z);

int a12(
  // -> cv1:UberCVQualifierSeq te:TypeofTypeSpecifier cv2:UberCVQualifierSeqOpt uma:AttributeSpecifier_UCVAASLOpt   forbid_next("__attribute__")
  // Columns: \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+
  const __typeof__(x) /**/     __attribute__((unused)) /**/                      /**/     p1,
  const __typeof__(x) volatile __attribute__((unused)) /**/                      /**/     p2,
  const __typeof__(x) volatile __attribute__((unused)) __attribute((mode(byte))) /**/     p3,
  const __typeof__(x) /**/     __attribute__((unused)) __attribute((mode(byte))) volatile p4,
  int z);

// EOF
