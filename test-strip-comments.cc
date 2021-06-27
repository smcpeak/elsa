// test-strip-comments.cc
// Test code for strip-comments.

#include "strip-comments.h"            // module under test

// smbase
#include "sm-test.h"                   // EXPECT_EQ


static void sc_one(string input, string expect)
{
  string actual = stripComments(input);
  EXPECT_EQ(actual, expect);
}


void strip_comments_unit_tests()
{
  sc_one("", "");
  sc_one("abc", "abc");
  sc_one("123/*456*/789", "123789");
  sc_one("123/*4/*5*/6*/789", "1236*/789");
  sc_one("123/*4", "123[[UNTERMINATED COMMENT]]");
  sc_one("123456*/789", "123456*/789");
  sc_one("123/*4*/56/*7*/89", "1235689");
  sc_one("123/*4**/56/*7*/89", "1235689");
  sc_one("123/*4*//56/*7*/89", "123/5689");
}


// EOF
