// cast-of-union-literal.c
// Type cast applied to a member of a union literal.

// Based on something in Musl src/complex/__cexp.c.

typedef long uint64_t;

uint64_t frexp_exp2(double exp_x)
{
  // The meaning here is we start with 'exp_x', use that to set the
  // '_f' field of a union literal, then extra the '_i' field from that
  // (thereby interpreting the bits of the double as a uint64_t), and
  // finally perform a redundant cast to uint64_t.
  //
  // The reason Elsa trips here is there is a syntactic ambiguity
  // because 'uint64_t' could be parsed as the name of a function, as
  // shown in pprint/ambig-cast-vs-funcall.c.  Consequently, the union
  // definition gets scanned twice, and we complain about a duplicate
  // definition on the second scan.
  return        ((uint64_t)((union{
                               double _f;
                               uint64_t _i;
                             }){exp_x})._i);
}

// EOF
