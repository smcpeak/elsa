// mangle.h
// name mangling

// For now, this is just a verson of Type::toString that does
// not print parameter names.  It's a hack.

// Eventually, it should:
//   - mangle names compactly
//   - demangle names back into types
//   - support more than one algorithm (gcc's, for example) (?)


#ifndef MANGLE_H
#define MANGLE_H

// elsa
#include "cc-type-fwd.h"     // Type, AtomicType
#include "template-fwd.h"    // TemplateInfo, STemplateArgument
#include "variable-fwd.h"    // Variable

// smbase
#include "objlist.h"         // ObjList
#include "str.h"             // string, stringBuilder


// main entry point
string mangle(Type const *t);


// helpers
string mangleAtomic(AtomicType const *t);
string leftMangle(Type const *t, bool innerParen = true);
string rightMangle(Type const *t, bool innerParen = true);
string mangleVariable(Variable const *v);
string mangleTemplateParams(TemplateInfo const *tp);


#endif // MANGLE_H
