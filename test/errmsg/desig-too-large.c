// desig-too-large.c
// Designator expression too large for array.

enum { fifty = 50 };

int arr2[2] = { [fifty] = 11 };

// EOF
