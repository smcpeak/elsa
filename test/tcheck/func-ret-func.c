// func-ret-func.c
// Check that we reject a function returning a function.

// Invalid.  Elsa rejects this one due to a parse error, although I'm
// not sure that's right since GCC rejects it during type checking.
//ERROR(1): int func_returning_func_returning_int(int)(int);

// Invalid: A function cannot return a function.
//ERROR(2): int (func_returning_func_returning_int(int))(int);

// Ok: A function *can* return a pointer to a function.
int (*func_returning_ptr_to_func_returning_int(int))(int);

// Invalid.  Again, Elsa rejects during parsing.
//ERROR(3): int func_returning_array_of_int(int)[3];

// Invalid: A function cannot return an array.
//ERROR(4): int (func_returning_array_of_int(int))[3];

// Ok: A function *can* return a pointer to an array.
int (*func_returning_ptr_to_array_of_int(int))[3];

// Invalid: Cannot have an array of functions.  Elsa rejects during
// type checking.
//ERROR(5): int array_of_func_returning_int[3](int);

// Also invalid.
//ERROR(6): int (array_of_func_returning_int[3])(int);

// Ok: Can have an array of pointers to functions.
int (*array_of_ptr_to_func_returning_int[3])(int);

// Ok: Can have an array of arrays.
int array_of_array_of_int[3][4];

// EOF
