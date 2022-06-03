// c11-6.7.2.1p3-flexible-array.c
// Check rules for flexible arrays in structs and unions.

// "A structure or union shall not contain a member with incomplete or
// function type (hence, a structure shall not contain an instance of
// itself, but may contain a pointer to an instance of itself), except
// that the last member of a structure with more than one named member
// may have incomplete array type; such a structure (and any union
// containing, possibly recursively, a member that is such a structure)
// shall not be a member of a structure or an element of an array."

// Interpretation:
//
// 1. As a special case, a struct with at least two members can have as
//    its last member an array with unspecified size.  Call this a
//    "Struct With Flexible Array" (SWFA).
//
// 2. An SWFA cannot be a member of another structure, nor an element of
//    an array.
//
// 3. A union may contain an SWFA in any position, although it cannot
//    directly contain the flexible array.  Call this a "Union With
//    SWFA" (UWSWFA).  Furthermore, a UWSWFA can contain another
//    UWSWFA, recursively.  Like an SWFA, a UWSWFA cannot be the member
//    of a structure nor an element of an array.


// ------------------------------ Rule 1 -------------------------------
// A single flexible array member is invalid.
//ERROR(one-fa-member): struct OneFAMember { int flex[]; };
//NOTWORKING(elsa): Rule not enforced.

struct SWFA {
  int x;
  int flex[];
};

static int test_swfa()
{
  int a[5] = { 1,2,3,4,5 };

  // C11 6.7.2.1p18 explains that the effect of this is as if 'flex'
  // were declared to have size 4, since that would make it just fit in
  // 'a'.
  struct SWFA *p = (struct SWFA *)a;

  return
    p->x == 1 &&
    p->flex[0] == 2 &&
    p->flex[1] == 3 &&
    p->flex[2] == 4 &&
    p->flex[3] == 5 &&
    1;
}


// ------------------------------ Rule 2 -------------------------------
// Try to embed in another struct.
struct SWSWFA {
  int y;
  //ERROR(swswfa): struct SWFA s;
};

// Try to embed in an array.
//ERROR(awswfa): struct SWFA awswfa[3];


// ------------------------------ Rule 3 -------------------------------
// Try to put the flexible array directly into the union.
//ERROR(uwfa): union UWFA { int flex[]; };

union UWSWFA1 {
  int x;
  struct SWFA s;
};

// Try to embed in a struct.
//ERROR(swuwswfa): struct SWUWSWFA1 { int x; union UWSWFA1 u1; };

// Try to embed in an array.
//ERROR(awuwswfa): union UWSWFA1 awuwswfa[3];

union UWSWFA2 {
  int x;
  union UWSWFA1 u1;
  struct SWFA s;
};

union UWSWFA3 {
  int x;
  union UWSWFA1 u1;
  union UWSWFA2 u2;
  struct SWFA s;
};

static int test_uwswfa()
{
  int a[5] = { 1,2,3,4,5 };

  union UWSWFA1 *u1 = (union UWSWFA1 *)a;
  union UWSWFA2 *u2 = (union UWSWFA2 *)a;
  union UWSWFA3 *u3 = (union UWSWFA3 *)a;

  return
    u1->x == 1 &&
    u1->s.x == 1 &&
    u1->s.flex[0] == 2 &&
    u1->s.flex[1] == 3 &&
    u1->s.flex[2] == 4 &&
    u1->s.flex[3] == 5 &&

    u2->x == 1 &&
    u2->u1.x == 1 &&
    u2->u1.s.x == 1 &&
    u2->u1.s.flex[0] == 2 &&
    u2->u1.s.flex[1] == 3 &&
    u2->u1.s.flex[2] == 4 &&
    u2->u1.s.flex[3] == 5 &&
    u2->s.x == 1 &&
    u2->s.flex[0] == 2 &&
    u2->s.flex[1] == 3 &&
    u2->s.flex[2] == 4 &&
    u2->s.flex[3] == 5 &&

    u3->x == 1 &&
    u3->u1.x == 1 &&
    u3->u1.s.x == 1 &&
    u3->u1.s.flex[0] == 2 &&
    u3->u1.s.flex[1] == 3 &&
    u3->u1.s.flex[2] == 4 &&
    u3->u1.s.flex[3] == 5 &&
    u3->u2.x == 1 &&
    u3->u2.u1.x == 1 &&
    u3->u2.u1.s.x == 1 &&
    u3->u2.u1.s.flex[0] == 2 &&
    u3->u2.u1.s.flex[1] == 3 &&
    u3->u2.u1.s.flex[2] == 4 &&
    u3->u2.u1.s.flex[3] == 5 &&
    u3->u2.s.x == 1 &&
    u3->u2.s.flex[0] == 2 &&
    u3->u2.s.flex[1] == 3 &&
    u3->u2.s.flex[2] == 4 &&
    u3->u2.s.flex[3] == 5 &&
    u3->s.x == 1 &&
    u3->s.flex[0] == 2 &&
    u3->s.flex[1] == 3 &&
    u3->s.flex[2] == 4 &&
    u3->s.flex[3] == 5 &&

    1;
}


// ------------------------------- main --------------------------------
int main()
{
  return
    test_swfa() &&
    test_uwswfa() &&
    1? 0 : 1;
}

// EOF
