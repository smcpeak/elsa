// subobject-access-path.cc
// Code for subobject-access-path.h.

#include "subobject-access-path.h"     // this module

// elsa
#include "cc-env.h"                    // Env
#include "cc-type.h"                   // Type

// smbase
#include "str.h"                       // stringb
#include "vector-utils.h"              // toString(std::vector<T>)


int SubobjectAccessPath::arrayIndexAt(int index) const
{
  xassert(0 <= index && index < size());
  return m_indices.at(index);
}


int SubobjectAccessPath::fieldIndexAt(int index) const
{
  xassert(0 <= index && index < size());
  return m_indices.at(index);
}


bool SubobjectAccessPath::stepForward(
  Env &env,
  int pathIndex,
  Type const *type)
{
  // In this function, we operate only on the portion of 'path' starting
  // at 'pathIndex'.  The part to the left of 'pathIndex' must all
  // exist.
  xassert(pathIndex <= size());

  if (size() == pathIndex) {
    // Return a path to the first element.
    if (ArrayType const *at = type->ifArrayTypeC()) {
      if (at->hasSize() && at->getSize() == 0) {
        // No elements, leave '[pathIndex:]' empty.
      }
      else {
        pushArrayIndex(0);
      }
    }

    else if (CompoundType const *ct = type->ifCompoundTypeC()) {
      if (ct->dataMembers.isEmpty()) {
        // No elements, leave '[pathIndex:]' empty.
      }
      else {
        pushFieldIndex(0);
      }
    }

    else {
      // We only step forward with an empty path right after starting to
      // process a brace-enclosed initializer list, so this is a
      // reasonable place to report this error.
      env.error(stringb(
        "Cannot use initializer with braces to initialize type '" <<
        type->toString() << "'."));
      return false;
    }
  }

  else /* '[pathIndex:]' is not empty */ {
    if (ArrayType const *at = type->ifArrayTypeC()) {
      int arrayIndex = m_indices[pathIndex];
      xassert(0 <= arrayIndex);
      xassert(!at->hasSize() || arrayIndex < at->getSize());

      if (size() > pathIndex+1) {
        // Step the remainder of the path.
        if (!stepForward(env, pathIndex+1, at->eltType)) {
          return false;
        }
      }

      if (size() == pathIndex+1) {
        // We're finished with element 'arrayIndex'.
        if (at->hasSize() && arrayIndex+1 == at->getSize()) {
          // Reached the end of this array.  Remove the final index.
          pop();
        }
        else {
          // Advance to 'arrayIndex+1'.
          m_indices[pathIndex] = arrayIndex+1;
        }
      }
    }

    else if (CompoundType const *ct = type->ifCompoundTypeC()) {
      int fieldIndex = m_indices[pathIndex];
      xassert(0 <= fieldIndex && fieldIndex < ct->dataMembers.count());

      if (size() > pathIndex+1) {
        // Step the remainder of the path.
        Type *fieldType = ct->dataMembers.nthC(fieldIndex)->type;
        if (!stepForward(env, pathIndex+1, fieldType)) {
          return false;
        }
      }

      if (size() == pathIndex+1) {
        // We're finished with the field at 'fieldIndex'.
        if (ct->keyword == CompoundType::K_UNION) {
          // Since this is a union, we only ever initialize one field,
          // so we're done.
          pop();
        }
        else if (fieldIndex+1 == ct->dataMembers.count()) {
          // Reached the end of this structure.  Remove the final index.
          pop();
        }
        else {
          // Advance to the next field.
          m_indices[pathIndex] = fieldIndex+1;
        }
      }
    }

    else {
      // It should not be possible to get here because we have a path
      // index, and we only add a path index when there is an array or
      // compound type to traverse.
      xfailure("SubobjectAccessPath element corresponds to non-array, non-compound.");
    }
  }

  return true;
}


Type const *SubobjectAccessPath::stepIntoAggregate(
  Env &env,
  Type const *type)
{
  if (ArrayType const *at = type->ifArrayTypeC()) {
    if (at->hasSize() && at->getSize() == 0) {
      // We know braces were not used because we only call this function
      // when we see a non-brace-enclosed initializer expression.  GCC
      // and Clang both enforce this rule, so I will to.
      //
      // Note that zero-length arrays are a GCC extension.
      env.error("Initialization of zero-length array requires explicit braces.");
      return NULL;
    }

    pushArrayIndex(0);
    return at->eltType;
  }

  else {
    CompoundType const *ct = type->asCompoundTypeC();
    xassert(ct->isAggregate());

    if (ct->dataMembers.isEmpty()) {
      env.error(stringb(
        "Initialization of empty " << ::toString(ct->keyword) <<
        " requires explicit braces."));
      return NULL;
    }

    pushFieldIndex(0);
    return ct->dataMembers.firstC()->type;
  }
}


Type const *SubobjectAccessPath::navigateToType(
  Env &env,
  int pathIndex,
  Type const *type) const
{
  xassert(size() >= pathIndex);

  if (pathIndex == size()) {
    return type;
  }

  if (ArrayType const *at = type->ifArrayTypeC()) {
    int arrayIndex = m_indices[pathIndex];
    xassert(0 <= arrayIndex);
    xassert(!at->hasSize() || arrayIndex < at->getSize());
    return navigateToType(env, pathIndex+1, at->eltType);
  }

  else if (CompoundType const *ct = type->ifCompoundTypeC()) {
    int fieldIndex = m_indices[pathIndex];
    xassert(0 <= fieldIndex && fieldIndex < ct->dataMembers.count());
    Type const *fieldType = ct->dataMembers.nthC(fieldIndex)->type;
    return navigateToType(env, pathIndex+1, fieldType);
  }

  else {
    xfailure("Attempt to navigate by path in non-array, non-compound.");
  }
}


void SubobjectAccessPath::pushArrayIndex(int index)
{
  m_indices.push_back(index);
}


void SubobjectAccessPath::pushFieldIndex(int index)
{
  m_indices.push_back(index);
}


std::string SubobjectAccessPath::toString() const
{
  return ::toString(m_indices);
}


// EOF
