// elab-activities.h
// ElabActivities enumeration.

#ifndef ELSA_ELAB_ACTIVITIES_H
#define ELSA_ELAB_ACTIVITIES_H

#include "sm-macros.h"    // ENUM_BITWISE_OPS


// This enumeration lists all the activities performed by the elaboration
// visitor.  Each can be individually turned on or off, though all are
// on by default.
enum ElabActivities {
  // Do not perform any elaboration.
  EA_NONE                              = 0x0000,

  // This replaces return-by-value for class-valued objects with
  // call-by-reference:
  //   - At every call site that would return by value, a temporary
  //     is created, and passed to the function as an additional
  //     argument.  Here, "passed" means the E_funCall or E_constructor
  //     has a 'retObj' pointer to the expression denoting the temporary.
  //   - In the callee, a special "<retVar>" variable is created; it is
  //     implicit that it gets its value from the passed temporary.  All
  //     S_returns are then rewritten to return nothing, but instead
  //     construct the <retVar> in their 'ctorStatement'.
  EA_ELIM_RETURN_BY_VALUE              = 0x0001,

  // At the end of a destructor, create a 'dtorStatement' which is the
  // sequence of dtors of superclasses and members.
  EA_MEMBER_DTOR                       = 0x0002,

  // At member initializers (MemberInits), create a 'ctorStatement'
  // which explicates the construction of the superclass subobject or
  // class member as an E_constructor call.
  EA_MEMBER_CTOR                       = 0x0004,

  // In constructors, for any superclasses or members that are not
  // explicitly initialized, insert MemberInits that invoke the
  // default (no-arg) constructor for the superclass or member.
  EA_IMPLICIT_MEMBER_CTORS             = 0x0008,

  // Add inline definitions for the compiler-supplied declarations of
  // certain member functions: default ctor, copy ctor, assignment op,
  // and dtor.
  EA_IMPLICIT_MEMBER_DEFN              = 0x0010,

  // For each class-valued local or global variable, annotate its
  // declarator with a 'ctorStatement' and 'dtorStatement' that
  // construct and destruct the member, respectively.
  EA_VARIABLE_DECL_CDTOR               = 0x0020,

  // At throw and catch sites, add statements to construct, copy, and
  // destroy the global exception object that communicates thrown
  // object values from throw to catch.
  EA_GLOBAL_EXCEPTION                  = 0x0040,

  // Eliminate pass-by-value for class-valued objects: at the call site,
  // create a temporary, and pass that.  Then, the callee can treat all
  // parameters (for class-valued params) as if they were pass by reference.
  EA_ELIM_PASS_BY_VALUE                = 0x0080,

  // Translate 'new' into allocation+construction, placing the
  // translation in the 'ctorStatement' of E_new.
  EA_TRANSLATE_NEW                     = 0x0100,

  // Translate 'delete' into destruction+deallocation, placing the
  // translation in the 'dtorStatement' of E_delete.
  EA_TRANSLATE_DELETE                  = 0x0200,

  // When certain AST nodes' semantics have been entirely captured by
  // the elaborated AST, remove those children too, leaving NULL
  // pointers where normally there cannot be NULL pointers.  This is
  // partly a memory optimization and partly a mechanism to ensure that
  // the elaborated AST is what is consumed by later analysis.
  EA_REMOVE_DEFUNCT_CHILDREN           = 0x0400,

  // all flags above
  EA_ALL                               = 0x07FF,


  // Elaboration activities for C.
  EA_C_ACTIVITIES = (
    EA_ELIM_RETURN_BY_VALUE |
    EA_ELIM_PASS_BY_VALUE |
    EA_REMOVE_DEFUNCT_CHILDREN
  ),


  // Note that a number of the above activities create temporary
  // objects.  To support their deletion at the proper time,
  // cc-elaborate.ast adds FullExpressionAnnot objects to some AST
  // nodes, and elaboration registers the temporaries with them
  // accordingly.  An analysis should pay attention to the
  // FullExpressionAnnot objects so it can properly track temporary
  // object lifetimes.
};

ENUM_BITWISE_OPS(ElabActivities, EA_ALL)


#endif // ELSA_ELAB_ACTIVITIES_H
