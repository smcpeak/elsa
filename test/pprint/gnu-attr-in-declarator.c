// gnu-attr-in-declarator.c
// Uses of GNU '__attribute__' associated with a specific declarator
// rather than the declaration as a whole.

int x __attribute__((unused)),
    y __attribute__((mode(byte)));

int * __attribute__((may_alias)) * __attribute__((may_alias)) ma2;

// This is the next thing to add.
int (__attribute__((unused)) in_paren);

// EOF
