// elsaparse.cc          see license.txt for copyright and terms of use
// Code for elsaparse.h.

#include "elsaparse.h"                 // this module

// elsa
#include "cc.gr.gen.h"                 // CCParse
#include "cc-ast.h"                    // C++ AST (r)
#include "cc-ast-aux.h"                // class LoweredASTVisitor
#include "cc-elaborate.h"              // ElabVisitor
#include "cc-env.h"                    // Env
#include "cc-lang.h"                   // CCLang
#include "cc-print.h"                  // PrintEnv
#include "integrity.h"                 // IntegrityVisitor
#include "parssppt.h"                  // ParseTreeAndTokens, treeMain
#include "sprint.h"                    // structurePrint

// elkhound
#include "parsetables.h"               // ParseTables
#include "ptreeact.h"                  // ParseTreeLexer, ParseTreeActions
#include "ptreenode.h"                 // PTreeNode

// smbase
#include "exc.h"                       // smbase::XBase
#include "gcc-options.h"               // gccLanguageForFile
#include "nonport.h"                   // getMilliseconds
#include "sm-fstream.h"                // ofstream
#include "sm-iostream.h"               // cerr
#include "smregexp.h"                  // regexpMatch
#include "srcloc.h"                    // SourceLocManager
#include "strtokp.h"                   // StrtokParse
#include "trace.h"                     // traceAddSys

// libc
#include <stdlib.h>                    // exit, getenv, abort

using namespace smbase;


// little check: is it true that only global declarators
// ever have Declarator::type != Declarator::var->type?
// .. no, because it's true for class members too ..
// .. it's also true of arrays declared [] but then inited ..
// .. finally, it's true of parameters whose types get
//    normalized as per C++98 8.3.5 para 3; I didn't include
//    that case below because there's no easy way to test for it ..
// Intended to be used with LoweredASTVisitor
class DeclTypeChecker : private ASTVisitor {
public:
  LoweredASTVisitor loweredVisitor; // use this as the argument for traverse()

  int instances;

public:
  DeclTypeChecker()
    : loweredVisitor(this)
    , instances(0)
  {}
  virtual ~DeclTypeChecker() {}

  virtual bool visitDeclarator(Declarator *obj) override;
};

bool DeclTypeChecker::visitDeclarator(Declarator *obj)
{
  if (obj->type != obj->var->type &&
      !obj->var->inGlobalScope() &&
      !obj->var->isClassMember() &&
      !obj->type->isArrayType()) {
    instances++;
    cerr << toString(obj->var->loc) << ": " << obj->var->name
         << " has type != var->type, but is not global or member or array\n";
  }
  return true;
}


// this scans the AST for E_variables, and writes down the locations
// to which they resolved; it's helpful for writing tests of name and
// overload resolution
class NameChecker : public ASTVisitor {
public:
  // accumulates the results
  stringBuilder sb;

public:
  NameChecker() {}

  virtual bool visitExpression(Expression *obj) override
  {
    Variable *v = NULL;
    if (obj->isE_variable()) {
      v = obj->asE_variable()->var;
    }
    else if (obj->isE_fieldAcc()) {
      v = obj->asE_fieldAcc()->field;
    }

    // this output format is designed to minimize the effect of
    // changes to unrelated details
    if (v
        && !streq("__testOverload", v->name)
        && !streq("dummy",          v->name)
        && !streq("__other",        v->name) // "__other": for inserted elaboration code
        && !streq("this",           v->name) // dsw: not sure why "this" is showing up
        && !streq("operator=",      v->name) // an implicitly defined member of every class
        && v->name[0]!='~'                   // don't print dtors
        ) {
      sb << " " << v->name << "=" << sourceLocManager->getLine(v->loc);
    }

    return true;
  }
};


class PrintStringLiteralsVisitor : public ASTVisitor {
public:
  virtual bool visitExpression(Expression *expr) override;
};

bool PrintStringLiteralsVisitor::visitExpression(Expression *expr)
{
  E_stringLit *lit = expr->ifE_stringLit();
  if (lit) {
    lit->m_stringData.print("decoded literal");
  }

  // Do not visit children.  The only children of an E_stringLit are
  // the continuations, and they have empty 'm_stringData'.
  return false;
}


class SectionTimer {
  long start;
  long &elapsed;

public:
  SectionTimer(long &e)
    : start(getMilliseconds()),
      elapsed(e)
  {}
  ~SectionTimer()
  {
    elapsed += getMilliseconds() - start;
  }
};


static void handle_XBase(Env &env, XBase &x, bool printWarnings)
{
  // typically an assertion failure from the tchecker; catch it here
  // so we can print the errors, and something about the location
  env.errors.print(cerr, printWarnings);
  cerr << x << endl;
  cerr << "Failure probably related to code near " << env.locStr() << endl;

  // print all the locations on the scope stack; this is sometimes
  // useful when the env.locStr refers to some template code that
  // was instantiated from somewhere else
  //
  // (unfortunately, env.instantiationLocStack isn't an option b/c
  // it will have been cleared by the automatic invocation of
  // destructors unwinding the stack...)
  cerr << "current location stack:\n";
  cerr << env.locationStackString();

  // I changed from using exit(4) here to using abort() because
  // that way the multitest.pl script can distinguish them; the
  // former is reserved for orderly exits, whereas signals (like
  // SIGABRT) mean that something went really wrong
  abort();
}


ElsaParse::ElsaParse(StringTable &stringTable_, CCLang &lang_)
  : m_stringTable(stringTable_),
    m_typeFactory(),
    m_lang(lang_),
    m_printErrorCount(false),
    m_printWarnings(true),
    m_prettyPrint(false),
    m_prettyPrintComments(true),
    m_prettyPrintISC(false),
    m_printStringLiterals(false),
    m_elabActivities(EA_ALL),
    m_translationUnit(NULL),
    m_mainFunction(NULL),
    m_parseTime(0),
    m_tcheckTime(0),
    m_integrityTime(0),
    m_elaborationTime(0),
    m_tcheckCompleted(false)
{}


ElsaParse::~ElsaParse()
{}


bool ElsaParse::setDashXLanguage(string const &lang)
{
  if (lang == "c" ||
      lang == "c-header" ||
      lang == "cpp-output") {
    m_lang.GNU_C();
    return true;
  }
  else if (lang == "c++" ||
           lang == "c++-cpp-output" ||
           lang == "c++-header") {
    m_lang.GNU_Cplusplus();
    return true;
  }
  else {
    // Leave the language alone.
    return false;
  }
}


bool ElsaParse::setDefaultLanguage(char const *inputFname)
{
  return setDashXLanguage(gccLanguageForFile(inputFname, ""));
}


bool ElsaParse::parse(char const *inputFname)
{
  // dsw: I needed this to persist past typechecking, so I moved it
  // out here.  Feel free to refactor.
  ArrayStack<Variable*> madeUpVariables;
  ArrayStack<Variable*> builtinVars;

  int parseWarnings = 0;
  {
    SectionTimer timer(m_parseTime);
    SemanticValue treeTop;
    ParseTreeAndTokens tree(m_lang, treeTop, m_stringTable, inputFname);

    // grab the lexer so we can check it for errors (damn this
    // 'tree' thing is stupid..)
    Lexer *lexer = dynamic_cast<Lexer*>(tree.lexer);
    xassert(lexer);

    CCParse *parseContext = new CCParse(m_stringTable, m_lang);
    tree.userAct = parseContext;

    traceProgress(2) << "building parse tables from internal data\n";
    ParseTables *tables = parseContext->makeTables();
    tree.tables = tables;

    maybeUseTrivialActions(tree);

    if (tracingSys("parseTree")) {
      // make some helpful aliases
      LexerInterface *underLexer = tree.lexer;
      UserActions *underAct = parseContext;

      // replace the lexer and parser with parse-tree-building versions
      tree.lexer = new ParseTreeLexer(underLexer, underAct);
      tree.userAct = new ParseTreeActions(underAct, tables);

      // 'underLexer' and 'tree.userAct' will be leaked.. oh well
    }

    if (!toplevelParse(tree, inputFname)) {
      xfatal("parse error");
    }

    // check for parse errors detected by the context class
    if (parseContext->errors || lexer->errors) {
      xfatal("parse error");
    }
    parseWarnings = lexer->warnings + parseContext->warnings;

    if (tracingSys("parseTree")) {
      // the 'treeTop' is actually a PTreeNode pointer; print the
      // tree and bail
      PTreeNode *ptn = (PTreeNode*)treeTop;
      ptn->printTree(cout, PTreeNode::PF_EXPAND);
      return true;
    }

    // treeTop is a TranslationUnit pointer
    m_translationUnit = (TranslationUnit*)treeTop;

    //m_translationUnit->debugPrint(cout, 0);

    delete parseContext;
    delete tables;
  }

  // print abstract syntax tree
  if (tracingSys("printAST")) {
    m_translationUnit->debugPrint(cout, 0);
  }

  //if (m_translationUnit) {     // when "-tr trivialActions" it's NULL...
  //  cout << "ambiguous nodes: " << numAmbiguousNodes(m_translationUnit) << endl;
  //}

  if (tracingSys("stopAfterParse")) {
    return true;
  }


  // ---------------- typecheck -----------------
  if (tracingSys("no-typecheck")) {
    cerr << "no-typecheck" << endl;
  } else {
    SectionTimer timer(m_tcheckTime);
    Env env(m_stringTable, m_lang, m_typeFactory, madeUpVariables, builtinVars, m_translationUnit);
    try {
      env.tcheckTranslationUnit(m_translationUnit);
      m_tcheckCompleted = true;
    }
    catch (XUnimp &x) {
      HANDLER();

      // relay to handler in main()
      cerr << "in code near " << env.locStr() << ":\n";
      throw;
    }
    catch (XAssert &x) {
      HANDLER();

      if (env.errors.hasFromNonDisambErrors()) {
        if (tracingSys("expect_confused_bail")) {
          cerr << "got the expected confused/bail\n";
          exit(0);
        }

        // The assertion failed only after we encountered and diagnosed
        // at least one real error in the input file.  Therefore, the
        // assertion might simply have been caused by a failure of the
        // error recovery code to fully restore all invariants (which is
        // often difficult).  So, we'll admit to being "confused", but
        // not to the presence of a bug in Elsa (which is what a failed
        // assertion otherwise nominally means).
        //
        // The error message is borrowed from gcc; I wouldn't be
        // surprised to discover they use a similar technique to decide
        // when to emit the same message.
        //
        // The reason I do not put the assertion failure message into
        // this one is I don't want it showing up in the output where
        // Delta might see it.  If I am intending to minimize an assertion
        // failure, it's no good if Delta introduces an error.
        env.error("confused by earlier errors, bailing out");
        env.errors.print(cerr, m_printWarnings);
        exit(4);
      }

      if (tracingSys("expect_xfailure")) {
        cerr << "got the expected xfailure\n";
        exit(0);
      }

      // if we don't have a basis for reducing severity, pass this on
      // to the normal handler
      handle_XBase(env, x, m_printWarnings);
    }
    catch (XBase &x) {
      HANDLER();
      handle_XBase(env, x, m_printWarnings);
    }

    int numErrors = env.errors.numErrors();
    int numWarnings = env.errors.numWarnings() + parseWarnings;

    // do this now so that 'printTypedAST' will include CFG info
    #ifdef CFG_EXTENSION
    // A possible TODO is to do this right after each function is type
    // checked.  However, in the current design, that means physically
    // inserting code into Function::tcheck (ifdef'd of course).  A way
    // to do it better would be to have a general post-function-tcheck
    // interface that analyses could hook in to.  That could be augmented
    // by a parsing mode that parsed each function, analyzed it, and then
    // immediately discarded its AST.
    if (numErrors == 0) {
      numErrors += computeUnitCFG(m_translationUnit);
    }
    #endif // CFG_EXTENSION

    // print abstract syntax tree annotated with types
    if (tracingSys("printTypedAST")) {
      m_translationUnit->debugPrint(cout, 0);
    }

    // structural delta thing
    if (tracingSys("structure")) {
      structurePrint(m_translationUnit);
    }

    if (numErrors==0 && tracingSys("secondTcheck")) {
      // this is useful to measure the cost of disambiguation, since
      // now the tree is entirely free of ambiguities
      traceProgress() << "beginning second tcheck...\n";
      Env env2(m_stringTable, m_lang, m_typeFactory, madeUpVariables,
               builtinVars, m_translationUnit);
      m_translationUnit->tcheck(env2);
      traceProgress() << "end of second tcheck\n";
    }

    // print errors and warnings
    env.errors.print(cerr, m_printWarnings);

    if (m_printErrorCount) {
      cerr << "typechecking results:\n"
           << "  errors:   " << numErrors << "\n"
           << "  warnings: " << numWarnings << "\n";
    }

    if (numErrors != 0) {
      // This is the primary place this method signals an error to its
      // caller.
      return false;
    }

    // lookup diagnostic
    if (env.collectLookupResults.length()) {
      // scan AST
      NameChecker nc;
      nc.sb << "collectLookupResults";
      m_translationUnit->traverse(nc);

      // compare to given text
      if (streq(env.collectLookupResults, nc.sb)) {
        // ok
      }
      else {
        cerr << "collectLookupResults do not match:\n"
             << "  source: " << env.collectLookupResults << "\n"
             << "  tcheck: " << nc.sb << "\n"
             ;
        exit(4);
      }
    }

    // For the benefit of the interpreter, get 'main' if it exists.
    {
      StringRef mainName = m_stringTable.add("main");
      Variable *mainVar =
        env.globalScope()->lookupVariable(mainName, env);
      if (mainVar) {
        m_mainFunction = mainVar->funcDefn;
      }
    }
  }

  // ---------------- integrity checking ----------------
  {
    SectionTimer timer(m_integrityTime);

    integrityCheckTU(m_lang, m_translationUnit);

    // check that the AST is a tree *and* that the lowered AST is a
    // tree; only do this *after* confirming that tcheck finished
    // without errors
    if (tracingSys("treeCheck")) {
      long start = getMilliseconds();
      LoweredIsTreeVisitor treeCheckVisitor;
      m_translationUnit->traverse(treeCheckVisitor.loweredVisitor);
      traceProgress() << "done with tree check 1 ("
                      << (getMilliseconds() - start)
                      << " ms)\n";
    }

    // check an expected property of the annotated AST
    if (tracingSys("declTypeCheck") || getenv("declTypeCheck")) {
      DeclTypeChecker vis;
      m_translationUnit->traverse(vis.loweredVisitor);
      cerr << "instances of type != var->type: " << vis.instances << endl;
    }

    if (tracingSys("stopAfterTCheck")) {
      return true;
    }
  }

  // ----------------- elaboration ------------------
  if (tracingSys("no-elaborate")) {
    cerr << "no-elaborate" << endl;
  }
  else if (m_elabActivities == EA_NONE) {
    // No elaboration requested.
  }
  else {
    SectionTimer timer(m_elaborationTime);

    if (!m_lang.isCplusplus) {
      // Do at most the C elaboration activities.
      m_elabActivities &= EA_C_ACTIVITIES;
    }

    // If we are going to pretty print, then we need to retain defunct
    // children.
    if (m_prettyPrint) {
      m_elabActivities &= ~EA_REMOVE_DEFUNCT_CHILDREN;
    }

    // Configure the elaborator.
    ElabVisitor vis(m_stringTable, m_typeFactory, m_lang, m_translationUnit);
    vis.activities = m_elabActivities;

    // do elaboration
    m_translationUnit->traverse(vis.loweredVisitor);

    // print abstract syntax tree annotated with elaborated semantics
    if (tracingSys("printElabAST")) {
      m_translationUnit->debugPrint(cout, 0);
    }
    if (tracingSys("stopAfterElab")) {
      return true;
    }
  }

  // mark "real" (non-template) variables as such
  {
    MarkRealVars markReal;
    visitVarsF(builtinVars, markReal);
    visitRealVarsF(m_translationUnit, markReal);
  }

  // more integrity checking
  {
    SectionTimer timer(m_integrityTime);

    // Check AST integrity again after elaboration.
    integrityCheckTU(m_lang, m_translationUnit);

    // check that the AST is a tree *and* that the lowered AST is a
    // tree (do this *after* elaboration!)
    if (tracingSys("treeCheck")) {
      long start = getMilliseconds();
      LoweredIsTreeVisitor treeCheckVisitor;
      m_translationUnit->traverse(treeCheckVisitor.loweredVisitor);
      traceProgress() << "done with tree check 2 ("
                      << (getMilliseconds() - start)
                      << " ms)\n";
    }
  }

  if (m_printStringLiterals) {
    PrintStringLiteralsVisitor visitor;
    LoweredASTVisitor loweredVisitor(&visitor);
    m_translationUnit->traverse(loweredVisitor);
  }

  maybePrettyPrint();

  // test AST cloning
  if (tracingSys("testClone")) {
    TranslationUnit *u2 = m_translationUnit->clone();

    if (tracingSys("cloneAST")) {
      cout << "------- cloned AST --------\n";
      u2->debugPrint(cout, 0);
    }

    if (tracingSys("cloneCheck")) {
      ArrayStack<Variable*> madeUpVariables2;
      ArrayStack<Variable*> builtinVars2;
      // dsw: I hope you intend that I should use the cloned TranslationUnit
      Env env3(m_stringTable, m_lang, m_typeFactory, madeUpVariables2, builtinVars2, u2);
      u2->tcheck(env3);

      if (tracingSys("cloneTypedAST")) {
        cout << "------- cloned typed AST --------\n";
        u2->debugPrint(cout, 0);
      }

      if (tracingSys("clonePrint")) {
        CTypePrinter typePrinter(m_lang);
        PrintEnv penv(typePrinter, m_lang);
        cout << "---- cloned pretty print ----" << endl;
        u2->print(penv);
        cout << penv.getResult();
      }
    }
  }

  // test debugPrint but send the output to /dev/null (basically just
  // make sure it doesn't segfault or abort)
  if (tracingSys("testDebugPrint")) {
    ofstream devnull("/dev/null");
    m_translationUnit->debugPrint(devnull, 0);
  }

  return true;
}


void ElsaParse::maybePrettyPrint()
{
  if (m_prettyPrint) {
    CTypePrinter typePrinter(m_lang);
    PrintEnv env(typePrinter, m_lang);
    env.m_printComments = m_prettyPrintComments;
    env.m_printISC = m_prettyPrintISC;

    // TODO: Plumb a proper command line switch through to here.
    if (tracingSys("printSyntacticInitializers")) {
      env.m_printSemanticInitializers = false;
    }

    cout << "// ---- START ----" << endl;
    cout << "// -*- c++ -*-" << endl;
    m_translationUnit->print(env);
    if (tracingSys("dumpTreePrintTree")) {
      env.debugPrintCout();
    }
    cout << env.getResult();
    cout << "// ---- STOP ----" << endl;
  }
}


void ElsaParse::printTimes()
{
  cerr << "parse=" << m_parseTime << "ms"
       << " tcheck=" << m_tcheckTime << "ms"
       << " integ=" << m_integrityTime << "ms"
       << " elab=" << m_elaborationTime << "ms"
       << "\n"
       ;
}


Type *ElsaParse::getGlobalType(char const *typeName_) const
{
  StringRef typeName = m_stringTable.add(typeName_);

  Variable *var = m_translationUnit->globalScope->rawLookupVariable(typeName);
  if (!var) {
    xfailure(stringbc("not in the global scope: " << typeName));
  }
  if (!var->hasFlag(DF_TYPEDEF)) {
    xfailure(stringbc("not a type: " << typeName));
  }
  return var->type;
}


Variable *ElsaParse::getGlobalVar(char const *varName) const
{
  Variable *var = getGlobalVarOpt(varName);
  if (!var) {
    xfailure(stringbc("not in the global scope: " << varName));
  }
  return var;
}


Variable *ElsaParse::getGlobalVarOpt(char const *varName_) const
{
  StringRef varName = m_stringTable.add(varName_);

  return m_translationUnit->globalScope->rawLookupVariable(varName);
}


Variable *ElsaParse::getFieldOf(Variable *container,
                                char const *fieldName_) const
{
  StringRef fieldName = m_stringTable.add(fieldName_);

  CompoundType *ct = container->type->asRval()->asCompoundType();
  Variable *field = ct->rawLookupVariable(fieldName);
  if (!field) {
    xfailure(stringbc("not in '" << container->name <<
                      "': " << fieldName));
  }
  return field;
}


// EOF
