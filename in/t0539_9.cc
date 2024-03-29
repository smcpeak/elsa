# 1 "t0539.cc"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "t0539.cc"
// t0539.cc
// scope search order test


class Visible {};
class Hidden {};


// Naming scheme:
//
// Names are of the form
//
//   <visible scope>_<hidden scopes>
//
// The name is bound to Visible in the visible scope, and bound to
// Hidden in each of the set of hidden scopes.  Consequently, a lookup
// that looks in the first scope before any of the other ones will
// find Visible, while a lookup that looks in any of the subsequent
// scopes before the first scope will find Hidden (and this will lead
// to a type clash).
//
// "G" stands for the global scope.
//
// These names are used to test the scope searching order by using
// several of these names in a given lookup context, as each
// successful or failing lookup adds a constraint on what the possible
// order is at that point, such that with a relatively small number of
// tests we can firmly establish the exact scope search order.


typedef Visible G_;
typedef Visible G_A;
typedef Visible G_ACD;
typedef Visible G_ABCD;
typedef Visible G_BCD;
typedef Visible G_Q;

typedef Hidden A_G;
typedef Hidden A_QG;
typedef Hidden B_AG;
typedef Hidden B_QAG;
typedef Hidden C_BAG;
typedef Hidden C_QBAG;
typedef Hidden D_CBAG;
typedef Hidden D_QCBAG;
typedef Hidden Q_G;
typedef Hidden E_CBAG;


namespace A {
  typedef Visible A_;
  typedef Visible A_G;
  typedef Visible A_BCD;
  typedef Visible A_QG;

  typedef Hidden B_ACD;
  typedef Hidden B_AG;
  typedef Hidden B_QAG;
  typedef Hidden C_BAG;
  typedef Hidden C_QBAG;
  typedef Hidden D_CBAG;
  typedef Hidden D_QCBAG;
  typedef Hidden E_CBAG;
  typedef Hidden G_A;
  typedef Hidden G_ACD;
  typedef Hidden G_ABCD;

  namespace B {
    typedef Visible B_;
    typedef Visible B_AG;
    typedef Visible B_ACD;
    typedef Visible B_QAG;

    typedef Hidden A_BCD;
    typedef Hidden C_BAG;
    typedef Hidden C_QBAG;
    typedef Hidden D_CBAG;
    typedef Hidden D_QCBAG;
    typedef Hidden E_CBAG;
    typedef Hidden G_ABCD;
    typedef Hidden G_BCD;

    struct C {
      typedef Visible C_;
      typedef Visible C_BAG;
      typedef Visible C_QBAG;

      typedef Hidden A_BCD;
      typedef Hidden B_ACD;
      typedef Hidden D_CBAG;
      typedef Hidden D_QCBAG;
      typedef Hidden E_CBAG;
      typedef Hidden G_ABCD;
      typedef Hidden G_ACD;
      typedef Hidden G_BCD;

      struct D {
        typedef Visible D_;
        typedef Visible D_CBAG;
        typedef Visible D_QCBAG;

        typedef Hidden A_BCD;
        typedef Hidden B_ACD;
        typedef Hidden G_ABCD;
        typedef Hidden G_ACD;
        typedef Hidden G_BCD;

        Visible f1(Visible);
        Visible f2(Visible);
        Visible f3(Visible);
        Visible f4(Visible);
        Visible f5(Visible);
        Visible f6(Visible);
        Visible f7(Visible);
        Visible f8(Visible);
        Visible f9(Visible);

        // lookup order of ret and param: D, C, B, A, global
        D_CBAG g1(D_CBAG);
         C_BAG g2( C_BAG);
          B_AG g3( B_AG);
           A_G g4( A_G);
            G_ g5( G_);
      };

      struct E;
    };

    struct C::E {
      typedef Visible E_;
      typedef Visible E_CBAG;

      Visible f1(Visible);
      Visible f2(Visible);
      Visible f3(Visible);
      Visible f4(Visible);
      Visible f5(Visible);
      Visible f6(Visible);
      Visible f7(Visible);
      Visible f8(Visible);
      Visible f9(Visible);

      // lookup order of ret and param: E, C, B, A, G
      E_CBAG g1(E_CBAG);
       C_BAG g2( C_BAG);
        B_AG g3( B_AG);
         A_G g4( A_G);
          G_ g5( G_);

      //ERROR(9):    G_ g6(    D_);
    };
  }
}


Visible A::B::C::D::g1(Visible) {}
Visible A::B::C::D::g2(Visible) {}
Visible A::B::C::D::g3(Visible) {}
Visible A::B::C::D::g4(Visible) {}
Visible A::B::C::D::g5(Visible) {}

Visible A::B::C::E::g1(Visible) {}
Visible A::B::C::E::g2(Visible) {}
Visible A::B::C::E::g3(Visible) {}
Visible A::B::C::E::g4(Visible) {}
Visible A::B::C::E::g5(Visible) {}
# 320 "t0539.cc"
  namespace Q {
    typedef Visible Q_;
    typedef Visible Q_G;

    typedef Hidden A_QG;
    typedef Hidden B_QAG;
    typedef Hidden C_QBAG;
    typedef Hidden D_QCBAG;
    typedef Hidden G_Q;

    // *not* legal ANSI C++, because definition does not appear in
    // a scope enclosing the declaration; rejected outright by ICC

    // but if we use GNU semantics:
    //   lookup order of ret: Q, global
    //   lookup order of param: D, C, B, A, global
    Q_G A::B::C::D::f1(D_QCBAG) {}
    G_ A::B::C::D::f2( C_QBAG) {}
    G_ A::B::C::D::f3( B_QAG) {}
    G_ A::B::C::D::f4( A_QG) {}

    // GCC looks up in G, and not in Q
    //G_     A::B::C::D::f5(    G_Q) {}

    // but Elsa looks up in Q before G, and it's a lot of work
    // to replicate the GCC bug, so I will just check that I see
    // the proper name in G
    G_ A::B::C::D::f5( G_) {}

    // what is visible inside the function?
    G_ A::B::C::D::f6( G_)
    {
      G_ g;
      //Q_ q;   // not visible to GCC
    }

                G_ A::B::C::D::f8( G_) {}
    //ERROR(1): A_     A::B::C::D::f9(     G_) {}
    //ERROR(2): B_     A::B::C::D::f9(     G_) {}
    //ERROR(3): C_     A::B::C::D::f9(     G_) {}
    //ERROR(4): D_     A::B::C::D::f9(     G_) {}

    // Elsa will probably allow this one, even though GCC does not
    // indeed.
    //nerfed(5): G_     A::B::C::D::f9(     Q_) {}
  }
# 412 "t0539.cc"
// EOF
