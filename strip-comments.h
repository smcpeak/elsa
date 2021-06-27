// strip-comments.h
// stripComments function.

// It is a little weird to have an entire module for one function, but
// it doesn't seem to fit well in any other.

#ifndef ELSA_STRIP_COMMENTS_H
#define ELSA_STRIP_COMMENTS_H

// smbase
#include "str.h"                       // string

// Return 's' with any "/*...*/" comments removed.
//
// This is used in a few places where I have 'toString()' methods that
// print something C comments, but what is inside them must not have any
// C comments because they don't nest.
string stripComments(string const &s);

// Unit tests.  Defined in test-strip-comments.cc.
void strip_comments_unit_tests();

#endif // ELSA_STRIP_COMMENTS_H
