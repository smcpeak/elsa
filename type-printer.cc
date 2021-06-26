// type-printer.cc
// Code for type-printer.h.

#include "type-printer.h"              // this module

#include "cc-print.h"                  // PrintEnv


// **************** class TypePrinter

TypeLike const *TypePrinter::getVariableTypeLike(Variable const *var)
{
  return var->type;
}

TypeLike const *TypePrinter::getFunctionTypeLike(Function const *func)
{
  return func->funcType;
}

TypeLike const *TypePrinter::getE_constructorTypeLike(E_constructor const *c)
{
  return c->type;
}

// **************** class CTypePrinter

string CTypePrinter::printType(TypeLike const *type, char const *name)
{
  // see the note at the interface TypePrinter::print()
  Type const *type0 = static_cast<Type const *>(type);
  return print(type0, name);
}


// **** AtomicType

string CTypePrinter::print(AtomicType const *atomic)
{
  // roll our own virtual dispatch
  switch(atomic->getTag()) {
  default: xfailure("bad tag");
  case AtomicType::T_SIMPLE:              return print(atomic->asSimpleTypeC());
  case AtomicType::T_COMPOUND:            return print(atomic->asCompoundTypeC());
  case AtomicType::T_ENUM:                return print(atomic->asEnumTypeC());
  case AtomicType::T_TYPEVAR:             return print(atomic->asTypeVariableC());
  case AtomicType::T_PSEUDOINSTANTIATION: return print(atomic->asPseudoInstantiationC());
  case AtomicType::T_DEPENDENTQTYPE:      return print(atomic->asDependentQTypeC());
  }
}

string CTypePrinter::print(SimpleType const *simpleType)
{
  if (simpleType->type == ST_BOOL && !m_lang.isCplusplus) {
    // In C mode, use its keyword instead of the C++ spelling.
    return "_Bool";
  }
  else {
    return simpleTypeName(simpleType->type);
  }
}

string CTypePrinter::print(CompoundType const *cpdType)
{
  stringBuilder sb;

  TemplateInfo *tinfo = cpdType->templateInfo();
  bool hasParams = tinfo && tinfo->params.isNotEmpty();
  if (hasParams) {
    sb << tinfo->paramsToCString() << ' ';
  }

  if (!tinfo || hasParams) {
    // only say 'class' if this is like a class definition, or
    // if we're not a template, since template instantiations
    // usually don't include the keyword 'class' (this isn't perfect..
    // I think I need more context)
    sb << CompoundType::keywordName(cpdType->keyword) << ' ';
  }

  sb << (cpdType->instName? cpdType->instName : "/*anonymous*/");

  // template arguments are now in the name
  // 4/22/04: they were removed from the name a long time ago;
  //          but I'm just now uncommenting this code
  // 8/03/04: restored the original purpose of 'instName', so
  //          once again that is name+args, so this code is not needed
  //if (tinfo && tinfo->arguments.isNotEmpty()) {
  //  sb << sargsToString(tinfo->arguments);
  //}

  return sb;
}

string CTypePrinter::print(EnumType const *enumType)
{
  return stringc << "enum " << (enumType->name? enumType->name : "/*anonymous*/");
}

string CTypePrinter::print(TypeVariable const *typeVar)
{
  // use the "typename" syntax instead of "class", to distinguish
  // this from an ordinary class, and because it's syntax which
  // more properly suggests the ability to take on *any* type,
  // not just those of classes
  //
  // but, the 'typename' syntax can only be used in some specialized
  // circumstances.. so I'll suppress it in the general case and add
  // it explicitly when printing the few constructs that allow it
  //
  // 8/09/04: sm: truncated down to just the name, since the extra
  // clutter was annoying and not that helpful
  return stringc //<< "/""*typevar"
//                   << "typedefVar->serialNumber:"
//                   << (typedefVar ? typedefVar->serialNumber : -1)
                 //<< "*/"
                 << typeVar->name;
}

string CTypePrinter::print(PseudoInstantiation const *pseudoInst)
{
  PrintEnv env(*this);

  env << pseudoInst->name;

  env << '<';
  int ct=0;
  FOREACH_OBJLIST(STemplateArgument, pseudoInst->args, iter) {
    if (ct++ > 0) {
      env << ',' << ' ';
    }
    printSTemplateArgument(env, iter.data());
  }
  env << '>';

  return env.getResult();
}

string CTypePrinter::print(DependentQType const *depType)
{
  PrintEnv env(*this);

  env << print(depType->first) << ':' << ':';
  depType->rest->print(env);

  return env.getResult();
}


// **** [Compound]Type

string CTypePrinter::print(Type const *type)
{
  if (type->isCVAtomicType()) {
    // special case a single atomic type, so as to avoid
    // printing an extra space
    CVAtomicType const *cvatomic = type->asCVAtomicTypeC();
    return stringc
      << print(cvatomic->atomic)
      << cvToString(cvatomic->cv);
  }
  else {
    return stringc
      << printLeft(type)
      << printRight(type);
  }
}

string CTypePrinter::print(Type const *type, char const *name)
{
  // print the inner parentheses if the name is omitted
  bool innerParen = (name && name[0])? false : true;

  #if 0    // wrong
  // except, if this type is a pointer, then omit the parens anyway;
  // we only need parens when the type is a function or array and
  // the name is missing
  if (isPointerType()) {
    innerParen = false;
  }
  #endif // 0

  stringBuilder s;
  s << printLeft(type, innerParen);
  s << (name? name : "/*anon*/");
  s << printRight(type, innerParen);

  // get rid of extra space
  while (s.length() >= 1 && s[s.length()-1] == ' ') {
    s.truncate(s.length() - 1);
  }

  return s;
}

string CTypePrinter::printRight(Type const *type, bool innerParen)
{
  // roll our own virtual dispatch
  switch(type->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC:          return printRight(type->asCVAtomicTypeC(), innerParen);
  case Type::T_POINTER:         return printRight(type->asPointerTypeC(), innerParen);
  case Type::T_REFERENCE:       return printRight(type->asReferenceTypeC(), innerParen);
  case Type::T_FUNCTION:        return printRight(type->asFunctionTypeC(), innerParen);
  case Type::T_ARRAY:           return printRight(type->asArrayTypeC(), innerParen);
  case Type::T_POINTERTOMEMBER: return printRight(type->asPointerToMemberTypeC(), innerParen);
  }
}

string CTypePrinter::printLeft(Type const *type, bool innerParen)
{
  // roll our own virtual dispatch
  switch(type->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC:          return printLeft(type->asCVAtomicTypeC(), innerParen);
  case Type::T_POINTER:         return printLeft(type->asPointerTypeC(), innerParen);
  case Type::T_REFERENCE:       return printLeft(type->asReferenceTypeC(), innerParen);
  case Type::T_FUNCTION:        return printLeft(type->asFunctionTypeC(), innerParen);
  case Type::T_ARRAY:           return printLeft(type->asArrayTypeC(), innerParen);
  case Type::T_POINTERTOMEMBER: return printLeft(type->asPointerToMemberTypeC(), innerParen);
  }
}

string CTypePrinter::printLeft(CVAtomicType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  s << print(type->atomic);
  s << cvToString(type->cv);

  // Separate the type specifier from the declarator(s).
  s << ' ';

  return s;
}

string CTypePrinter::printRight(CVAtomicType const *type, bool /*innerParen*/)
{
  return "";
}

string CTypePrinter::printLeft(PointerType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  s << printLeft(type->atType, false /*innerParen*/);
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << '(';
  }
  s << '*';
  if (type->cv) {
    // 1/03/03: added this space so "Foo * const arf" prints right (t0012.cc)
    s << cvToString(type->cv) << ' ';
  }
  return s;
}

string CTypePrinter::printRight(PointerType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << ')';
  }
  s << printRight(type->atType, false /*innerParen*/);
  return s;
}

string CTypePrinter::printLeft(ReferenceType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  s << printLeft(type->atType, false /*innerParen*/);
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << '(';
  }
  s << '&';
  return s;
}

string CTypePrinter::printRight(ReferenceType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << ')';
  }
  s << printRight(type->atType, false /*innerParen*/);
  return s;
}

string CTypePrinter::printLeft(FunctionType const *type, bool innerParen)
{
  stringBuilder sb;

  // FIX: FUNC TEMPLATE LOSS
  // template parameters
//    if (templateInfo) {
//      sb << templateInfo->paramsToCString() << " ";
//    }

  // return type and start of enclosing type's description
  if (type->flags & (/*FF_CONVERSION |*/ FF_CTOR | FF_DTOR)) {
    // don't print the return type, it's implicit

    // 7/18/03: changed so we print ret type for FF_CONVERSION,
    // since otherwise I can't tell what it converts to!
  }
  else {
    sb << printLeft(type->retType);
  }

  // NOTE: we do *not* propagate 'innerParen'!
  if (innerParen) {
    sb << '(';
  }

  return sb;
}

string CTypePrinter::printRight(FunctionType const *type, bool innerParen)
{
  // I split this into two pieces because the Cqual++ concrete
  // syntax puts $tainted into the middle of my rightString,
  // since it's following the placement of 'const' and 'volatile'
  return stringc
    << printRightUpToQualifiers(type, innerParen)
    << printRightQualifiers(type, type->getReceiverCV())
    << printRightAfterQualifiers(type);
}

string CTypePrinter::printRightUpToQualifiers(FunctionType const *type, bool innerParen)
{
  // finish enclosing type
  stringBuilder sb;
  if (innerParen) {
    sb << ')';
  }

  // arguments
  sb << '(';
  int ct=0;
  SFOREACH_OBJLIST(Variable, type->params, iter) {
    ct++;
    if (type->isMethod() && ct==1) {
      // don't actually print the first parameter;
      // the 'm' stands for nonstatic member function
      sb << "/""*m: " << print(iter.data()->type) << " *""/";
      continue;
    }
    if (type->isMethod() && ct==2) {
      // Print a space after the /*m*/ thing if there are parameters
      // that follow it.
      sb << ' ';
    }
    if (ct >= 3 || (!type->isMethod() && ct>=2)) {
      sb << ',' << ' ';
    }
    sb << printAsParameter(iter.data());
  }

  if (type->acceptsVarargs()) {
    if (ct++ > 0) {
      sb << ',' << ' ';
    }
    sb << '.' << '.' << '.';
  }

  sb << ')';

  return sb;
}

string CTypePrinter::printRightQualifiers(FunctionType const *type, CVFlags cv)
{
  if (cv) {
    return stringc << ' ' << ::toString(cv);
  }
  else {
    return "";
  }
}

string CTypePrinter::printRightAfterQualifiers(FunctionType const *type)
{
  stringBuilder sb;

  // exception specs
  if (type->exnSpec) {
    sb << " throw(";
    int ct=0;
    SFOREACH_OBJLIST(Type, type->exnSpec->types, iter) {
      if (ct++ > 0) {
        sb << ',' << ' ';
      }
      sb << print(iter.data());
    }
    sb << ')';
  }

  // hook for verifier syntax
  printExtraRightmostSyntax(type, sb);

  // finish up the return type
  sb << printRight(type->retType);

  return sb;
}

void CTypePrinter::printExtraRightmostSyntax(FunctionType const *type, stringBuilder &)
{}

string CTypePrinter::printLeft(ArrayType const *type, bool /*innerParen*/)
{
  return printLeft(type->eltType);
}

string CTypePrinter::printRight(ArrayType const *type, bool /*innerParen*/)
{
  stringBuilder sb;

  if (type->hasSize()) {
    sb << '[' << type->size << ']';
  }
  else {
    sb << '[' << ']';
  }

  sb << printRight(type->eltType);

  return sb;
}

string CTypePrinter::printLeft(PointerToMemberType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  s << printLeft(type->atType, false /*innerParen*/);
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << '(';
  }
  s << type->inClassNAT->name << ':' << ':' << '*';
  s << cvToString(type->cv);
  return s;
}

string CTypePrinter::printRight(PointerToMemberType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << ')';
  }
  s << printRight(type->atType, false /*innerParen*/);
  return s;
}

string CTypePrinter::printAsParameter(Variable const *var)
{
  stringBuilder sb;
  if (var->type->isTypeVariable()) {
    // type variable's name, then the parameter's name (if any)
    sb << var->type->asTypeVariable()->name;
    if (var->name) {
      sb << ' ' << var->name;
    }
  }
  else {
    sb << print(var->type, var->name);
  }

  if (var->value) {
    sb << renderExpressionAsString(" = ", var->value);
  }
  return sb;
}


// EOF
