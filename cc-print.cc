// cc-print.cc            see license.txt for copyright and terms of use
// code for cc-print.h

// Adapted from cc-tcheck.cc by Daniel Wilkerson dsw@cs.berkeley.edu

#include "cc-print.h"                  // this module

// elsa
#include "cc-lang.h"                   // CCLang
#include "strip-comments.h"            // stripComments

// smbase
#include "sm-macros.h"                 // NULLABLE
#include "trace.h"                     // trace

// libc++
#include <sstream>                     // std::ostringstream


// This is a dummy global so that this file will compile both in
// default mode and in qualifiers mode.
class dummyType;                // This does nothing.
dummyType *ql;
string toString(class dummyType*) {return "";}


// **************** class PrintEnv

string PrintEnv::getResult()
{
  // Dumping the tree can be useful for diagnosing bad formatting.
  if (tracingSys("dumpPrintEnvTree")) {
    debugPrintCout();
  }

  // Make sure we didn't forget to close something.
  xassert(allSequencesClosed());

  // Print the formatted tree to 'oss'.
  std::ostringstream oss;
  prettyPrint(oss);

  // Copy the std::string to my string.
  return string(oss.str().c_str());
}


string PrintEnv::commentary(string const &info, char const *after) const
{
  if (m_printComments) {
    return stringb("/*" << stripComments(info) << "*/" << after);
  }
  else {
    return string("");
  }
}


char const *PrintEnv::possiblyAnonName(StringRef name) const
{
  if (name) {
    return name;
  }
  else if (m_printComments) {
    return "/*anon*/";
  }
  else {
    return "";
  }
}


char const *PrintEnv::asmKeywordSpelling() const
{
  if (m_lang.isCplusplus) {
    // In C++, 'asm' is a reserved keyword (C++14 2.11/1), and
    // asm-definition is part of the core language.
    return "asm";
  }
  else {
    // In C, it is not a keyword (C11 6.4.1/1).  Instead, C11 J.5.10
    // explains that 'asm' is a commonly available extension.  GCC does
    // not recognize it in standard-conforming mode (like "-std=c99"),
    // so we use its extension keyword instead.
    return "__asm__";
  }
}


void PrintEnv::ptype(Type const *type, char const *name)
{
  *this << m_typePrinter.printType(type, name);
}


void PrintEnv::iprintStatement(
  Statement const *stmt, StatementContext context)
{
  stmt->iprint(*this, context);
}


void PrintEnv::iprintExpression(Expression const *expr)
{
  expr->iprint(*this);
}


Initializer const * NULLABLE PrintEnv::selectInitializer(
  Initializer const * NULLABLE syntacticInit,
  SemanticInitializer const * NULLABLE semanticInit)
{
  if (m_printSemanticInitializers && semanticInit) {
    return semanticInit;
  }
  else {
    return syntacticInit;
  }
}


string printTypeToString(CCLang const &lang, Type const *type)
{
  CTypePrinter typePrinter(lang);
  PrintEnv env(typePrinter, lang);
  env.ptype(type, "" /*do not print "anon"*/);
  return env.getResult();
}


string printStatementToString(
  CCLang const &lang, Statement const *stmt, StatementContext context,
  bool printComments)
{
  CTypePrinter typePrinter(lang);
  PrintEnv env(typePrinter, lang);
  env.m_printComments = printComments;
  stmt->print(env, context);
  return env.getResult();
}


string printExpressionToString(
  CCLang const &lang, Expression const *expr, OperatorPrecedence prec,
  bool printComments)
{
  CTypePrinter typePrinter(lang);
  PrintEnv env(typePrinter, lang);
  env.m_printComments = printComments;
  expr->print(env, prec);
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
  class TPBeginEndSequence {
    TreePrint &m_tp;
  public:
    TPBeginEndSequence(TreePrint &tp, bool consistent)
      : m_tp(tp)
    {
      if (consistent) {
        m_tp.beginConsistent();
      }
      else {
        m_tp.begin();
      }
    }
    ~TPBeginEndSequence()
    {
      m_tp.end();
    }
  };

  // Begin/end vertical or sequence boxes.
  #define TPSEQUENCE TPBeginEndSequence bpBeginEndBox(env, false /*consistent*/)
  #define TP_H_OR_V  TPBeginEndSequence bpBeginEndBox(env, true /*consistent*/)

  // Print opening and closing braces.
  class TPBraces {
    PrintEnv &m_env;
  public:
    TPBraces(PrintEnv &env)
      : m_env(env)
    {
      m_env.begin();
      m_env << "{" << m_env.br;
    }

    ~TPBraces()
    {
      m_env.end();
      m_env << "}" << m_env.br;
    }
  };

  #define TPBRACES TPBraces bpBraces(env)
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
  TPBRACES;
  forms->print(env);
}

void TF_one_linkage::print(PrintEnv &env) const
{
  env << "extern " << linkageType << " ";
  form->print(env);
}

void TF_asm::print(PrintEnv &env) const
{
  ad->print(env);
}

void TF_namespaceDefn::print(PrintEnv &env) const
{
  env << "namespace " << env.possiblyAnonName(name) << " ";
  TPBRACES;
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
  PrintEnv &env, TypeSpecifier *spec, Declarator *declarator,
  bool inDeclaration)
{
  if (declaratorIsConstructor(declarator)) {
    // Print the type specifier,  The return type of a constructor says
    // what we build, hence the name of the class.
    spec->print(env, inDeclaration);

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
    spec->print(env, inDeclaration);
    if (ideclaratorWantsSpace(spec, declarator->decl)) {
      env << " ";
    }
    declarator->print(env);
  }
}

void Function::print(PrintEnv &env, DeclFlags declFlagsMask) const
{
  {
    TPSEQUENCE;

    printDeclFlags(env, dflags & declFlagsMask);
    printTypeSpecifierAndDeclarator(env, retspec, nameAndParams,
                                    true /*inDeclaration*/);
  }

  if (instButNotTchecked()) {
    // this is an unchecked instantiation
    env << "; // not instantiated" << env.br;
    return;
  }

  env << env.br;
  if (handlers) {
    env << "try" << env.br;
  }

  if (inits) {
    env << "  : ";
    env.begin(0 /*indent*/);

    int ct=0;      // Number of initializers printed.
    FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
      if (ct > 0) {
        env << "," << env.br;
      }

      env << iter->name->toString();
      printArgExprListWithParens(env, iter->args);
      ct++;
    }

    env.end();
    env << env.br;
  }

  if (body->stmts.isEmpty()) {
    // more concise
    env << "{}";
  }
  else {
    TPSEQUENCE;
    body->print(env, SC_FUNCTION_BODY);
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

// --------------------------- Declaration -----------------------------
void Declaration::print(PrintEnv &env) const
{
  // Check if the type specifier wants to print vertically.  I cannot
  // wait until I am inside 'spec->print()' because the declaration
  // flags get printed first.
  //
  // TODO: Needed?
  //bool isClassOrEnum = spec->isTS_classSpec() || spec->isTS_enumSpec();

  if (fl_count(decllist) == 1) {
    // If there is only one declarator, we will wrap with indentation
    // back to where the type specifier started.
    TP_H_OR_V;

    printDeclFlags(env, dflags);
    spec->print(env, true /*inDeclaration*/);
    Declarator *declarator = fl_first(decllist);
    if (ideclaratorWantsSpace(spec, declarator->decl)) {
      env << " ";
    }
    declarator->print(env);
    env << ";";
    return;
  }

  // Sequence for the entire declaration, encompassing any breaks in
  // the type specifier.
  TPSEQUENCE;

  printDeclFlags(env, dflags);

  spec->print(env, true /*inDeclaration*/);

  // Print the space that follows the specifier before starting the
  // first declarator so breaks within the declarator list return to
  // the column after this space.
  if (fl_isNotEmpty(decllist) &&
      ideclaratorWantsSpace(spec, fl_first(decllist)->decl)) {
    env << " ";
  }

  {
    // Enclose the declarator list in its own sequence, breaking lines
    // after every declarator if we do so after any.  But don't indent
    // when wrapping so all of the declarators are lined up vertically.
    // See example pprint/long-decl.c.
    env.begin(0 /*indent*/, true /*consistentBreaks*/);

    FAKELIST_FOREACH_NC(Declarator, decllist, declarator) {
      if (declarator != fl_first(decllist)) {
        env << "," << env.sp;
      }

      // If we break after the '=' before an initializer, we will wrap to
      // a point indented relative to the start of the declarator.
      TP_H_OR_V;

      declarator->print(env);
    }
    env << ";";

    env.end();
  }
}


// -------------------- ASTTypeId -------------------
void ASTTypeId::print(PrintEnv &env) const
{
  printTypeSpecifierAndDeclarator(env, spec, decl,
                                  false /*inDeclaration*/);
}


// ---------------------- PQName -------------------
void printTemplateArgumentList
  (PrintEnv &env, /*fakelist*/TemplateArgument *args)
{
  int ct=0;
  while (args) {
    if (!args->isTA_templateUsed()) {
      if (ct++ > 0) {
        env << ',' << env.sp;
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
void TypeSpecifier::print(PrintEnv &env, bool inDeclaration) const
{
  // Declarations already take care of making sequences as desired, but
  // for type specifiers outside declarations, particularly for
  // parameter lists, I want to have a sequence so the forced break in
  // the list doesn't extend down to the breaks within the specifier.
  if (!inDeclaration) {
    env.begin();
  }

  // Let extension modules have a chance to print things.
  ext_preprint(env);

  iprint(env);
  if (cv) {
    env << " " << toString(cv);
  }

  if (!inDeclaration) {
    env.end();
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
  ext_printAfterClassKey(env);
  name->print(env);
}


void TS_classSpec::iprint(PrintEnv &env) const
{
  env << toString(ql);          // see string toString(class dummyType*) above
  env << toString(keyword) << ' ';
  ext_printAfterClassKey(env);

  if (name) {
    env << name->toString();
  }
  else if (ctype &&     // Tolerate syntax that hasn't been tchecked.
           ctype->m_isAnonymousCompound) {
    env << "/*anonymous compound member*/";
  }

  if (fl_isNotEmpty(bases)) {
    TPSEQUENCE;

    bool first_time = true;
    FAKELIST_FOREACH_NC(BaseClassSpec, bases, iter) {
      if (first_time) {
        env << ' ' << ':' << ' ';
        first_time = false;
      }
      else {
        env << ',' << env.sp;
      }
      iter->print(env);
    }
  }

  // TODO: Members are printed without regard to their access control
  // status.  This is particularly a problem for compiler-generated
  // functions, which will simply be appended to the member list even
  // if that gives them the wrong access.

  env << " {" << env.br;

  env.begin(0);
  FOREACH_ASTLIST(Member, members->list, iter2) {
    iter2.data()->print(env);
    env << env.br;
  }
  env.end();

  env << env.und << '}';
}


void TS_enumSpec::iprint(PrintEnv &env) const
{
  TPSEQUENCE;

  env << toString(ql);          // see string toString(class dummyType*) above
  env << "enum ";
  ext_printAfterClassKey(env);
  if (name) {
    env << name << ' ';
  }
  env << '{' << env.br;

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
    env << env.br;
  }
  env << env.und << '}';
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
  env << env.und << toString(k) << ":";
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
// Return true if the printed form of 'init' begins with a left-brace.
static bool beginsWithLeftBrace(Initializer const *init)
{
  switch (init->kind()) {
    default:
      return false;

    case Initializer::IN_COMPOUND:
    case Initializer::SIN_STRINGLIT:
    case Initializer::SIN_ARRAY:
    case Initializer::SIN_STRUCT:
    case Initializer::SIN_UNION:
      return true;
  }
}


void Declarator::print(PrintEnv &env) const
{
  ext_pre_print(env);
  decl->print(env);
  ext_post_print(env);

  if (Initializer const *selInit =
        env.selectInitializer(init, m_semanticInit)) {
    if (!selInit->isIN_ctor()) {
      env << " =";
      if (beginsWithLeftBrace(selInit)) {
        // No break before the open brace.
        env << " ";
      }
      else {
        // Optional break after '='.
        env << env.sp;
      }
    }

    bool const outermost = true;
    selInit->print(env, outermost);
  }
}


// -------------------- IDeclarator --------------------
void D_name::print(PrintEnv &env) const
{
  if (name) {
    name->print(env);
  }

  ext_print(env);
}


void D_pointer::print(PrintEnv &env) const
{
  env << "*";
  if (cv) {
    env << " " << toString(cv) << " ";
  }
  ext_print(env);
  base->print(env);
}


void D_reference::print(PrintEnv &env) const
{
  env << "&";
  ext_print(env);
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
void IDeclarator::printBaseDeclaratorOfSuffixDeclarator(
  PrintEnv &env, IDeclarator const *base) const
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
      env << "," << env.sp;
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

  ext_print(env);
}


void D_array::print(PrintEnv &env) const
{
  printBaseDeclaratorOfSuffixDeclarator(env, base);

  env << "[";
  if (size) {
    size->print(env, OPREC_LOWEST);
  }
  env << "]";

  ext_print(env);
}


void D_bitfield::print(PrintEnv &env) const
{
  if (name) {
    name->print(env);
  }

  env << " : ";

  bits->print(env, OPREC_LOWEST);

  ext_print(env);
}


void D_ptrToMember::print(PrintEnv &env) const
{
  nestedName->print(env);
  env << "::*";
  if (cv) {
    env << " " << toString(cv) << " ";
  }
  ext_print(env);
  base->print(env);
}


void D_grouping::print(PrintEnv &env) const
{
  // Like with E_grouping, we will drop the parens here, and instead
  // supply them automatically where needed.
  ext_print(env);
  base->print(env);
}


// ------------------- ExceptionSpec --------------------
void ExceptionSpec::print(PrintEnv &env) const
{
  env << "throw(";
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    if (iter != fl_first(types)) {
      env << "," << env.sp;
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
void Statement::print(PrintEnv &env, StatementContext context) const
{
  env.iprintStatement(this, context);
}

// no-op
void S_skip::iprint(PrintEnv &env, StatementContext) const
{
  env << ";";
}

void S_label::iprint(PrintEnv &env, StatementContext context) const
{
  env << env.und << name << ":";
  ext_print(env, context);
  env << env.br;
  s->print(env, SC_LABEL);
}

void S_case::iprint(PrintEnv &env, StatementContext) const
{
  env << env.und << "case ";
  expr->print(env, OPREC_LOWEST);
  env << ":" << env.br;

  s->print(env, SC_CASE);
}

void S_default::iprint(PrintEnv &env, StatementContext) const
{
  // The colon is printed separately so PrintEnv::lastStringIs(":") will
  // be true afterward, thus allowing that test to recognize being after
  // a 'case' and after a 'default' with one test.
  env << env.und << "default" << ":" << env.br;
  s->print(env, SC_DEFAULT);
}

void S_expr::iprint(PrintEnv &env, StatementContext) const
{
  expr->print(env);
  env << ";";
}

void S_compound::iprint(PrintEnv &env, StatementContext context) const
{
  // When the parent is an S_compound or a label statement, we make our
  // own sequence.  Other forms do their own sequence creation a bit
  // earlier so we wrap to indent under their starting points.
  bool makeSequence = (context == SC_COMPOUND ||
                       context == SC_LABEL ||
                       context == SC_CASE ||
                       context == SC_DEFAULT);
  if (makeSequence) {
    env.begin();
  }

  env << "{" << env.br;

  FOREACH_ASTLIST(Statement, stmts, iter) {
    Statement const *stmt = iter.data();
    stmt->print(env, SC_COMPOUND);
    env << env.br;
  }

  if (context == SC_SWITCH) {
    // We need an extra unindent.
    env << env.und;
  }
  env << env.und << "}";

  if (makeSequence) {
    env.end();
  }
}

void S_if::iprint(PrintEnv &env, StatementContext) const
{
  {
    TPSEQUENCE;
    env << "if (";
    cond->print(env);
    env << ") ";
    thenBranch->print(env, SC_IF_THEN);
  }

  if (S_compound const *comp = elseBranch->ifS_compoundC()) {
    if (comp->m_implicit && comp->stmts.count() == 1) {
      if (S_skip const *skip = comp->stmts.firstC()->ifS_skipC()) {
        if (skip->m_implicit) {
          // The 'else' branch is implicit, and consists only of an
          // implicit skip statement.  Do not print it at all.
          return;
        }
      }
    }
  }

  if (S_skip const *skip = elseBranch->ifS_skipC()) {
    if (skip->m_implicit) {
      // Similar but without the surrounding compound.
      return;
    }
  }

  env << env.br;

  {
    TPSEQUENCE;
    env << "else ";
    elseBranch->print(env, SC_IF_ELSE);
  }
}

void S_switch::iprint(PrintEnv &env, StatementContext) const
{
  // In the common case of an S_compound, use my preferred style where
  // labels are indented one level and everything else is indented two
  // levels.
  env.begin(env.INDENT_SPACES * (branches->isS_compound()? 2 : 1));

  env << "switch (";
  cond->print(env);
  env << ") ";

  branches->print(env, SC_SWITCH);
  env.end();
}

void S_while::iprint(PrintEnv &env, StatementContext) const
{
  TPSEQUENCE;

  env << "while (";
  cond->print(env);
  env << ") ";
  body->print(env, SC_WHILE);
}

void S_doWhile::iprint(PrintEnv &env, StatementContext) const
{
  TPSEQUENCE;

  env << "do ";
  body->print(env, SC_DO_WHILE);
  env << " while (";
  expr->print(env);
  env << ");";
}

void S_for::iprint(PrintEnv &env, StatementContext) const
{
  TPSEQUENCE;

  env << "for (";
  {
    TPSEQUENCE;

    init->print(env, SC_FOR_INIT);
    if (init->isS_skip()) {
      // If there is no initializer, then do not break the line after
      // the semicolon.
      env << " ";
    }
    else {
      env << env.sp;
    }
    cond->print(env);
    env << ";";
    env << env.sp;
    after->print(env);
  }
  env << ") ";
  body->print(env, SC_FOR_BODY);
}

void S_break::iprint(PrintEnv &env, StatementContext) const
{
  env << "break;";
}

void S_continue::iprint(PrintEnv &env, StatementContext) const
{
  env << "continue;";
}

void S_return::iprint(PrintEnv &env, StatementContext) const
{
  env << "return";
  if (expr) {
    env << " ";
    expr->print(env);
  }
  env << ";";
}

void S_goto::iprint(PrintEnv &env, StatementContext) const
{
  env << "goto ";
  env << target;
  env << ";";
}

void S_decl::iprint(PrintEnv &env, StatementContext) const
{
  decl->print(env);
}

void S_try::iprint(PrintEnv &env, StatementContext) const
{
  env << "try";
  body->print(env, SC_TRY);
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->print(env);
  }
}

void S_asm::iprint(PrintEnv &env, StatementContext) const
{
  ad->print(env);
}

void S_namespaceDecl::iprint(PrintEnv &env, StatementContext) const
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
  TPSEQUENCE;

  env << "catch (";
  if (isEllipsis()) {
    env << "...";
  }
  else {
    typeId->print(env);
  }
  env << ") ";
  body->print(env, SC_HANDLER);
}


// -------------------------- AsmDefinition ----------------------------
void AD_string::print(PrintEnv &env) const
{
  env << env.asmKeywordSpelling() << "(";

  // It is common for the string literal to have multiple elements,
  // and those should be printed so they line up in a column.
  env.begin(0 /*indent*/, true /*consistentBreaks*/);

  text->print(env, OPREC_LOWEST);

  env.end();

  env << ");";
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

  if (thisPrec < parentPrec) {
    // 'this' expression has higher precedence than its parent, so does
    // not need parentheses.
    env.iprintExpression(this);
  }
  else if (thisPrec==OPREC_POSTFIX && parentPrec==OPREC_POSTFIX) {
    // Although they both have the same precedence, postfix operators
    // can only associate one way, so no parens are needed.
    env.iprintExpression(this);
  }
  else {
    // Parens may be needed.
    env << "(";
    env.iprintExpression(this);
    env << ")";
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
  PrintEnv env(typePrinter, lang);

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
      env << env.sp;
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
      env << stringc << env.commentary("int", " ") << sta->value.i;
      break;
    case STemplateArgument::STA_ENUMERATOR:
      env << stringc << env.commentary("enum", " ") << sta->value.v->name;
      break;
    case STemplateArgument::STA_REFERENCE:
      env << stringc << env.commentary("ref", " ") << sta->value.v->name;
      break;
    case STemplateArgument::STA_POINTER:
      env << stringc << env.commentary("ptr", " ") << "&" << sta->value.v->name;
      break;
    case STemplateArgument::STA_MEMBER:
      env << stringc
              << env.commentary("member", " ")
              << "&"
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
      env << ',' << env.sp;
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
  int ct=0;
  FAKELIST_FOREACH_NC(ArgExpression, list, iter) {
    if (ct > 0) {
      env << ',' << env.sp;
    }
    iter->expr->print(env, OPREC_COMMA);
    ct++;
  }
}

static void printArgExprListWithParens(PrintEnv &env,
                                       FakeList<ArgExpression> *list)
{
  env << "(" << env.optbr;
  printArgExprList(env, list);
  env << ")";
}

void E_funCall::iprint(PrintEnv &env) const
{
  TPSEQUENCE;

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
  // Skip ISCs since this logic is written for the case where ISCs are
  // not printed.
  if (e->isE_implicitStandardConversion()) {
    e = e->asE_implicitStandardConversionC()->expr;
  }

  return e->isE_boolLit() ||
         e->isE_intLit() ||
         e->isE_floatLit() ||
         e->isE_charLit() ||
         e->isE_this() ||
         e->isE_variable();
}


void E_binary::iprint(PrintEnv &env) const
{
  TPSEQUENCE;

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
      env << env.sp;
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
    env.iprintExpression(expr);
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
  TP_H_OR_V;

  cond->print(env, this->getPrecedence());

  if (th) {
    env << "?" << env.sp;
    th->print(env, this->getPrecedence());
    env << " :" << env.sp;
  }
  else {
    // GNU binary conditional.
    env << " ?:" << env.sp;
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
  TPSEQUENCE;

  target->print(env, this->getPrecedence());

  env << " ";
  if (op != BIN_ASSIGN) {
    env << toString(op);
  }
  env << '=' << env.sp;

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
  if (env.m_printISC) {
    TPSEQUENCE;
    env << "ISC(" << env.optbr
        << toString(conv) << ',' << env.sp
        << type->toString() << ',' << env.sp;
    expr->print(env, OPREC_COMMA);
    env << ')';
  }
  else {
    env.iprintExpression(expr);
  }
}

OperatorPrecedence E_implicitStandardConversion::getPrecedence() const
{
  // Behave transparently w.r.t. printing.
  return expr->getPrecedence();
}


void E_offsetof::iprint(PrintEnv &env) const
{
  // I print using the GNU syntax even though E_offsetof is now a
  // part of the core AST.  That may need to change at some point.
  env << "__builtin_offsetof(";

  env.begin(0 /*indent*/);

  atype->print(env);
  env << "," << env.sp;
  fieldName->print(env);

  env.end();

  env << ")";
}

OperatorPrecedence E_offsetof::getPrecedence() const
{
  return OPREC_POSTFIX;
}


// ----------------------- Initializer --------------------

// this is under a declaration
// int x = 3;
//         ^ only
void IN_expr::print(PrintEnv &env, bool) const
{
  e->print(env, OPREC_ASSIGN);
}


static void beginInitializerBraces(PrintEnv &env, bool outermost)
{
  if (outermost) {
    // There will already be a sequence started where the type
    // specifier is.  Do not start a new sequence at the brace,
    // because that would cause too much indentation.
    env << '{' << env.sp;

    // But, do start a sequence after the brace so that all of the
    // data inside can be broken or not, independent of the breaks
    // at the braces.
    env.begin(0);
  }

  else {
    // For non-outermost, wrap the entire thing in a sequence.
    env.begin();
    env << '{' << env.sp;
  }
}


static void endInitializerBraces(PrintEnv &env, bool outermost)
{
  if (outermost) {
    // Close the inner sequence first.
    env.end();

    // Then add the final brace as part of the outer sequence.
    env << env.sp << env.und << '}';
  }

  else {
    // Put everything inside the one sequence.
    env << env.sp << env.und << '}';
    env.end();
  }
}


// int x[] = {1, 2, 3};
//           ^^^^^^^^^ only
void IN_compound::print(PrintEnv &env, bool outermost) const
{
  beginInitializerBraces(env, outermost);

  {
    bool first_time = true;
    FOREACH_ASTLIST(Initializer, inits, iter) {
      if (first_time) {
        first_time = false;
      }
      else {
        env << "," << env.sp;
      }
      iter.data()->print(env, false /*outermost*/);
    }
  }

  endInitializerBraces(env, outermost);
}


void IN_ctor::print(PrintEnv &env, bool) const
{
  if (fl_isEmpty(args)) {
    // Don't print "()" in an empty IN_ctor initializer (C++98 8.5p8).
  }
  else {
    // Constructor arguments.
    env << "(";
    printArgExprList(env, args);
    env << ")";
  }
}


void SIN_stringLit::print(PrintEnv &env, bool) const
{
  m_stringLit->print(env, OPREC_ASSIGN);
}


// Print syntax to zero-initialize a value of 'type'.
static void printZeroInitForType(PrintEnv &env, Type const *type)
{
  if (type->isArrayType()) {
    // Although an empty brace pair is not allowed by the C11
    // standard, there isn't any other way to explicitly indicate zero
    // initialization, especially with GNU extensions that allow
    // structs and arrays to be empty.
    env << "{}";
  }
  else if (CompoundType const *ct = type->ifCompoundTypeC()) {
    if (ct->isAggregate()) {
      env << "{}";
    }
    else {
      // This is questionable.  I might need qualifiers at least.
      env << ct->name << "()";
    }
  }
  else {
    // For anything else, let's hope this suffices.
    env << "0";
  }
}


void SIN_array::print(PrintEnv &env, bool outermost) const
{
  beginInitializerBraces(env, outermost);

  bool first = true;
  for (SemanticInitializer const * NULLABLE si : m_elements) {
    if (first) {
      first = false;
    }
    else {
      env << "," << env.sp;
    }

    if (si) {
      si->print(env, false /*outermost*/);
    }
    else {
      printZeroInitForType(env, m_arrayType->eltType);
    }
  }

  endInitializerBraces(env, outermost);
}


void SIN_struct::print(PrintEnv &env, bool outermost) const
{
  beginInitializerBraces(env, outermost);

  for (size_t index = 0; index < m_members.size(); ++index) {
    SemanticInitializer const * NULLABLE si = m_members[index];
    Variable const *field = m_compoundType->getDataMemberByPositionC(index);

    if (index > 0) {
      env << "," << env.sp;
    }

    if (si) {
      si->print(env, false /*outermost*/);
    }
    else {
      printZeroInitForType(env, field->type);
    }
  }

  endInitializerBraces(env, outermost);
}


void SIN_union::print(PrintEnv &env, bool outermost) const
{
  beginInitializerBraces(env, outermost);

  if (m_initMember != m_unionType->dataMembers.firstC()) {
    // Print a designator to initialize a non-first member.
    env << "." << m_initMember->name << " = ";
  }

  m_initValue->print(env, false /*outermost*/);

  endInitializerBraces(env, outermost);
}


// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::print(PrintEnv &env) const
{
  env << "template <";
  int ct=0;
  for (TemplateParameter *iter = params; iter; iter = iter->next) {
    if (ct++ > 0) {
      env << "," << env.sp;
    }
    iter->print(env);
  }
  env << ">" << env.br;

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
      env << inst->toQualifiedString() << ";    // decl but not defn" << env.br;
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
    env.ptype(type0, env.possiblyAnonName(var->name));
    env << var->namePrintSuffix() << env.br;
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
      env << "#if 0    // instantiations of " << ct->name << env.br;

      SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
        Variable const *instV = iter.data();

        env << "// ";
        TypeLike const *type0 = env.m_typePrinter.getVariableTypeLike(instV);
        env.ptype(type0);
        CompoundType *instCT = instV->type->asCompoundType();
        if (instCT->syntax) {
          env << env.br;
          instCT->syntax->print(env, true /*inDeclaration*/);
          env << ";" << env.br;
        }
        else {
          env << ";     // body not instantiated" << env.br;
        }
      }
      env << "#endif   // instantiations of " << ct->name << env.br << env.br;
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
  env << "class " << env.possiblyAnonName(name);

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
  env << ";" << env.br;
}

void ND_usingDecl::print(PrintEnv &env) const
{
  env << "using ";
  name->print(env);
  env << ";" << env.br;
}

void ND_usingDir::print(PrintEnv &env) const
{
  env << "using namespace ";
  name->print(env);
  env << ";" << env.br;
}


// EOF
