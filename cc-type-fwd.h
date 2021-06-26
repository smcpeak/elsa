// cc-type-fwd.h
// Forwards for cc-type.h.

#ifndef ELSA_CC_TYPE_FWD_H
#define ELSA_CC_TYPE_FWD_H

class TypeVisitor;

class AtomicType;
class SimpleType;
class NamedAtomicType;
class BaseClass;
class BaseClassSubobj;
class CompoundType;
class EnumType;

class TypePred;

class BaseType;
class Type;
class CVAtomicType;
class PointerType;
class ReferenceType;
class FunctionType;
class ArrayType;
class PointerToMemberType;
class TypedefType;

class TypeFactory;
class BasicTypeFactory;

class XReprSize;

// TODO: Move into new template-fwd.h.
class TemplateInfo;
class STemplateArgument;

#endif // ELSA_CC_TYPE_FWD_H
