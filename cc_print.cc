// cc_print.cc            see license.txt for copyright and terms of use
// code for cc_print.h

// Adapted from cc_tcheck.cc by Daniel Wilkerson dsw@cs.berkeley.edu

#include "cc_print.h"           // this module

#include "cc_lang.h"            // CCLang

#include "trace.h"              // trace
#include "strutil.h"            // string utilities

#include <ctype.h>              // isalnum
#include <stdlib.h>             // getenv


// set this environment variable to see the twalk_layer debugging
// output
OStreamOutStream treeWalkOut0(cout);
TreeWalkOutStream treeWalkOut(treeWalkOut0, getenv("TWALK_VERBOSE"));

// This is a dummy global so that this file will compile both in
// default mode and in qualifiers mode.
class dummyType;                // This does nothing.
dummyType *ql;
string toString(class dummyType*) {return "";}

// **** class CodeOutStream

CodeOutStream::~CodeOutStream()
{
  if (bufferedNewlines) {
    cout << "**************** ERROR.  "
         << "You called my destructor before making sure all the buffered newlines\n"
         << "were flushed (by, say, calling finish())\n";
  }
}

// // write N spaces to OUT.
// static inline
// void writeSpaces(OutStream &out, size_t n)
// {
//   static char const spaces[] =
//     "                                                  "
//     "                                                  "
//     "                                                  "
//     "                                                  ";

//   static size_t const max_spaces = sizeof spaces - 1;

//   // If we're printing more than this many spaces it's pretty useless anyway,
//   // since it's only for human viewing pleasure!
//   while (n > max_spaces) {
//     out.write(spaces, max_spaces);
//     n -= max_spaces;
//   }
//   out.write(spaces, n);
// }

// TODO: add write(char*,int len) methods to OutStream et al so we can do
// 'printIndentation' efficiently
void CodeOutStream::printIndentation(int n) {
  // writeSpaces(out, n);
  for (int i=0; i<n; ++i) {
    out << ' ' << ' ';
  }
}

void CodeOutStream::finish()
{
  // Empty the buffered newlines.
  while (bufferedNewlines > 0) {
    out << '\n';
    bufferedNewlines--;
  }

  flush();
}

CodeOutStream & CodeOutStream::operator << (ostream& (*manipfunc)(ostream& outs))
{
  // sm: just assume it's "endl"; the only better thing I could
  // imagine doing is pointer comparisons with some other well-known
  // omanips, since we certainly can't execute it...
  this->operator<<("\n");

  out.flush();
  return *this;
}

CodeOutStream & CodeOutStream::operator << (char const *message)
{
  for (; *message; message++) {
    if (*message != '\n') {
      // Non-newline: Empty the buffered newlines.
      if (bufferedNewlines > 0) {
        while (bufferedNewlines > 0) {
          out << '\n';
          bufferedNewlines--;
        }

        // We are about to print a non-newline, so indent.
        printIndentation(depth);
      }

      out << *message;
    }
    else {
      // Newline: accumulate in buffer.
      bufferedNewlines++;
    }
  }

  return *this;
}

// **** class PairDelim

PairDelim::PairDelim(PrintEnv &env, rostring message, rostring open, char const *close0)
    : close(close0), out(*env.out)
{
  out << message;
  out << open;
  if (strchr(toCStr(open), '{')) out.down();
}

PairDelim::PairDelim(PrintEnv &env, rostring message)
  : close(""), out(*env.out)
{
  out << message;
}

PairDelim::~PairDelim() {
  if (strchr(close, '}')) out.up();
  out << close;
}

// **** class TreeWalkOutStream

void TreeWalkOutStream::indent() {
  out << endl;
  out.flush();
  for(int i=0; i<depth; ++i) out << ' ';
  out.flush();
  out << ":::::";
  out.flush();
}

TreeWalkOutStream & TreeWalkOutStream::operator << (ostream& (*manipfunc)(ostream& outs))
{
  if (on) out << manipfunc;
  return *this;
}

// **** class TreeWalkDebug

TreeWalkDebug::TreeWalkDebug(char const *message, TreeWalkOutStream &out)
  : out(out)
{
  out << message << endl;
  out.flush();
  out.down();
}

TreeWalkDebug::~TreeWalkDebug()
{
  out.up();
}


// **************** class TypePrinter

TypeLike const *TypePrinter::getTypeLike(Variable const *var)
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

bool CTypePrinter::enabled = true;

void CTypePrinter::print(OutStream &out, TypeLike const *type, char const *name)
{
  // temporarily suspend the Type::toCString, Variable::toCString(),
  // etc. methods
  Restorer<bool> res0(global_mayUseTypeAndVarToCString, false);
  xassert(enabled);
  // see the note at the interface TypePrinter::print()
  Type const *type0 = static_cast<Type const *>(type);
  out << print(type0, name);
  // old way
//    out << type->toCString(name);
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
  stringBuilder sb0;
  StringBuilderOutStream out0(sb0);
  CodeOutStream codeOut(out0);
  PrintEnv env(*this, &codeOut); // Yuck!
  // FIX: what about the env.loc?

  codeOut << pseudoInst->name;

  // NOTE: This was inlined from sargsToString; it would read as
  // follows:
//    codeOut << sargsToString(pseudoInst->args);
  codeOut << '<';
  int ct=0;
  FOREACH_OBJLIST(STemplateArgument, pseudoInst->args, iter) {
    if (ct++ > 0) {
      codeOut << ',' << ' ';
    }
    printSTemplateArgument(env, iter.data());
  }
  codeOut << '>';

  codeOut.finish();
  return sb0;
}

string CTypePrinter::print(DependentQType const *depType)
{
  stringBuilder sb0;
  StringBuilderOutStream out0(sb0);
  CodeOutStream codeOut(out0);
  PrintEnv env(*this, &codeOut); // Yuck!
  // FIX: what about the env.loc?

  codeOut << print(depType->first) << ':' << ':';
  depType->rest->print(env);

  codeOut.finish();
  return sb0;
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


// **************** class PrintEnv

void PrintEnv::ptype(Type const *type, char const *name)
{
  typePrinter.print(*out, type, name);
}


// ****************

// WARNING: if you are inclined to inline this function back into its
// one call site, be sure you are careful that you do not change the
// semantics of Restorer below: the site of its destructor call is
// very important to the semantics of the printing, as it changes
// which OutStream is being printed to.
string printDeclaration_makeName
  (PrintEnv &env,
   Type const *type,
   PQName const * /*nullable*/ pqname,
   Variable *var,
   StringRef finalName)
{
  stringBuilder sb0;
  StringBuilderOutStream out0(sb0);
  CodeOutStream codeOut0(out0);
  // NOTE: temporarily change the OutStream in the PrintEnv; see the
  // WARNING above.
  Restorer<CodeOutStream*> tempSubstCodeOutInEnv(env.out, &codeOut0);

  if (pqname) {
    codeOut0 << pqname->qualifierString();
  }
  codeOut0 << finalName;
  if (type->isFunctionType() &&
      var->templateInfo() &&
      var->templateInfo()->isCompleteSpecOrInstantiation()) {
    // print the spec/inst args after the function name;
    //
    // NOTE: This was inlined from sargsToString; it used to read as follows:
    //          codeOut0 << sargsToString(var->templateInfo()->arguments);
    codeOut0 << '<';
    int ct=0;
    FOREACH_OBJLIST_NC(STemplateArgument, var->templateInfo()->arguments, iter) {
      if (ct++ > 0) {
        codeOut0 << ',' << ' ';
      }
      printSTemplateArgument(env, iter.data());
    }
    codeOut0 << '>';
  }

  codeOut0 << var->namePrintSuffix();    // hook for verifier
  codeOut0.finish();
  return sb0;
}

// hooks for Oink
//
// sm: My intuition is that these hooks and ought to be parallel
// (i.e., just one hook function), but they are not, either in type
// signature or in runtime behavior, so I suspect there is a bug here.
TypeLike const *getDeclarationRetTypeLike(TypeLike const *type);
Type const *getDeclarationTypeLike(TypeLike const *type);

// Elsa implementations
TypeLike const *getDeclarationRetTypeLike(TypeLike const *type)
{
  return type->asFunctionTypeC()->retType;
}

Type const *getDeclarationTypeLike(TypeLike const *type)
{
  return type;
}

// function for printing declarations (without the final semicolon);
// handles a variety of declarations such as:
//   int x
//   int x()
//   C()                    // ctor inside class C
//   operator delete[]()
//   char operator()        // conversion operator to 'char'
void printDeclaration
  (PrintEnv &env,

  // declflags present in source; not same as 'var->flags' because
  // the latter is a mixture of flags present in different
  // declarations
  DeclFlags dflags,

  // type of the variable; not same as 'var->type' because the latter
  // can come from a prototype and hence have different parameter
  // names
  TypeLike const *type,

  // original name in the source; for now this is redundant with
  // 'var->name', but we plan to print the qualifiers by looking
  // at 'pqname'
  PQName const * /*nullable*/ pqname,

  // associated variable; in the final design, this will only be
  // used to look up the variable's scope
  Variable *var)
{
  TreeWalkDebug treeDebug("printDeclaration");

  // mask off flags used for internal purposes, so all that's
  // left is the flags that were present in the source
  dflags = (DeclFlags)(dflags & DF_SOURCEFLAGS);
  if (dflags) {
    env << toString(dflags) << " ";
  }

  // the string name after all of the qualifiers; if this is
  // a special function, we're getting the encoded version
  StringRef finalName = pqname? pqname->getName() : NULL;

  if (finalName && streq(finalName, "conversion-operator")) {
    // special syntax for conversion operators; first the keyword
    env << "operator ";

    // then the return type and the function designator
    TypeLike const *retThing = getDeclarationRetTypeLike(type);
    env.ptype(retThing);

    env << " ()";
  }

  else if (finalName && streq(finalName, "constructor-special")) {
    // extract the class name, which can be found by looking up
    // the name of the scope which contains the associated variable
    env.ptype(type, var->scope->curCompound->name);
  }

  else {
    if (finalName) {
      Type const *type0 = getDeclarationTypeLike(type);
      string name = printDeclaration_makeName(env, type0, pqname, var, finalName);
      env.ptype(type, name.c_str());
    } else {
      env.ptype(type, finalName);
    }
  }
}


// more specialized version of the previous function
void printVar(PrintEnv &env, Variable *var, PQName const * /*nullable*/ pqname)
{
  TreeWalkDebug treeDebug("printVar");

  TypeLike const *type0 = env.getTypeLike(var);

  printDeclaration(env, var->flags,
                   type0, pqname, var);
}


// this is a prototype for a function down near E_funCall::iprint
void printArgExprList(PrintEnv &env, FakeList<ArgExpression> *list);


// ------------------- TranslationUnit --------------------
void TranslationUnit::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TranslationUnit");
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->print(env);
  }
}

// --------------------- TopForm ---------------------
void TF_decl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_decl");
  env.loc = loc;
  decl->print(env);
}

void TF_func::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_func");
  env << "\n";
  env.loc = loc;
  f->print(env);
}

void TF_template::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_template");
  env.loc = loc;
  td->print(env);
}

void TF_explicitInst::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_explicitInst");
  env.loc = loc;
  env << "template ";
  d->print(env);
}

void TF_linkage::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_linkage");
  env.loc = loc;
  env << "extern " << linkageType;
  PairDelim pair(env, "", " {\n", "}\n");
  forms->print(env);
}

void TF_one_linkage::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_one_linkage");
  env.loc = loc;
  env << "extern " << linkageType << " ";
  form->print(env);
}

void TF_asm::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_asm");
  env.loc = loc;
  env << "asm(" << text << ");\n";
}

void TF_namespaceDefn::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_namespaceDefn");
  env.loc = loc;
  env << "namespace " << (name? name : "/*anon*/") << " {\n";
  FOREACH_ASTLIST_NC(TopForm, forms, iter) {
    iter.data()->print(env);
  }
  env << "} /""* namespace " << (name? name : "(anon)") << " */\n";
}

void TF_namespaceDecl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_namespaceDecl");
  env.loc = loc;
  decl->print(env);
}


// --------------------- Function -----------------
static bool ideclaratorIsDestructor(IDeclarator const *id)
{
  // TODO: Ugly, just like the next function.
  if (D_func const *df = id->ifD_funcC()) {
    if (D_name const *dn = df->base->ifD_nameC()) {
      if (PQ_variable const *pqv = dn->name->ifPQ_variable()) {
        if (pqv->var->name[0] == '~') {
          return true;
        }
      }
    }
  }

  return false;
}

// True if 'c' could be part of an identifier.
static bool isIdentifierChar(int c)
{
  return isalnum(c) || c == '_';
}

// True if the declarator needs a space after the specifier 'spec'.
static bool ideclaratorWantsSpace(TypeSpecifier const *spec,
                                  IDeclarator const *id)
{
  // TODO: This is extremely ugly.
  if (TS_type const *tst = spec->ifTS_typeC()) {
    string specString = tst->type->toString();
    if (!specString.empty()) {
      char lastChar = specString[specString.length()-1];
      if (!isIdentifierChar((unsigned char)lastChar)) {
        // The "specifier" will print a string that does not end in an
        // identifier character, so no space is needed.
        return false;
      }
    }
  }

  if (TS_simple const *tss = spec->ifTS_simpleC()) {
    if (tss->id == ST_CDTOR) {
      // No space after the emptiness representing the "return type"
      // of constructors.
      return false;
    }
  }

  if (D_name const *dn = id->ifD_nameC()) {
    if (dn->name == NULL) {
      return false;
    }
  }

  // TODO: This is ugly and it breaks the idea that cc_elaborate.ast is
  // not depended upon by the core.  Once I confirm this is what I want,
  // I need to devise a better method of making this query.
  if (D_func const *df = id->ifD_funcC()) {
    if (D_name const *dn = df->base->ifD_nameC()) {
      if (PQ_variable const *pqv = dn->name->ifPQ_variable()) {
        if (streq(pqv->var->name, "constructor-special")) {
          return false;
        }
      }
    }
  }

  return true;
}

// Print a type specifier followed by a declarator.
static void printTypeSpecifierAndDeclarator(
  PrintEnv &env, TypeSpecifier *spec, Declarator *declarator)
{
  if (!ideclaratorIsDestructor(declarator->decl)) {
    spec->detailPrint(env);
    if (ideclaratorWantsSpace(spec, declarator->decl)) {
      env << " ";
    }
  }
  declarator->detailPrint(env);
}

void Function::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Function");

  // TODO: Remove this stuff.
  TypeLike const *type0 = env.typePrinter.getFunctionTypeLike(this);
  Restorer<bool> res0(CTypePrinter::enabled, type0 == funcType);

  DeclFlags sourceFlags = dflags & DF_SOURCEFLAGS;
  if (sourceFlags) {
    env << toString(sourceFlags) << " ";
  }
  printTypeSpecifierAndDeclarator(env, retspec, nameAndParams);

  if (instButNotTchecked()) {
    // this is an unchecked instantiation
    env << "; // not instantiated\n";
    return;
  }

  env << "\n";
  if (handlers) {
    env << "try";
    if (inits) {
      // Newline before member inits.
      env << "\n";
    }
    else {
      // Space before opening brace.
      env << " ";
    }
  }

  if (inits) {
    env << "  : ";
    bool first_time = true;
    FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
      if (first_time) first_time = false;
      else env << ",\n    ";
      // NOTE: eventually will be able to figure out if we are
      // initializing a base class or a member variable.  There will
      // be a field added to class MemberInit that will say.
      PairDelim pair(env, iter->name->toString(), "(", ")");
      printArgExprList(env, iter->args);
    }
    env << "\n";
  }

  if (body->stmts.isEmpty()) {
    // more concise
    env << "{}\n";
  }
  else {
    body->print(env);
  }

  if (handlers) {
    FAKELIST_FOREACH_NC(Handler, handlers, iter) {
      iter->print(env);
    }
    env << "\n";
  }
}


// MemberInit

// -------------------- Declaration -------------------
void Declaration::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Declaration");

  DeclFlags sourceFlags = dflags & DF_SOURCEFLAGS;
  if (sourceFlags) {
    env << toString(sourceFlags) << " ";
  }

  spec->detailPrint(env);

  FAKELIST_FOREACH_NC(Declarator, decllist, declarator) {
    if (declarator != fl_first(decllist)) {
      env << ", ";
    }
    else if (ideclaratorWantsSpace(spec, declarator->decl)) {
      env << " ";
    }
    declarator->detailPrint(env);
  }
  env << ";\n";
}


// -------------------- ASTTypeId -------------------
void printInitializerOpt(PrintEnv &env, Initializer /*nullable*/ *init)
{
  if (init) {
    IN_ctor *ctor = dynamic_cast<IN_ctor*>(init);
    if (ctor) {
      // sm: don't print "()" as an IN_ctor initializer (cppstd 8.5 para 8)
      if (fl_isEmpty(ctor->args)) {
        // Don't print anything.
      }
      else {
        // dsw:Constructor arguments.
        PairDelim pair(env, "", "(", ")");
        ctor->print(env);       // NOTE: You can NOT factor this line out of the if!
      }
    } else {
      env << " = ";
      init->print(env);         // Don't pull this out!
    }
  }
}

void ASTTypeId::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("ASTTypeId");

  printTypeSpecifierAndDeclarator(env, spec, decl);
}


// ---------------------- PQName -------------------
void printTemplateArgumentList
  (PrintEnv &env, /*fakelist*/TemplateArgument *args)
{
  int ct=0;
  while (args) {
    if (!args->isTA_templateUsed()) {
      if (ct++ > 0) {
        env << ',' << ' ';
      }
      args->print(env);
    }

    args = args->next;
  }
}

void PQ_qualifier::print(PrintEnv &env)
{
  if (templateUsed()) {
    env << "template ";
  }

  if (qualifier) {
    env << qualifier;
    if (templArgs/*isNotEmpty*/) {
      env << '<';
      printTemplateArgumentList(env, templArgs);
      env << '>';
    }
  }
  env << ':' << ':';

  rest->print(env);
}

void PQ_name::print(PrintEnv &env)
{
  // TODO: Put this string into the string table.
  if (streq(name, "constructor-special")) {
    // Do not print the name.
  }
  else {
    env << name;
  }
}

void PQ_operator::print(PrintEnv &env)
{
  o->print(env);
}

void PQ_template::print(PrintEnv &env)
{
  if (templateUsed()) {
    env << "template ";
  }

  env << name << '<';
  printTemplateArgumentList(env, templArgs);
  env << '>';
}


// --------------------- TypeSpecifier --------------
void TS_name::print(PrintEnv &env)
{
  xassert(0);                   // I'll bet this is never called.
//    TreeWalkDebug treeDebug("TS_name");
//    env << toString(ql);          // see string toString(class dummyType*) above
//    name->print(env);
}

void TS_simple::print(PrintEnv &env)
{
  xassert(0);                   // I'll bet this is never called.
//    TreeWalkDebug treeDebug("TS_simple");
//    env << toString(ql);          // see string toString(class dummyType*) above
}

void TS_elaborated::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TS_elaborated");
  env.loc = loc;
  env << toString(ql);          // see string toString(class dummyType*) above
  env << toString(keyword) << ' ';
  name->print(env);
}

void TS_classSpec::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TS_classSpec");
  env << toString(ql);          // see string toString(class dummyType*) above
  env << toString(cv);
  env << toString(keyword) << ' ';
  if (name) env << name->toString();
  bool first_time = true;
  FAKELIST_FOREACH_NC(BaseClassSpec, bases, iter) {
    if (first_time) {
      env << ' ' << ':' << ' ';
      first_time = false;
    }
    else env << ',' << ' ';
    iter->print(env);
  }
  PairDelim pair(env, " ", "{\n", "}");
  FOREACH_ASTLIST_NC(Member, members->list, iter2) {
    iter2.data()->print(env);
  }
}

void TS_enumSpec::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TS_enumSpec");
  env << toString(ql);          // see string toString(class dummyType*) above
  env << toString(cv);
  env << "enum ";
  if (name) env << name;
  PairDelim pair(env, "", "{\n", "}");
  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->print(env);
    env << "\n";
  }
}


void TypeSpecifier::detailPrint(PrintEnv &env)
{
  idetailPrint(env);
  if (cv) {
    env << " " << toString(cv);
  }
}


void TS_name::idetailPrint(PrintEnv &env)
{
  if (typenameUsed) {
    env << "typename ";
  }
  name->print(env);
}


void TS_simple::idetailPrint(PrintEnv &env)
{
  if (id != ST_CDTOR) {
    env << toString(id);
  }
}


void TS_elaborated::idetailPrint(PrintEnv &env)
{
  env << toString(keyword) << " ";
  name->print(env);
}


void TS_classSpec::idetailPrint(PrintEnv &env)
{
  // The existing 'print' is probably adequate.
  this->print(env);
}


void TS_enumSpec::idetailPrint(PrintEnv &env)
{
  this->print(env);
}


// ---------------------- BaseClassSpec ----------------------
void BaseClassSpec::print(PrintEnv &env) {
  TreeWalkDebug treeDebug("BaseClassSpec");
  if (isVirtual) env << "virtual ";
  if (access!=AK_UNSPECIFIED) env << toString(access) << " ";
  env << name->toString();
}


// MemberList

// ---------------------- Member ----------------------
void MR_decl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_decl");
  d->print(env);
}

void MR_func::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_func");
  f->print(env);
}

void MR_access::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_access");

  // Print access specifiers un-intended.
  env.out->up();

  env << toString(k) << ":\n";

  env.out->down();
}

void MR_usingDecl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_usingDecl");
  decl->print(env);
}

void MR_template::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_template");
  d->print(env);
}


// -------------------- Enumerator --------------------
void Enumerator::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Enumerator");
  env << name;
  if (expr) {
    env << '=';
    expr->print(env, OPREC_ASSIGN);
  }
  env << ',' << ' ';
}

// -------------------- Declarator --------------------
void Declarator::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Declarator");

  printVar(env, var, decl->getDeclaratorId());
  D_bitfield *b = dynamic_cast<D_bitfield*>(decl);
  if (b) {
    env << ':';
    b->bits->print(env, OPREC_LOWEST);
  }
  printInitializerOpt(env, init);
}


void Declarator::detailPrint(PrintEnv &env)
{
  decl->detailPrint(env);
  printInitializerOpt(env, init);
}


// -------------------- IDeclarator --------------------
void D_name::detailPrint(PrintEnv &env)
{
  if (name) {
    name->print(env);
  }
}


void D_pointer::detailPrint(PrintEnv &env)
{
  env << "*";
  if (cv) {
    env << " " << toString(cv) << " ";
  }
  base->detailPrint(env);
}


void D_reference::detailPrint(PrintEnv &env)
{
  env << "&";
  base->detailPrint(env);
}


void D_func::detailPrint(PrintEnv &env)
{
  base->detailPrint(env);
  env << "(";

  FAKELIST_FOREACH_NC(ASTTypeId, params, param) {
    param->print(env);
    if (param->next) {
      env << ", ";
    }
  }

  env << ")";

  if (cv) {
    env << " " << toString(cv);
  }

  if (exnSpec) {
    env << " ";
    exnSpec->print(env);
  }
}


void D_array::detailPrint(PrintEnv &env)
{
  base->detailPrint(env);
  env << "[";
  if (size) {
    size->print(env, OPREC_LOWEST);
  }
  env << "]";
}


void D_bitfield::detailPrint(PrintEnv &env)
{
  if (name) {
    name->print(env);
  }

  env << " : ";

  bits->print(env, OPREC_LOWEST);
}


void D_ptrToMember::detailPrint(PrintEnv &env)
{
  nestedName->print(env);
  env << "::*";
  if (cv) {
    env << " " << toString(cv) << " ";
  }
  base->detailPrint(env);
}


void D_grouping::detailPrint(PrintEnv &env)
{
  // It might be nice to automatically supply the parens where needed,
  // similar to how I do for expressions, but I don't have a pressing
  // need for that, and I'm concerned it might be complicated due to the
  // possibility of introducing ambiguities with the expression syntax.

  env << "(";
  base->detailPrint(env);
  env << ")";
}


// ------------------- ExceptionSpec --------------------
void ExceptionSpec::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("ExceptionSpec");
  env << "throw(";
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    if (iter != fl_first(types)) {
      env << ", ";
    }
    iter->print(env);
  }
  env << ")";
}


// ---------------------- OperatorName ---------------------
void ON_newDel::print(PrintEnv &env)
{
  env << "operator "
           << (isNew? "new" : "delete")
           << (isArray? "[]" : "");
}


void ON_operator::print(PrintEnv &env)
{
  env << "operator" << toString(op);
}


void ON_conversion::print(PrintEnv &env)
{
  env << "operator ";
  type->print(env);
}


// ---------------------- Statement ---------------------
void Statement::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Statement");
  env.loc = loc;
  iprint(env);
  //    env << ";\n";
}

// no-op
void S_skip::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_skip::iprint");
  env << ";\n";
}

void S_label::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_label::iprint");
  env << name << ':';
  s->print(env);
}

void S_case::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_case::iprint");
  env << "case";
  expr->print(env, OPREC_LOWEST);
  env << ':';
  s->print(env);
}

void S_default::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_default::iprint");
  env << "default:";
  s->print(env);
}

void S_expr::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_expr::iprint");
  expr->print(env);
  env << ";\n";
}

void S_compound::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_compound::iprint");
  PairDelim pair(env, "", "{\n", "}\n");
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->print(env);
  }
}

void S_if::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_if::iprint");
  {
    PairDelim pair(env, "if ", "(", ")");
    cond->print(env);
  }
  thenBranch->print(env);
  env << "else ";
  elseBranch->print(env);
}

void S_switch::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_switch::iprint");
  {
    PairDelim pair(env, "switch ", "(", ")");
    cond->print(env);
  }
  branches->print(env);
}

void S_while::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_while::iprint");
  {
    PairDelim pair(env, "while ", "(", ")");
    cond->print(env);
  }
  body->print(env);
}

void S_doWhile::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_doWhile::iprint");
  {
    PairDelim pair(env, "do");
    body->print(env);
  }
  {
    PairDelim pair(env, "while ", "(", ")");
    expr->print(env);
  }
  env << ";\n";
}

void S_for::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_for::iprint");
  {
    PairDelim pair(env, "for ", "(", ")");
    init->print(env);
    // this one not needed as the declaration provides one
    //          env << ";";
    cond->print(env);
    env << ';';
    after->print(env);
  }
  body->print(env);
}

void S_break::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_break::iprint");
  env << "break;\n";
}

void S_continue::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_continue::iprint");
  env << "continue;\n";
}

void S_return::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_return::iprint");
  env << "return";
  if (expr) {
    env << " ";
    expr->print(env);
  }
  env << ";\n";
}

void S_goto::iprint(PrintEnv &env)
{
  // dsw: When doing a control-flow pass, keep a current function so
  // we know where to look for the label.
  TreeWalkDebug treeDebug("S_goto::iprint");
  env << "goto ";
  env << target;
  env << ";\n";
}

void S_decl::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_decl::iprint");
  decl->print(env);
  //      env << ";\n";
}

void S_try::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_try::iprint");
  env << "try";
  body->print(env);
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->print(env);
  }
}

void S_asm::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_asm::iprint");
  env << "asm(" << text << ");\n";
}

void S_namespaceDecl::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_namespaceDecl::iprint");
  decl->print(env);
}


// ------------------- Condition --------------------
// CN = ConditioN

// this situation: if (gronk()) {...
void CN_expr::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("CN_expr");
  expr->print(env);
}

// this situation: if (bool b=gronk()) {...
void CN_decl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("CN_decl");
  typeId->print(env);
}

// ------------------- Handler ----------------------
// catch clause
void Handler::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Handler");
  {
    PairDelim pair(env, "catch ", "(", ") ");
    if (isEllipsis()) {
      env << "...";
    }
    else {
      typeId->print(env);
    }
  }
  body->print(env);
}


// ------------------- Full Expression print -----------------------
void FullExpression::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("FullExpression");
  // FIX: for now I omit printing the declarations of the temporaries
  // since we really don't have a syntax for it.  We would have to
  // print some curlies somewhere to make it legal to parse it back in
  // again, and we aren't using E_statement, so it would not reflect
  // the actual ast.
  if (expr) {
    expr->print(env, OPREC_LOWEST);
  } else {
    // 2006-05-25
    //   TODO: this can happen e.g.
    //       struct S1 func () {
    //         struct S1 v;
    //         ({ return v; });
    //       }
    //
    // xassert(expr && "39ce4334-0ca1-4e19-aaf9-7f27f335a629");
  }
}


// ------------------- Expression print -----------------------


void Expression::print(PrintEnv &env, OperatorPrecedence parentPrec)
{
  TreeWalkDebug treeDebug("Expression");

  OperatorPrecedence thisPrec = this->getPrecedence();
  if (thisPrec >= parentPrec) {
    PairDelim pair(env, "", "(", ")");
    iprint(env);
  }
  else {
    // 'this' expression has higher precedence than its parent, so does
    // not need parentheses.
    iprint(env);
  }
}


string Expression::exprToString() const
{
  TreeWalkDebug treeDebug("Expression::exprToString");
  stringBuilder sb;
  StringBuilderOutStream out0(sb);
  CodeOutStream codeOut(out0);

  // TODO: This is not right since we're just conjuring a new language
  // object rather than passing down the one created at the top level.
  // But for the moment it's not causing me problems so I'll avoid the
  // work of plumbing the real language for now.
  CCLang lang;

  CTypePrinter typePrinter(lang);
  Restorer<bool> res0(CTypePrinter::enabled, true);
  PrintEnv env(typePrinter, &codeOut);

  // sm: I think all the 'print' methods should be 'const', but
  // I'll leave such a change up to this module's author (dsw)
  const_cast<Expression*>(this)->print(env, OPREC_LOWEST);
  codeOut.flush();

  return sb;
}

string renderExpressionAsString(char const *prefix, Expression const *e)
{
  return stringc << prefix << e->exprToString();
}

char *expr_toString(Expression const *e)
{
  // this function is defined in smbase/strutil.cc
  return copyToStaticBuffer(e->exprToString().c_str());
}

int expr_debugPrint(Expression const *e)
{
  e->debugPrint(cout, 0);
  return 0;    // make gdb happy?
}


void E_boolLit::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_boolLit::iprint");
  env << b;
}

OperatorPrecedence E_boolLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_intLit::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_intLit::iprint");
  // FIX: do this correctly from the internal representation
  // fails to print the trailing U for an unsigned int.
//    env << i;
  env << text;
}

OperatorPrecedence E_intLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_floatLit::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_floatLit::iprint");
  // FIX: do this correctly from the internal representation
  // this fails to print ".0" for a float/double that happens to lie
  // on an integer boundary
//    env << d;
  // doing it this way also preserves the trailing "f" for float
  // literals
  env << text;
}

OperatorPrecedence E_floatLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_stringLit::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_stringLit::iprint");

  E_stringLit *p = this;
  while (p) {
    env << p->text;
    p = p->continuation;
    if (p) {
      env << ' ';
    }
  }
}

OperatorPrecedence E_stringLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_charLit::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_charLit::iprint");
  env << text;
}

OperatorPrecedence E_charLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_this::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_this::iprint");
  env << "this";
}

OperatorPrecedence E_this::getPrecedence() const
{
  return OPREC_HIGHEST;
}


// modified from STemplateArgument::toString()
void printSTemplateArgument(PrintEnv &env, STemplateArgument const *sta)
{
  switch (sta->kind) {
    default: xfailure("bad kind");
    case STemplateArgument::STA_NONE:
      env << string("STA_NONE");
      break;
    case STemplateArgument::STA_TYPE:
      {
      // FIX: not sure if this is a bug but there is no abstract value
      // lying around to be printed here so we just print what we
      // have; enable the normal type printer temporarily in order to
      // do this
      Restorer<bool> res0(CTypePrinter::enabled, true);
      env.ptype(sta->value.t); // assume 'type' if no comment
      }
      break;
    case STemplateArgument::STA_INT:
      env << stringc << "/*int*/ " << sta->value.i;
      break;
    case STemplateArgument::STA_ENUMERATOR:
      env << stringc << "/*enum*/ " << sta->value.v->name;
      break;
    case STemplateArgument::STA_REFERENCE:
      env << stringc << "/*ref*/ " << sta->value.v->name;
      break;
    case STemplateArgument::STA_POINTER:
      env << stringc << "/*ptr*/ &" << sta->value.v->name;
      break;
    case STemplateArgument::STA_MEMBER:
      env << stringc
              << "/*member*/ &" << sta->value.v->scope->curCompound->name
              << "::" << sta->value.v->name;
      break;
    case STemplateArgument::STA_DEPEXPR:
      // OPREC_RELATIONAL because we need parens if there are '<' or '>'
      // in the expression.
      sta->getDepExpr()->print(env, OPREC_RELATIONAL);
      break;
    case STemplateArgument::STA_TEMPLATE:
      env << string("template (?)");
      break;
    case STemplateArgument::STA_ATOMIC:
      env << sta->value.at->toString();
      break;
  }
}

// print template args, if any
void printTemplateArgs(PrintEnv &env, Variable *var)
{
  if (!( var && var->templateInfo() )) {
    return;
  }

  TemplateInfo *tinfo = var->templateInfo();
  int totArgs = tinfo->arguments.count();
  if (totArgs == 0) {
    return;
  }

  // use only arguments that apply to non-inherited parameters
  int args = totArgs;
  if (tinfo->isInstantiation()) {
    args = tinfo->instantiationOf->templateInfo()->params.count();
    if (args == 0) {
      return;
    }
  }

  // print final 'args' arguments
  ObjListIter<STemplateArgument> iter(var->templateInfo()->arguments);
  for (int i=0; i < (totArgs-args); i++) {
    iter.adv();
  }
  env << '<';
  int ct=0;
  for (; !iter.isDone(); iter.adv()) {
    if (ct++ > 0) {
      env << ',' << ' ';
    }
    printSTemplateArgument(env, iter.data());
  }
  env << '>';
}

void E_variable::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_variable::iprint");
  if (var && var->isBoundTemplateParam()) {
    // this is a bound template variable, so print its value instead
    // of printing its name
    xassert(var->value);

    // OPREC_HIGHEST because we do not know what context we are in here,
    // so should always use parentheses.
    var->value->print(env, OPREC_HIGHEST);
  }
  else {
    env << name->qualifierString() << name->getName();
    printTemplateArgs(env, var);
  }
}

OperatorPrecedence E_variable::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void printArgExprList(PrintEnv &env, FakeList<ArgExpression> *list)
{
  TreeWalkDebug treeDebug("printArgExprList");
  bool first_time = true;
  FAKELIST_FOREACH_NC(ArgExpression, list, iter) {
    if (first_time) first_time = false;
    else env << ',' << ' ';
    iter->expr->print(env, OPREC_COMMA);
  }
}

void E_funCall::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_funCall::iprint");
  func->print(env, this->getPrecedence());
  PairDelim pair(env, "", "(", ")");
  printArgExprList(env, args);
}

OperatorPrecedence E_funCall::getPrecedence() const
{
  return OPREC_POSTFIX;
}


void E_constructor::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_constructor::iprint");

  TypeLike const *type0 = env.typePrinter.getE_constructorTypeLike(this);
  Restorer<bool> res0(CTypePrinter::enabled, type == type0);

  env.ptype(type0);
  PairDelim pair(env, "", "(", ")");
  printArgExprList(env, args);
}

OperatorPrecedence E_constructor::getPrecedence() const
{
  return OPREC_POSTFIX;
}


void printVariableName(PrintEnv &env, Variable *var)
{
  // I anticipate possibly expanding this to cover more cases
  // of Variables that need to printed specially, possibly
  // including printing needed qualifiers.

  if (var->type->isFunctionType() &&
      var->type->asFunctionType()->isConversionOperator()) {
    // the name is just "conversion-operator", so print differently
    env << "/""*conversion*/operator(";
    Type *t = var->type->asFunctionType()->retType;
    Restorer<bool> res0(CTypePrinter::enabled, true);
    env.ptype(t);
    env << ')';
    return;
  }

  // normal case
  env << var->name;
}

void E_fieldAcc::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_fieldAcc::iprint");
  obj->print(env, this->getPrecedence());
  env << '.';
  if (field &&
      !field->type->isDependent()) {
    printVariableName(env, field);
    printTemplateArgs(env, field);
  }
  else {
    // the 'field' remains NULL if we're in a template
    // function and the 'obj' is dependent on the template
    // arguments.. there are probably a few other places
    // lurking that will need similar treatment, because
    // typechecking of templates is very incomplete and in
    // any event when checking the template *itself* (as
    // opposed to an instantiation) we never have enough
    // information to fill in all the variable references..
    env << fieldName->toString();
  }
}

OperatorPrecedence E_fieldAcc::getPrecedence() const
{
  return OPREC_POSTFIX;
}


void E_arrow::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_arrow::iprint");

  // E_arrow shouldn't normally be present in code that is to be
  // prettyprinted, so it doesn't much matter what this does.
  obj->print(env, this->getPrecedence());
  env << "->";
  fieldName->print(env);
}

OperatorPrecedence E_arrow::getPrecedence() const
{
  return OPREC_POSTFIX;
}


void E_sizeof::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_sizeof::iprint");

  env << "sizeof(";
  expr->print(env, OPREC_LOWEST);
  env << ")";
}

OperatorPrecedence E_sizeof::getPrecedence() const
{
  // I choose to always put parens, even though they are optional here.
  return OPREC_HIGHEST;
}


// dsw: unary expression?
void E_unary::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_unary::iprint");
  env << toString(op);
  expr->print(env, this->getPrecedence());
}

OperatorPrecedence E_unary::getPrecedence() const
{
  switch (op) {
    default:            xfailure("bad unary operator");
    case UNY_PLUS:      return OPREC_ADD;
    case UNY_MINUS:     return OPREC_ADD;
    case UNY_NOT:       return OPREC_PREFIX;
    case UNY_BITNOT:    return OPREC_PREFIX;
  }
}


void E_effect::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_effect::iprint");
  if (!isPostfix(op)) env << toString(op);
  expr->print(env, this->getPrecedence());
  if (isPostfix(op)) env << toString(op);
}

OperatorPrecedence E_effect::getPrecedence() const
{
  switch (op) {
    default:            xfailure("bad effect operator");
    case EFF_POSTINC:   return OPREC_POSTFIX;
    case EFF_POSTDEC:   return OPREC_POSTFIX;
    case EFF_PREINC:    return OPREC_PREFIX;
    case EFF_PREDEC:    return OPREC_PREFIX;
  }
}


// In my opinion, there is a "gray area" in C/C++ operator precedence
// where a lot of people, including myself, do not easily remember the
// exact order.  When both operators in an expression are in the gray
// area, I will print parens even when not strictly necessary.
static bool isGrayArea(OperatorPrecedence prec)
{
  return OPREC_SHIFT <= prec && prec <= OPREC_BIT_OR;
}

static bool bothGray(OperatorPrecedence p1, OperatorPrecedence p2)
{
  if (p1 == OPREC_SHIFT && p2 == OPREC_SHIFT) {
    // Shift versus shift is an exception since the "x << y << z" idiom
    // is so common in C++.  No parens needed.
    return false;
  }

  return isGrayArea(p1) && isGrayArea(p2);
}

// dsw: binary operator.
void E_binary::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_binary::iprint");

  OperatorPrecedence thisPrec = this->getPrecedence();
  OperatorPrecedence e1Prec   = e1->getPrecedence();
  OperatorPrecedence e2Prec   = e2->getPrecedence();

  if (bothGray(thisPrec, e1Prec)) {
    // Always print parens when both operators are in the gray area.
    e1->print(env, OPREC_HIGHEST);
  }
  else {
    // Pretend the parent has one level lower precedence in order to
    // take advantage of left associativity.
    e1->print(env, (OperatorPrecedence)(thisPrec+1));
  }

  if (op != BIN_BRACKETS) {
    // Does this operator or either operand have precedence of shift
    // or lower?  If so, we'll put spaces around this operator.
    bool shiftOrLower =
      thisPrec >= OPREC_SHIFT ||
      e1Prec   >= OPREC_SHIFT ||
      e2Prec   >= OPREC_SHIFT;

    if (op != BIN_COMMA && shiftOrLower) {
      env << " ";
    }

    env << toString(op);

    if (shiftOrLower) {
      env << " ";
    }

    if (bothGray(thisPrec, e2Prec)) {
      // Use parens.
      e2->print(env, OPREC_HIGHEST);
    }
    else {
      e2->print(env, thisPrec);
    }
  }

  else {
    env << "[";
    e2->print(env, OPREC_LOWEST);
    env << "]";
  }
}

OperatorPrecedence E_binary::getPrecedence() const
{
  switch (op) {
    default:                 xfailure("bad binary operator");

    case BIN_EQUAL:          return OPREC_EQUALITY;
    case BIN_NOTEQUAL:       return OPREC_EQUALITY;
    case BIN_LESS:           return OPREC_RELATIONAL;
    case BIN_GREATER:        return OPREC_RELATIONAL;
    case BIN_LESSEQ:         return OPREC_RELATIONAL;
    case BIN_GREATEREQ:      return OPREC_RELATIONAL;

    case BIN_MULT:           return OPREC_MULTIPLY;
    case BIN_DIV:            return OPREC_MULTIPLY;
    case BIN_MOD:            return OPREC_MULTIPLY;
    case BIN_PLUS:           return OPREC_ADD;
    case BIN_MINUS:          return OPREC_ADD;
    case BIN_LSHIFT:         return OPREC_SHIFT;
    case BIN_RSHIFT:         return OPREC_SHIFT;
    case BIN_BITAND:         return OPREC_BIT_AND;
    case BIN_BITXOR:         return OPREC_BIT_XOR;
    case BIN_BITOR:          return OPREC_BIT_OR;
    case BIN_AND:            return OPREC_LOGICAL_AND;
    case BIN_OR:             return OPREC_LOGICAL_OR;
    case BIN_COMMA:          return OPREC_COMMA;

    // https://gcc.gnu.org/pipermail/gcc-help/2011-March/102507.html
    case BIN_MINIMUM:        return OPREC_RELATIONAL;
    case BIN_MAXIMUM:        return OPREC_RELATIONAL;

    case BIN_BRACKETS:       return OPREC_POSTFIX;

    case BIN_ASSIGN:         return OPREC_ASSIGN;

    case BIN_DOT_STAR:       return OPREC_PTR_TO_MEMB;
    case BIN_ARROW_STAR:     return OPREC_PTR_TO_MEMB;

    // The old theorem prover used a different prec/assoc scheme than
    // the base language, so it doesn't make much sense to assign them a
    // precedence.  I will use LOWEST since that will safely cause them
    // to be printed with parentheses (if I ever even print them).
    case BIN_IMPLIES:        return OPREC_LOWEST;
    case BIN_EQUIVALENT:     return OPREC_LOWEST;
  }
}


void E_addrOf::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_addrOf::iprint");
  env << "&";
  if (expr->isE_variable()) {
    // could be forming ptr-to-member, do not parenthesize
    expr->iprint(env);
  }
  else {
    expr->print(env, this->getPrecedence());
  }
}

OperatorPrecedence E_addrOf::getPrecedence() const
{
  return OPREC_PREFIX;
}


void E_deref::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_deref::iprint");
  env << "*";
  ptr->print(env, this->getPrecedence());
}

OperatorPrecedence E_deref::getPrecedence() const
{
  return OPREC_PREFIX;
}


// C-style cast
void E_cast::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_cast::iprint");
  {
    PairDelim pair(env, "", "(", ")");
    ctype->print(env);
  }
  expr->print(env, this->getPrecedence());
}

OperatorPrecedence E_cast::getPrecedence() const
{
  return OPREC_PREFIX;
}


// ? : syntax
void E_cond::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_cond::iprint");

  cond->print(env, this->getPrecedence());

  if (th) {
    env << "? ";
    th->print(env, this->getPrecedence());
    env << " : ";
  }
  else {
    // GNU binary conditional.
    env << " ?: ";
  }

  el->print(env, this->getPrecedence());
}

OperatorPrecedence E_cond::getPrecedence() const
{
  return OPREC_ASSIGN;
}


void E_sizeofType::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_sizeofType::iprint");
  PairDelim pair(env, "sizeof", "(", ")"); // NOTE yes, you do want the parens because argument is a type.
  atype->print(env);
}

OperatorPrecedence E_sizeofType::getPrecedence() const
{
  return OPREC_PREFIX;
}


void E_assign::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_assign::iprint");

  target->print(env, this->getPrecedence());

  env << " ";
  if (op != BIN_ASSIGN) {
    env << toString(op);
  }
  env << "= ";

  src->print(env, this->getPrecedence());
}

OperatorPrecedence E_assign::getPrecedence() const
{
  return OPREC_ASSIGN;
}


void E_new::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_new::iprint");
  if (colonColon) env << "::";
  env << "new ";
  if (placementArgs) {
    PairDelim pair(env, "", "(", ")");
    printArgExprList(env, placementArgs);
  }

  if (!arraySize) {
    // no array size, normal type-id printing is fine
    atype->print(env);
  }
  else {
    // sm: to correctly print new-declarators with array sizes, we
    // need to dig down a bit, because the arraySize is printed right
    // where the variable name would normally go in an ordinary
    // declarator
    //
    // for example, suppose the original syntax was
    //   new int [n][5];
    // the type-id of the object being allocated is read as
    // "array of 5 ints" and 'n' of them are created; so:
    //   "array of 5 ints"->leftString()   is "int"
    //   arraySize->print()                is "n"
    //   "array of 5 ints"->rightString()  is "[5]"
    Type const *t = atype->decl->var->type;   // type-id in question
    env << t->leftString() << "[";
    arraySize->print(env, OPREC_LOWEST);
    env << "]" << t->rightString();
  }

  if (ctorArgs) {
    PairDelim pair(env, "", "(", ")");
    printArgExprList(env, ctorArgs->list);
  }
}

OperatorPrecedence E_new::getPrecedence() const
{
  return OPREC_PREFIX;
}


void E_delete::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_delete::iprint");
  if (colonColon) env << "::";
  env << "delete";
  if (array) env << "[]";

  // There was previously a comment here claiming that 'expr' could be
  // NULL, but that makes no sense.
  xassert(expr);

  env << " ";
  expr->print(env, this->getPrecedence());
}

OperatorPrecedence E_delete::getPrecedence() const
{
  return OPREC_PREFIX;
}


void E_throw::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_throw::iprint");
  env << "throw";
  if (expr) {
    env << " ";
    expr->print(env, this->getPrecedence());
  }
}

OperatorPrecedence E_throw::getPrecedence() const
{
  return OPREC_ASSIGN;
}


// C++-style cast
void E_keywordCast::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_keywordCast::iprint");
  env << toString(key);
  {
    PairDelim pair(env, "", "<", ">");
    ctype->print(env);
  }
  PairDelim pair(env, "", "(", ")");
  expr->print(env, this->getPrecedence());
}

OperatorPrecedence E_keywordCast::getPrecedence() const
{
  // The syntax includes its own parentheses, and precedence cannot be
  // used to interfere with the binding of the cast keyword to the angle
  // brackets.
  return OPREC_HIGHEST;
}


// RTTI: typeid(expression)
void E_typeidExpr::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_typeidExpr::iprint");
  PairDelim pair(env, "typeid", "(", ")");
  expr->print(env, this->getPrecedence());
}

OperatorPrecedence E_typeidExpr::getPrecedence() const
{
  // Syntax has its own parentheses.
  return OPREC_HIGHEST;
}


// RTTI: typeid(type)
void E_typeidType::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_typeidType::iprint");
  PairDelim pair(env, "typeid", "(", ")");
  ttype->print(env);
}

OperatorPrecedence E_typeidType::getPrecedence() const
{
  // Syntax has its own parentheses.
  return OPREC_HIGHEST;
}


void E_grouping::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_grouping::iprint");

  // Do not print these parentheses.  The printer will insert parens
  // where they are needed.
  expr->print(env, OPREC_LOWEST);
}

OperatorPrecedence E_grouping::getPrecedence() const
{
  // We never print parens for E_grouping itself.
  return OPREC_HIGHEST;
}


void E_implicitStandardConversion::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_implicitStandardConversion::iprint");

  env << "/*ISC:" << type->toString() << "*/";
  expr->iprint(env);
}

OperatorPrecedence E_implicitStandardConversion::getPrecedence() const
{
  // Since the only thing we print is a comment, we behave transparentlt
  // w.r.t. printing.
  return expr->getPrecedence();
}


// ----------------------- Initializer --------------------

// this is under a declaration
// int x = 3;
//         ^ only
void IN_expr::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("IN_expr");
  e->print(env, OPREC_ASSIGN);
}

// int x[] = {1, 2, 3};
//           ^^^^^^^^^ only
void IN_compound::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("IN_compound");
  PairDelim pair(env, "", "{\n", "\n}");
  bool first_time = true;
  FOREACH_ASTLIST_NC(Initializer, inits, iter) {
    if (first_time) first_time = false;
    else env << ",\n";
    iter.data()->print(env);
  }
}

void IN_ctor::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("IN_ctor");
  printArgExprList(env, args);
}

// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TemplateDeclaration");

  env << "template <";
  int ct=0;
  for (TemplateParameter *iter = params; iter; iter = iter->next) {
    if (ct++ > 0) {
      env << ", ";
    }
    iter->print(env);
  }
  env << ">\n";

  iprint(env);
}

void printFuncInstantiations(PrintEnv &env, Variable const *var)
{
  TemplateInfo *ti = var->templateInfo();
  SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
    Variable const *inst = iter.data();
    if (inst->funcDefn) {
      inst->funcDefn->print(env);
    }
    else {
      env << inst->toQualifiedString() << ";    // decl but not defn\n";
    }
  }
}

void TD_func::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TD_func");
  f->print(env);

  // print instantiations
  Variable *var = f->nameAndParams->var;
  if (var->isTemplate() &&      // for complete specializations, don't print
      !var->templateInfo()->isPartialInstantiation()) {     // nor partial inst
    env << "#if 0    // instantiations of ";
    // NOTE: inlined from Variable::toCString()

    TypeLike const *type0 = env.getTypeLike(var);
    env.ptype(type0, (var->name? var->name : "/*anon*/"));
    env << var->namePrintSuffix() << "\n";
    printFuncInstantiations(env, var);

    TemplateInfo *varTI = var->templateInfo();
    if (!varTI->definitionTemplateInfo) {
      // little bit of a hack: if this does not have a
      // 'definitionTemplateInfo', then it was defined inline, and
      // the partial instantiations will be printed when the class
      // instantiation is
    }
    else {
      // also look in partial instantiations
      SFOREACH_OBJLIST(Variable, varTI->partialInstantiations, iter) {
        printFuncInstantiations(env, iter.data());
      }
    }

    env << "#endif   // instantiations of " << var->name << "\n\n";
  }
}

void TD_decl::iprint(PrintEnv &env)
{
  d->print(env);

  // print instantiations
  if (d->spec->isTS_classSpec()) {
    CompoundType *ct = d->spec->asTS_classSpec()->ctype;
    TemplateInfo *ti = ct->typedefVar->templateInfo();
    if (!ti->isCompleteSpec()) {
      env << "#if 0    // instantiations of " << ct->name << "\n";

      SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
        Variable const *instV = iter.data();

        env << "// ";
        TypeLike const *type0 = env.getTypeLike(instV);
        env.ptype(type0);
        CompoundType *instCT = instV->type->asCompoundType();
        if (instCT->syntax) {
          env << "\n";
          instCT->syntax->print(env);
          env << ";\n";
        }
        else {
          env << ";     // body not instantiated\n";
        }
      }
      env << "#endif   // instantiations of " << ct->name << "\n\n";
    }
  }
  else {
    // it could be a forward declaration of a template class;
    // do nothing more
  }
}

void TD_tmember::iprint(PrintEnv &env)
{
  d->print(env);
}


// ------------------- TemplateParameter ------------------
void TP_type::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TP_type");
  env << "class " << (name? name : "/*anon*/");

  if (defaultType) {
    env << " = ";
    defaultType->print(env);
  }
}

void TP_nontype::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TP_nontype");
  param->print(env);
}


// -------------------- TemplateArgument ------------------
void TA_type::print(PrintEnv &env)
{
  // dig down to prevent printing "/*anon*/" since template
  // type arguments are always anonymous so it's just clutter
  Restorer<bool> res0(CTypePrinter::enabled, true);
  env.ptype(type->decl->var->type);
}

void TA_nontype::print(PrintEnv &env)
{
  expr->print(env, OPREC_RELATIONAL /* use parens if '<' or '>' */);
}

void TA_templateUsed::print(PrintEnv &env)
{
  // the caller should have recognized the presence of TA_templateUsed,
  // adjusted its printing accordingly, and then skipped this element
  xfailure("do not print TA_templateUsed");
}


// -------------------- NamespaceDecl ---------------------
void ND_alias::print(PrintEnv &env)
{
  env << "namespace " << alias << " = ";
  original->print(env);
  env << ";\n";
}

void ND_usingDecl::print(PrintEnv &env)
{
  env << "using ";
  name->print(env);
  env << ";\n";
}

void ND_usingDir::print(PrintEnv &env)
{
  env << "using namespace ";
  name->print(env);
  env << ";\n";
}


// EOF
