// subobject-access-path.h
// Class SubobjectAccessPath.

#ifndef ELSA_SUBOBJECT_ACCESS_PATH_H
#define ELSA_SUBOBJECT_ACCESS_PATH_H

#include "subobject-access-path-fwd.h" // fwds for this module

// elsa
#include "cc-env.h"                    // Env
#include "cc-type-fwd.h"               // Type

// smbase
#include "xassert.h"                   // xassert

// libc++
#include <string>                      // std::string
#include <vector>                      // std::vector


// This is a sequence of steps to navigate through some hierarchical
// object (like an array of structs containing arrays, etc.) down to a
// particular subobject.
class SubobjectAccessPath {
  // There wouldn't be a technical problem making a copy, but the
  // algorithm that uses this class is not intended to make copies, and
  // I'd like to ensure that intent is fulfilled.
  NO_OBJECT_COPIES(SubobjectAccessPath);

private:     // data
  // When the object is an array, the integer is an array index.  When
  // the object is a compound (struct/class/union), it is the index of a
  // field in CompoundType::dataMembers.
  //
  // TODO: The use of 'int' to represent indices into target platform
  // arrays is crude.  Figure out a more appropriate data type, and add
  // checks to catch any range errors while using it.
  //
  // TODO: Use different representations for the two cases, in part so I
  // can expand the array case to express a range.
  //
  std::vector<int> m_indices;

public:      // methods
  // Make an empty path.
  SubobjectAccessPath()
    : m_indices()
  {}

  ~SubobjectAccessPath()
  {}

  // Number of elements in the sequence.
  //
  // The type is not 'size_t' because I want to use 'int' indices, and
  // not get GCC warnings about signedness mismatches.
  int size() const
    { return (int)m_indices.size(); }

  // True if the sequence is empty.
  bool empty() const
    { return m_indices.empty(); }

  // Return the first index.  The sequence must not be empty.
  int frontIndex() const
    { xassert(!empty()); return m_indices.front(); }

  // Return the array index at the given position.
  int arrayIndexAt(int index) const;

  // Return the field index at the given position.
  int fieldIndexAt(int index) const;

  // Treat '[pathIndex:]' as a path navigating within 'type'.  That is,
  // the sequence of elements starting with the one with index
  // 'pathIndex', then the one after it, and so on to the end, is the
  // access path, and we ignore elements with indices less than
  // 'pathIndex'.
  //
  // Modify the path ('this' object, but only elements at 'pathIndex'
  // and greater) so that it points to the next element, i.e., the
  // element that would be initialized after the one named by
  // '[pathIndex:]' within a list of initializers.
  //
  // As a special case, if '[pathIndex:]' is empty, set it to a path to
  // the first element in 'type'.
  //
  // Remove '[pathIndex:]' (truncating 'this' to a length of
  // 'pathIndex') to indicate there are no more elements.  This also
  // applies to the case of '[pathIndex:]' being empty, and means that
  // 'type' contains no elements.
  //
  // On error, add an error to 'env' and return false.
  //
  bool stepForward(
    Env &env,
    int pathIndex,
    Type const *type);

  // Given a path '[pathIndex:]' that navigates to aggregate 'type'
  // within some unspecified whole type, append a first element
  // designator to 'this' and return the resulting type.
  //
  // On error, add an error to 'env' and return NULL.
  //
  Type const *stepIntoAggregate(
    Env &env,
    Type const *type);

  // Navigate from 'type' to a subobject type by following
  // '[pathIndex:]'.
  Type const *navigateToType(
    Env &env,
    int pathIndex,
    Type const *type) const;

  // Extend the current path by array 'index'.
  void pushArrayIndex(int index);

  // Extend the current path by a field named by 'index'.
  void pushFieldIndex(int index);

  // Remove the final index.
  void pop()
    { xassert(!empty()); m_indices.pop_back(); }

  // Set the path to be empty.
  void clear()
    { m_indices.clear(); }

  // Yield a string that describes this access path.  Currently, this is
  // only used within debug trace messages, but it might get used in
  // syntax error messages at some point.
  std::string toString() const;
};


#endif // ELSA_SUBOBJECT_ACCESS_PATH_H
