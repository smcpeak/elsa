// c11-6.9.2-tentative-definitions.c
// Test C "tentative definitions" rules.


struct Incomplete;

// This would be ok if 'Incomplete' had a later definition.
//ERROR(tentative-incomplete): struct Incomplete inc2;
//NOTWORKING(elsa): Elsa fails to diagnose.


struct LaterComplete;

// It is an error to try to give a tentative definition of a "static"
// object with incomplete type (C11 6.9.2p3).  GCC does not diagnose
// this but Clang does.
//ERROR(static-tentative-incomplete): static struct LaterComplete inc1;
//NOTWORKING(gcc): GCC fails to diagnose.
//NOTWORKING(elsa): Elsa fails to diagnose.

// It is ok to have a tentative definition with no storage class and
// an incomplete type.
struct LaterComplete later1;

// So long as the type is completed by the end.
struct LaterComplete {
  int x;
  int y;
};


struct S { int x; int y; };

// Tentative, but not used.
struct S tentThenFirm;

// Firm definition.
struct S tentThenFirm = { 1,2 };


struct S tentThenFirmThenTent;
struct S tentThenFirmThenTent = { 3,4 };
struct S tentThenFirmThenTent;


// Two tentative definitions, no firm definition.
int tentTent;
int tentTent;


void __elsa_checkTentativeDefinitions(int dummy, ...) {}
int main()
{
  __elsa_checkTentativeDefinitions(0,
    // I provide these strings in the error cases to ensure the test
    // continues to erroneously *not* fail, since Elsa does not enforce
    // the relevant rule.
    //ERROR(static-tentative-incomplete): "inc1 "
    //ERROR(tentative-incomplete): "inc2 "

    "later1 "
    "tentTent "
  );

  return
    later1.x == 0 &&
    later1.y == 0 &&
    tentThenFirm.x == 1 &&
    tentThenFirm.y == 2 &&
    tentThenFirmThenTent.x == 3 &&
    tentThenFirmThenTent.y == 4 &&
    tentTent == 0 &&
    1? 0 : 1;
}


// EOF
