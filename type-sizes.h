// type-sizes.h
// TypeSizes class.

#ifndef ELSA_TYPE_SIZES_H
#define ELSA_TYPE_SIZES_H

#include "type-sizes-fwd.h"            // forward for this module

// elsa
#include "scalar-type-set.h"           // ScalarTypeSet

// smbase
#include "str.h"                       // string


// Configurable map from scalar type to size and alignment.
class TypeSizes {
private:     // data
  // Size for each set.
  int m_stsSize[NUM_SCALAR_TYPE_SETS];

public:      // class methods
  // Returns "BOOL", etc.
  static char const *stsName(ScalarTypeSet sts);

public:      // methods
  TypeSizes() { set_build_compiler(); }

  // Get the size of 'sts'.
  int getSize(ScalarTypeSet sts) const;

  // Use the sizes that the compiler used to compie Elsa uses.
  void set_build_compiler();

  // Configure to emulate specific platforms.
  void set_linux_x86_64();
  void set_windows_x86_64();
  void set_windows_x86();

  // Return a string where each line specifies one of the sizes.  This
  // is meant for debugging purposes.
  string toString() const;
};


#endif // ELSA_TYPE_SIZES_H
