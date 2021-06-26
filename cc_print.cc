// cc_print.cc            see license.txt for copyright and terms of use
// code for cc_print.h

// Adapted from cc_tcheck.cc by Daniel Wilkerson dsw@cs.berkeley.edu

#include "cc_print.h"                  // this module

// elsa
#include "cc-lang.h"                   // CCLang

// smbase
#include "trace.h"                     // trace


// This is a dummy global so that this file will compile both in
// default mode and in qualifiers mode.
class dummyType;                // This does nothing.
dummyType *ql;
string toString(class dummyType*) {return "";}


// **************** class PrintEnv

string PrintEnv::getResult()
{
  BPBox *tree = this->takeTree();

  // Dumping the BoxPrint tree can be useful for diagnosing bad
  // formatting.
  if (tracingSys("dumpPrintEnvTree")) {
    tree->debugPrint(cout, 0);
    cout << "\n";
  }

  BPRender bpRender;
  tree->render(bpRender);
  string rendered = bpRender.sb.str();
  delete tree;

  return rendered;
}

void PrintEnv::ptype(Type const *type, char const *name)
{
  *this << m_typePrinter.printType(type, name);
}


string printTypeToString(CCLang const &lang, Type const *type)
{
  CTypePrinter typePrinter(lang);
  PrintEnv env(typePrinter);
  env.ptype(type, "" /*do not print "anon"*/);
  return env.getResult();
}


// ****************

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

// Print the flags in 'dflags' that can appear in source code.  If there
// are any, print a space afterward.
static void printDeclFlags(PrintEnv &env, DeclFlags dflags)
{
  DeclFlags sourceFlags = dflags & DF_SOURCEFLAGS;
  if (sourceFlags) {
    env << toString(sourceFlags) << " ";
  }
}

// this is a prototype for a function down near E_funCall::iprint
void printArgExprList(PrintEnv &env, FakeList<ArgExpression> *list);
static void printArgExprListWithParens(PrintEnv &env, FakeList<ArgExpression> *list);


// ------------------ BoxPrint helpers --------------------
namespace {
  // Begin and end a box with a scope.
  class BPBeginEndBox {
    BoxPrint &m_bp;
  public:
    BPBeginEndBox(BoxPrint &bp, BPKind kind)
      : m_bp(bp)
    {
      m_bp << kind;
    }
    ~BPBeginEndBox()
    {
      m_bp << BoxPrint::end;
    }
  };

  // Begin/end vertical or sequence boxes.
  #define BPVERTICAL BPBeginEndBox bpBeginEndBox(env, BoxPrint::vert)
  #define BPSEQUENCE BPBeginEndBox bpBeginEndBox(env, BoxPrint::seq)

  // Print opening and closing braces.
  class BPBraces {
    PrintEnv &m_env;
  public:
    BPBraces(PrintEnv &env)
      : m_env(env)
    {
      m_env << "{" << m_env.ind;
    }
    ~BPBraces()
    {
      m_env << m_env.und << "}";
    }
  };

  #define BPBRACES BPBraces bpBraces(env)

  // Temporarily un-indent.
  class BPUnindent {
    PrintEnv &m_env;
  public:
    BPUnindent(PrintEnv &env)
      : m_env(env)
    {
      m_env << m_env.und;
    }
    ~BPUnindent()
    {
      m_env << m_env.ind;
    }
  };

  #define BPUNINDENT BPUnindent bpUnindent(env)
}



// ------------------- TranslationUnit --------------------
static void printTopFormList(PrintEnv &env, ASTList<TopForm> const &topForms)
{
  FOREACH_ASTLIST(TopForm, topForms, iter) {
    iter.data()->print(env);

    // The first break puts a newline after the form.  The second
    // puts a blank line after each TopForm.
    env << env.br << env.br;
  }
}

void TranslationUnit::print(PrintEnv &env) const
{
  printTopFormList(env, topForms);
}

// --------------------- TopForm ---------------------
void TF_decl::print(PrintEnv &env) const
{
  decl->print(env);
}

void TF_func::print(PrintEnv &env) const
{
  f->print(env);
}

void TF_template::print(PrintEnv &env) const
{
  td->print(env);
}

void TF_explicitInst::print(PrintEnv &env) const
{
  env << "template ";
  d->print(env);
}

void TF_linkage::print(PrintEnv &env) const
{
  env << "extern " << linkageType << " ";
  BPBRACES;
  forms->print(env);
}

void TF_one_linkage::print(PrintEnv &env) const
{
  env << "extern " << linkageType << " ";
  form->print(env);
}

void TF_asm::print(PrintEnv &env) const
{
  env << "asm(";
  text->print(env, OPREC_LOWEST);
  env << ");";
}

void TF_namespaceDefn::print(PrintEnv &env) const
{
  env << "namespace " << (name? name : "/*anon*/") << " ";
  BPBRACES;
  printTopFormList(env, forms);
}

void TF_namespaceDecl::print(PrintEnv &env) const
{
  decl->print(env);
}


// --------------------- Function -----------------
static bool declaratorIsDestructor(Declarator const *declarator)
{
  // TODO: There should be a better way of recognizing these.
  if (declarator->var &&
      declarator->var->name &&
      declarator->var->name[0] == '~') {
    return true;
  }

  return false;
}

static bool declaratorIsConstructor(Declarator const *declarator)
{
  // TODO: There should be a better way of recognizing these.
  if (declarator->var &&
      declarator->var->name &&
      streq(declarator->var->name, specialName_constructor)) {
    return true;
  }

  return false;
}

// True if the declarator needs a space after the specifier 'spec'.
static bool ideclaratorWantsSpace(TypeSpecifier const *spec,
                                  IDeclarator const *id)
{
  if (TS_simple const *tss = spec->ifTS_simpleC()) {
    if (tss->id == ST_CDTOR) {
      // No space after the emptiness representing the "return type"
      // of conversion operators.
      return false;
    }
  }

  if (D_name const *dn = id->ifD_nameC()) {
    if (dn->name == NULL) {
      return false;
    }
  }

  return true;
}

// Print a type specifier followed by a declarator.
static void printTypeSpecifierAndDeclarator(
  PrintEnv &env, TypeSpecifier *spec, Declarator *declarator)
{
  if (declaratorIsConstructor(declarator)) {
    // Print the type specifier,  The return type of a constructor says
    // what we build, hence the name of the class.
    spec->print(env);

    // Now print the declarator.  The special constructor name will be
    // omitted, but we need the parens and the param types.
    declarator->print(env);
  }

  else if (declaratorIsDestructor(declarator)) {
    // Print the declarator only.  It has the tilde we want to print,
    // while the return type is 'void'.
    declarator->print(env);
  }

  else {
    // Print both, possibly with a space.
    spec->print(env);
    if (ideclaratorWantsSpace(spec, declarator->decl)) {
      env << " ";
    }
    declarator->print(env);
  }
}

void Function::print(PrintEnv &env, DeclFlags declFlagsMask) const
{
  BPVERTICAL;

  {
    BPSEQUENCE;

    printDeclFlags(env, dflags & declFlagsMask);
    printTypeSpecifierAndDeclarator(env, retspec, nameAndParams);
  }

  if (instButNotTchecked()) {
    // this is an unchecked instantiation
    env << "; // not instantiated" << env.fbr;
    return;
  }

  env << env.fbr;
  if (handlers) {
    env << "try";
    if (inits) {
      // Newline before member inits.
      env << env.fbr;
    }
    else {
      // Space before opening brace.
      env << " ";
    }
  }

  if (inits) {
    BPVERTICAL;

    env << "  : ";
    int ct=0;      // Number of initializers printed.
    FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
      if (ct > 0) {
        env << "," << env.br;
      }
      if (ct == 1) {
        env.adjustIndent(+2);
      }

      env << iter->name->toString();
      printArgExprListWithParens(env, iter->args);
      ct++;
    }
    env << env.br;
    if (ct >= 2) {
      // Undo the double-indent if we did it.
      env.adjustIndent(-2);
    }
  }

  if (body->stmts.isEmpty()) {
    // more concise
    env << "{}";
  }
  else {
    body->print(env);
  }

  if (handlers) {
    FAKELIST_FOREACH_NC(Handler, handlers, iter) {
      // We put a break after the body, and after each handler.  The
      // containing context is responsible for the break after the
      // last handler.
      env << env.br;
      iter->print(env);
    }
  }
}


// MemberInit

// -------------------- Declaration -------------------
void Declaration::print(PrintEnv &env) const
{
  // Check if the type specifier wants to print vertically.  I cannot
  // wait until I am inside 'spec->print()' because the declaration
  // flags get printed first.
  bool isClassOrEnum = spec->isTS_classSpec() || spec->isTS_enumSpec();

  BPBeginEndBox bpBeginEndBox(env,
    isClassOrEnum? BoxPrint::vert : BoxPrint::seq);

  printDeclFlags(env, dflags);

  spec->print(env);

  FAKELIST_FOREACH_NC(Declarator, decllist, declarator) {
    if (declarator != fl_first(decllist)) {
      env << "," << env.br;
    }
    else if (ideclaratorWantsSpace(spec, declarator->decl)) {
      env << " ";
    }
    declarator->print(env);
  }
  env << ";";
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
        // Constructor arguments.
        env << "(";
        ctor->print(env);
        env << ")";
      }
    } else {
      env << " = ";
      init->print(env);
    }
  }
}

void ASTTypeId::print(PrintEnv &env) const
{
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
        env << ',' << env.br;
      }
      args->print(env);
    }

    args = args->next;
  }
}

void PQ_qualifier::print(PrintEnv &env) const
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

void PQ_name::print(PrintEnv &env) const
{
  if (streq(name, specialName_constructor)) {
    // Do not print the name.
  }
  else {
    env << name;
  }
}

void PQ_operator::print(PrintEnv &env) const
{
  o->print(env);
}

void PQ_template::print(PrintEnv &env) const
{
  if (templateUsed()) {
    env << "template ";
  }

  env << name << '<';
  printTemplateArgumentList(env, templArgs);
  env << '>';
}


// --------------------- TypeSpecifier --------------
void TypeSpecifier::print(PrintEnv &env) const
{
  iprint(env);
  if (cv) {
    env << " " << toString(cv);
  }
}


void TS_name::iprint(PrintEnv &env) const
{
  if (typenameUsed) {
    env << "typename ";
  }
  name->print(env);
}


void TS_simple::iprint(PrintEnv &env) const
{
  if (id != ST_CDTOR) {
    env << toString(id);
  }
}


void TS_elaborated::iprint(PrintEnv &env) const
{
  env << toString(keyword) << " ";
  name->print(env);
}


void TS_classSpec::iprint(PrintEnv &env) const
{
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
    else {
      env << ',' << env.br;
    }
    iter->print(env);
  }
  env << " ";
  BPBRACES;
  FOREACH_ASTLIST(Member, members->list, iter2) {
    iter2.data()->print(env);
    if (iter2.data()->isMR_access()) {
      // Hack: MR_access prints its own break as part of BPUNINDENT,
      // so avoiding doing so here.
    }
    else {
      env << env.br;
    }
  }
}


void TS_enumSpec::iprint(PrintEnv &env) const
{
  env << toString(ql);          // see string toString(class dummyType*) above
  env << toString(cv);
  env << "enum ";
  if (name) {
    env << name << ' ';
  }
  BPBRACES;

  int ct=0;
  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    if (ct >= 1) {
      env << ',' << env.br;
    }
    iter->print(env);
    ct++;
  }

  // Break but no comma after the last enumerator.
  if (ct >= 1) {
    env << env.fbr;
  }
}


// ---------------------- BaseClassSpec ----------------------
void BaseClassSpec::print(PrintEnv &env) const {
  if (isVirtual) env << "virtual ";
  if (access!=AK_UNSPECIFIED) env << toString(access) << " ";
  env << name->toString();
}


// MemberList

// ---------------------- Member ----------------------
void MR_decl::print(PrintEnv &env) const
{
  d->print(env);
}

void MR_func::print(PrintEnv &env) const
{
  // Methods that are defined in their class body are implicitly inline,
  // and Elsa makes that explicit.  Remove DF_INLINE to reduce clutter.
  f->print(env, ~DF_INLINE);
}

void MR_access::print(PrintEnv &env) const
{
  BPUNINDENT;
  env << toString(k) << ":";
}

void MR_usingDecl::print(PrintEnv &env) const
{
  decl->print(env);
}

void MR_template::print(PrintEnv &env) const
{
  d->print(env);
}


// -------------------- Enumerator --------------------
void Enumerator::print(PrintEnv &env) const
{
  env << name;
  if (expr) {
    env << " = ";
    expr->print(env, OPREC_ASSIGN);
  }
}

// -------------------- Declarator --------------------
void Declarator::print(PrintEnv &env) const
{
  decl->print(env);
  printInitializerOpt(env, init);
}


// -------------------- IDeclarator --------------------
void D_name::print(PrintEnv &env) const
{
  if (name) {
    name->print(env);
  }
}


void D_pointer::print(PrintEnv &env) const
{
  env << "*";
  if (cv) {
    env << " " << toString(cv) << " ";
  }
  base->print(env);
}


void D_reference::print(PrintEnv &env) const
{
  env << "&";
  base->print(env);
}


// Return true if 'idecl' is a declarator whose syntax comes before
// the base declarator rather than after.
static bool isPrefixDeclarator(IDeclarator const *idecl)
{
  // Skip D_grouping because we won't print those.
  while (D_grouping const *g = idecl->ifD_groupingC()) {
    idecl = g->base;
  }

  return idecl->isD_pointer() ||
         idecl->isD_reference() ||
         idecl->isD_ptrToMember();
}


// Print base declarator 'base' in the context of a suffix declarator.
static void printBaseDeclaratorOfSuffixDeclarator(
  PrintEnv &env, IDeclarator const *base)
{
  if (isPrefixDeclarator(base)) {
    env << "(";
  }
  base->print(env);
  if (isPrefixDeclarator(base)) {
    env << ")";
  }
}


void D_func::print(PrintEnv &env) const
{
  printBaseDeclaratorOfSuffixDeclarator(env, base);

  env << "(";

  FAKELIST_FOREACH_NC(ASTTypeId, params, param) {
    param->print(env);
    if (param->next) {
      env << "," << env.br;
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


void D_array::print(PrintEnv &env) const
{
  printBaseDeclaratorOfSuffixDeclarator(env, base);

  env << "[";
  if (size) {
    size->print(env, OPREC_LOWEST);
  }
  env << "]";
}


void D_bitfield::print(PrintEnv &env) const
{
  if (name) {
    name->print(env);
  }

  env << " : ";

  bits->print(env, OPREC_LOWEST);
}


void D_ptrToMember::print(PrintEnv &env) const
{
  nestedName->print(env);
  env << "::*";
  if (cv) {
    env << " " << toString(cv) << " ";
  }
  base->print(env);
}


void D_grouping::print(PrintEnv &env) const
{
  // Like with E_grouping, we will drop the parens here, and instead
  // supply them automatically where needed.
  base->print(env);
}


// ------------------- ExceptionSpec --------------------
void ExceptionSpec::print(PrintEnv &env) const
{
  env << "throw(";
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    if (iter != fl_first(types)) {
      env << "," << env.br;
    }
    iter->print(env);
  }
  env << ")";
}


// ---------------------- OperatorName ---------------------
void ON_newDel::print(PrintEnv &env) const
{
  env << "operator "
           << (isNew? "new" : "delete")
           << (isArray? "[]" : "");
}


void ON_operator::print(PrintEnv &env) const
{
  env << "operator" << toString(op);
}


void ON_conversion::print(PrintEnv &env) const
{
  env << "operator ";
  type->print(env);
}


// ---------------------- Statement ---------------------
void Statement::print(PrintEnv &env) const
{
  iprint(env);
}

// no-op
void S_skip::iprint(PrintEnv &env) const
{
  env << ";";
}

void S_label::iprint(PrintEnv &env) const
{
  {
    BPUNINDENT;
    env << name << ":";
  }
  s->print(env);
}

void S_case::iprint(PrintEnv &env) const
{
  {
    BPUNINDENT;

    env << "case ";
    expr->print(env, OPREC_LOWEST);
    env << ":";
  }

  s->print(env);
}

void S_default::iprint(PrintEnv &env) const
{
  {
    BPUNINDENT;
    env << "default:";
  }
  s->print(env);
}

void S_expr::iprint(PrintEnv &env) const
{
  expr->print(env);
  env << ";";
}

void S_compound::iprint(PrintEnv &env) const
{
  BPBRACES;
  FOREACH_ASTLIST(Statement, stmts, iter) {
    iter.data()->print(env);
    env << env.br;
  }
}

void S_if::iprint(PrintEnv &env) const
{
  BPVERTICAL;

  env << "if (";
  cond->print(env);
  env << ") ";
  thenBranch->print(env);
  env << env.br << "else ";
  elseBranch->print(env);
}

void S_switch::iprint(PrintEnv &env) const
{
  BPVERTICAL;

  env << "switch (";
  cond->print(env);
  env << ") ";

  if (S_compound const *comp = branches->ifS_compoundC()) {
    // In the common case of an S_compound, use my preferred style where
    // labels are indented one level and everything else is indented two
    // levels.

    BPBRACES;
    env.adjustIndent(1);   // Additional indentation.
    FOREACH_ASTLIST(Statement, comp->stmts, iter) {
      iter.data()->print(env);
      env << env.br;
    }
    env.adjustIndent(-1);
  }
  else {
    branches->print(env);
  }
}

void S_while::iprint(PrintEnv &env) const
{
  env << "while (";
  cond->print(env);
  env << ") ";
  body->print(env);
}

void S_doWhile::iprint(PrintEnv &env) const
{
  env << "do ";
  body->print(env);
  env << " while (";
  expr->print(env);
  env << ");";
}

void S_for::iprint(PrintEnv &env) const
{
  env << "for (";
  {
    BPSEQUENCE;

    init->print(env);
    env << env.br;
    cond->print(env);
    env << "; ";
    after->print(env);
  }
  env << ") ";
  body->print(env);
}

void S_break::iprint(PrintEnv &env) const
{
  env << "break;";
}

void S_continue::iprint(PrintEnv &env) const
{
  env << "continue;";
}

void S_return::iprint(PrintEnv &env) const
{
  env << "return";
  if (expr) {
    env << " ";
    expr->print(env);
  }
  env << ";";
}

void S_goto::iprint(PrintEnv &env) const
{
  env << "goto ";
  env << target;
  env << ";";
}

void S_decl::iprint(PrintEnv &env) const
{
  decl->print(env);
}

void S_try::iprint(PrintEnv &env) const
{
  env << "try";
  body->print(env);
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->print(env);
  }
}

void S_asm::iprint(PrintEnv &env) const
{
  env << "asm(";
  text->print(env, OPREC_LOWEST);
  env << ");";
}

void S_namespaceDecl::iprint(PrintEnv &env) const
{
  decl->print(env);
}


// ------------------- Condition --------------------
// CN = ConditioN

// this situation: if (gronk()) {...
void CN_expr::print(PrintEnv &env) const
{
  expr->print(env);
}

// this situation: if (bool b=gronk()) {...
void CN_decl::print(PrintEnv &env) const
{
  typeId->print(env);
}

// ------------------- Handler ----------------------
// catch clause
void Handler::print(PrintEnv &env) const
{
  env << "catch (";
  if (isEllipsis()) {
    env << "...";
  }
  else {
    typeId->print(env);
  }
  env << ") ";
  body->print(env);
}


// ------------------- Full Expression print -----------------------
void FullExpression::print(PrintEnv &env) const
{
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


void Expression::print(PrintEnv &env, OperatorPrecedence parentPrec) const
{
  OperatorPrecedence thisPrec = this->getPrecedence();
  if (thisPrec >= parentPrec) {
    env << "(";
    iprint(env);
    env << ")";
  }
  else {
    // 'this' expression has higher precedence than its parent, so does
    // not need parentheses.
    iprint(env);
  }
}


string Expression::exprToString() const
{
  // TODO: This is not right since we're just conjuring a new language
  // object rather than passing down the one created at the top level.
  // But for the moment it's not causing me problems so I'll avoid the
  // work of plumbing the real language for now.
  CCLang lang;

  CTypePrinter typePrinter(lang);
  PrintEnv env(typePrinter);

  this->print(env, OPREC_LOWEST);
  return env.getResult();
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


void E_boolLit::iprint(PrintEnv &env) const
{
  env << b;
}

OperatorPrecedence E_boolLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_intLit::iprint(PrintEnv &env) const
{
  // FIX: do this correctly from the internal representation
  // fails to print the trailing U for an unsigned int.
//    env << i;
  env << text;
}

OperatorPrecedence E_intLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_floatLit::iprint(PrintEnv &env) const
{
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


void E_stringLit::iprint(PrintEnv &env) const
{
  E_stringLit const *p = this;
  while (p) {
    env << p->text;
    p = p->continuation;
    if (p) {
      env << env.br;
    }
  }
}

OperatorPrecedence E_stringLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_charLit::iprint(PrintEnv &env) const
{
  env << text;
}

OperatorPrecedence E_charLit::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_this::iprint(PrintEnv &env) const
{
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
              << "/*member*/ &"
              << sta->value.v->m_containingScope->curCompound->name
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
      env << ',' << env.br;
    }
    printSTemplateArgument(env, iter.data());
  }
  env << '>';
}

void E_variable::iprint(PrintEnv &env) const
{
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
  BPSEQUENCE;

  bool first_time = true;
  FAKELIST_FOREACH_NC(ArgExpression, list, iter) {
    if (first_time) {
      first_time = false;
    }
    else {
      env << ',' << env.br;
    }
    iter->expr->print(env, OPREC_COMMA);
  }
}

static void printArgExprListWithParens(PrintEnv &env,
                                       FakeList<ArgExpression> *list)
{
  env << "(";
  printArgExprList(env, list);
  env << ")";
}

void E_funCall::iprint(PrintEnv &env) const
{
  func->print(env, this->getPrecedence());
  printArgExprListWithParens(env, args);
}

OperatorPrecedence E_funCall::getPrecedence() const
{
  return OPREC_POSTFIX;
}


void E_constructor::iprint(PrintEnv &env) const
{
  TypeLike const *type0 = env.m_typePrinter.getE_constructorTypeLike(this);

  env.ptype(type0);
  printArgExprListWithParens(env, args);
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
    env.ptype(t);
    env << ')';
    return;
  }

  // normal case
  env << var->name;
}

void E_fieldAcc::iprint(PrintEnv &env) const
{
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


void E_arrow::iprint(PrintEnv &env) const
{
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


void E_sizeof::iprint(PrintEnv &env) const
{
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
void E_unary::iprint(PrintEnv &env) const
{
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


void E_effect::iprint(PrintEnv &env) const
{
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

// Return true if 'e' is something so simple that it makes sense to
// write it as an operand of '+' or similar without a separating space.
// So, for example, "x+y" is fine, but "a.x+y" should be written as
// "a.x + y".
//
// This isn't perfect because it produces "x*y * z" rather than "x*y*z"
// but I'll accept it for now.
static bool verySimple(Expression const *e)
{
  return e->isE_boolLit() ||
         e->isE_intLit() ||
         e->isE_floatLit() ||
         e->isE_charLit() ||
         e->isE_this() ||
         e->isE_variable();
}

// dsw: binary operator.
void E_binary::iprint(PrintEnv &env) const
{
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

    // Is either operand not very simple, and this operand an arithmetic
    // operator?  That too will cause a space.
    bool arithmeticOnNonIdentifier =
      (!verySimple(e1) || !verySimple(e2)) &&
      thisPrec >= OPREC_MULTIPLY;

    bool addSpaces = shiftOrLower || arithmeticOnNonIdentifier;

    if (op != BIN_COMMA && addSpaces) {
      env << " ";
    }

    env << toString(op);

    if (addSpaces) {
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


void E_addrOf::iprint(PrintEnv &env) const
{
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


void E_deref::iprint(PrintEnv &env) const
{
  env << "*";
  ptr->print(env, this->getPrecedence());
}

OperatorPrecedence E_deref::getPrecedence() const
{
  return OPREC_PREFIX;
}


// C-style cast
void E_cast::iprint(PrintEnv &env) const
{
  env << "(";
  ctype->print(env);
  env << ")";
  expr->print(env, this->getPrecedence());
}

OperatorPrecedence E_cast::getPrecedence() const
{
  return OPREC_PREFIX;
}


// ? : syntax
void E_cond::iprint(PrintEnv &env) const
{
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


void E_sizeofType::iprint(PrintEnv &env) const
{
  env << "sizeof(";
  atype->print(env);
  env << ")";
}

OperatorPrecedence E_sizeofType::getPrecedence() const
{
  return OPREC_PREFIX;
}


void E_assign::iprint(PrintEnv &env) const
{
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


void E_new::iprint(PrintEnv &env) const
{
  if (colonColon) env << "::";
  env << "new ";
  if (placementArgs) {
    printArgExprListWithParens(env, placementArgs);
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
    printArgExprListWithParens(env, ctorArgs->list);
  }
}

OperatorPrecedence E_new::getPrecedence() const
{
  return OPREC_PREFIX;
}


void E_delete::iprint(PrintEnv &env) const
{
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


void E_throw::iprint(PrintEnv &env) const
{
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
void E_keywordCast::iprint(PrintEnv &env) const
{
  env << toString(key) << "<";
  ctype->print(env);
  env << ">(";
  expr->print(env, this->getPrecedence());
  env << ")";
}

OperatorPrecedence E_keywordCast::getPrecedence() const
{
  // The syntax includes its own parentheses, and precedence cannot be
  // used to interfere with the binding of the cast keyword to the angle
  // brackets.
  return OPREC_HIGHEST;
}


// RTTI: typeid(expression)
void E_typeidExpr::iprint(PrintEnv &env) const
{
  env << "typeid(";
  expr->print(env, this->getPrecedence());
  env << ")";
}

OperatorPrecedence E_typeidExpr::getPrecedence() const
{
  // Syntax has its own parentheses.
  return OPREC_HIGHEST;
}


// RTTI: typeid(type)
void E_typeidType::iprint(PrintEnv &env) const
{
  env << "typeid(";
  ttype->print(env);
  env << ")";
}

OperatorPrecedence E_typeidType::getPrecedence() const
{
  // Syntax has its own parentheses.
  return OPREC_HIGHEST;
}


void E_grouping::iprint(PrintEnv &env) const
{
  // Do not print these parentheses.  The printer will insert parens
  // where they are needed.
  expr->print(env, OPREC_LOWEST);
}

OperatorPrecedence E_grouping::getPrecedence() const
{
  // We never print parens for E_grouping itself.
  return OPREC_HIGHEST;
}


void E_implicitStandardConversion::iprint(PrintEnv &env) const
{
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
void IN_expr::print(PrintEnv &env) const
{
  e->print(env, OPREC_ASSIGN);
}

// int x[] = {1, 2, 3};
//           ^^^^^^^^^ only
void IN_compound::print(PrintEnv &env) const
{
  // This does not print quite how I would like it to, as shown by
  // test/pprint/longlines.cc, but I'm not sure how much more effort I
  // want to put into it.  I think BoxPrint would need to be made
  // significantly more flexible.

  BPBRACES;

  {
    BPSEQUENCE;

    bool first_time = true;
    FOREACH_ASTLIST(Initializer, inits, iter) {
      if (first_time) {
        first_time = false;
      }
      else {
        env << "," << env.br;
      }
      iter.data()->print(env);
    }
  }

  env << env.br;
}

void IN_ctor::print(PrintEnv &env) const
{
  printArgExprList(env, args);
}

// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::print(PrintEnv &env) const
{
  env << "template <";
  int ct=0;
  for (TemplateParameter *iter = params; iter; iter = iter->next) {
    if (ct++ > 0) {
      env << "," << env.br;
    }
    iter->print(env);
  }
  env << ">" << env.fbr;

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
      env << inst->toQualifiedString() << ";    // decl but not defn" << env.fbr;
    }
  }
}

void TD_func::iprint(PrintEnv &env) const
{
  f->print(env);

  // print instantiations
  Variable *var = f->nameAndParams->var;
  if (var->isTemplate() &&      // for complete specializations, don't print
      !var->templateInfo()->isPartialInstantiation()) {     // nor partial inst
    env << env.br
        << "#if 0    // instantiations of ";
    // NOTE: inlined from Variable::toCString()

    TypeLike const *type0 = env.m_typePrinter.getVariableTypeLike(var);
    env.ptype(type0, (var->name? var->name : "/*anon*/"));
    env << var->namePrintSuffix() << env.fbr;
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

    env << env.br
        << "#endif   // instantiations of " << var->name;
  }
}

void TD_decl::iprint(PrintEnv &env) const
{
  d->print(env);

  // print instantiations
  if (d->spec->isTS_classSpec()) {
    CompoundType *ct = d->spec->asTS_classSpec()->ctype;
    TemplateInfo *ti = ct->typedefVar->templateInfo();
    if (!ti->isCompleteSpec()) {
      env << "#if 0    // instantiations of " << ct->name << env.fbr;

      SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
        Variable const *instV = iter.data();

        env << "// ";
        TypeLike const *type0 = env.m_typePrinter.getVariableTypeLike(instV);
        env.ptype(type0);
        CompoundType *instCT = instV->type->asCompoundType();
        if (instCT->syntax) {
          env << env.fbr;
          instCT->syntax->print(env);
          env << ";" << env.fbr;
        }
        else {
          env << ";     // body not instantiated" << env.fbr;
        }
      }
      env << "#endif   // instantiations of " << ct->name << env.fbr << env.fbr;
    }
  }
  else {
    // it could be a forward declaration of a template class;
    // do nothing more
  }
}

void TD_tmember::iprint(PrintEnv &env) const
{
  d->print(env);
}


// ------------------- TemplateParameter ------------------
void TP_type::print(PrintEnv &env) const
{
  env << "class " << (name? name : "/*anon*/");

  if (defaultType) {
    env << " = ";
    defaultType->print(env);
  }
}

void TP_nontype::print(PrintEnv &env) const
{
  param->print(env);
}


// -------------------- TemplateArgument ------------------
void TA_type::print(PrintEnv &env) const
{
  // dig down to prevent printing "/*anon*/" since template
  // type arguments are always anonymous so it's just clutter
  env.ptype(type->decl->var->type);
}

void TA_nontype::print(PrintEnv &env) const
{
  expr->print(env, OPREC_RELATIONAL /* use parens if '<' or '>' */);
}

void TA_templateUsed::print(PrintEnv &env) const
{
  // the caller should have recognized the presence of TA_templateUsed,
  // adjusted its printing accordingly, and then skipped this element
  xfailure("do not print TA_templateUsed");
}


// -------------------- NamespaceDecl ---------------------
void ND_alias::print(PrintEnv &env) const
{
  env << "namespace " << alias << " = ";
  original->print(env);
  env << ";" << env.fbr;
}

void ND_usingDecl::print(PrintEnv &env) const
{
  env << "using ";
  name->print(env);
  env << ";" << env.fbr;
}

void ND_usingDir::print(PrintEnv &env) const
{
  env << "using namespace ";
  name->print(env);
  env << ";" << env.fbr;
}


// EOF
