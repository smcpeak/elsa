// maybe-implicit.h       see license.txt for copyright and terms of use
// Define MaybeImplicit.

#ifndef MAYBE_IMPLICIT_H
#define MAYBE_IMPLICIT_H

// This is a specialized boolean of sorts, specifically do indicate
// whether something is implicit or explicit.  It can be used as a
// boolean, where true means implicit.
//
// This definition does not constrain exactly what "implicit" means;
// that is dependent on the context.
//
enum MaybeImplicit {
  MI_EXPLICIT = 0,
  MI_IMPLICIT = 1
};

#endif // MAYBE_IMPLICIT_H
