// struct-with-flex-array.c
// Several conditions relating to C11 6.7.2.1p3.

// A single flexible array member is invalid.
struct OneFAMember { int flex[]; };              // ERROR

struct SWFA {
  int x;
  int flex[];
};

// Try to embed in another struct.
struct SWSWFA {
  int y;
  struct SWFA s;                                 // ERROR
};

// Try to embed in an array.
struct SWFA awswfa[3];                           // ERROR

// Try to put the flexible array directly into the union.
union UWFA { int flex[]; };                      // ERROR

union UWSWFA1 {
  int x;
  struct SWFA s;
};

// Try to embed in a struct.
struct SWUWSWFA1 { int x; union UWSWFA1 u1; };   // ERROR

// Try to embed in an array.
union UWSWFA1 awuwswfa[3];                       // ERROR

// Put the FA in other than last place.
struct FANotLast {
  int flex[];                                    // ERROR
  int x;
};

// EOF
