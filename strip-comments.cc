// strip-comments.cc
// Code for strip-comments.h.

#include "strip-comments.h"            // this module

#include <string.h>                    // strstr


string stripComments(string const &s)
{
  stringBuilder sb;

  char const *p = s.c_str();
  while (char const *start = strstr(p, "/*")) {
    // Everything up to the comment start.
    sb << string(p, start-p);

    char const *end = strstr(start+2, "*/");
    if (!end) {
      // Unterminated comment.  We will just put an indicator into the
      // string, as this is mainly meant for debugging anyway.
      sb << "[[UNTERMINATED COMMENT]]";
      return sb.str();
    }

    // Advance 'p' past the end of comment so we will resume scanning
    // there.
    p = end+2;
  }

  // Finally, add everything else.
  sb << p;
  return sb.str();
}


// EOF
