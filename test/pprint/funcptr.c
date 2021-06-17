// funcptr.c
// Pretty printing of pointer to function, etc.

int x;
int (y);    // redundant parens
int f(int);
int (*ptr_to_func_returning_int)(int);
int const (*ptr_to_func_returning_const_int)(int);
int array_of_int[3];
int array_of_array_of_int[3][4];
int * (*ptr_to_func_returning_ptr_to_int)(int);
int (* (*ptr_to_func_returning_ptr_to_func_returning_int)(int) )(int);
int (*ptr_to_array_of_int)[3];
int (* (*ptr_to_array_of_ptr_to_func_returning_int)[3] )(int);
int (* (*ptr_to_func_returning_ptr_to_array_of_int)(int) )[3];

void client()
{
  x = f(5);
  x = (*ptr_to_func_returning_int)(5);
  x = (*ptr_to_func_returning_const_int)(5);
  x = array_of_int[0];
  x = array_of_array_of_int[0][1];
  x = *((*ptr_to_func_returning_ptr_to_int)(5));
  x = (*(*ptr_to_func_returning_ptr_to_func_returning_int)(5))(6);
  x = ptr_to_func_returning_ptr_to_func_returning_int(5)(6);  // implicit derefs
  x = (*ptr_to_array_of_int)[1];
  x = (* (*ptr_to_array_of_ptr_to_func_returning_int)[1] )(5);
  x = (* (*ptr_to_func_returning_ptr_to_array_of_int)(5) )[1];
}

// EOF
