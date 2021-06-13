// elsaparse.cc          see license.txt for copyright and terms of use
// Code for elsaparse.h.

#include "elsaparse.h"    // this module

#include "sm-iostream.h"  // cout
#include <stdlib.h>       // exit, getenv, abort
#include "sm-fstream.h"   // ofstream

#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "srcloc.h"       // SourceLocManager
#include "ckheap.h"       // checkHeap
#include "cc_env.h"       // Env
#include "cc_ast.h"       // C++ AST (r)
#include "cc_ast_aux.h"   // class LoweredASTVisitor
#include "cc_lang.h"      // CCLang
#include "parsetables.h"  // ParseTables
#include "cc_print.h"     // PrintEnv
#include "cc.gr.gen.h"    // CCParse
#include "nonport.h"      // getMilliseconds
#include "ptreenode.h"    // PTreeNode
#include "ptreeact.h"     // ParseTreeLexer, ParseTreeActions
#include "sprint.h"       // structurePrint
#include "strtokp.h"      // StrtokParse
#include "smregexp.h"     // regexpMatch
#include "cc_elaborate.h" // ElabVisitor
#include "integrity.h"    // IntegrityVisitor


// little check: is it true that only global declarators
// ever have Declarator::type != Declarator::var->type?
// .. no, because it's true for class members too ..
// .. it's also true of arrays declared [] but then inited ..
// .. finally, it's true of parameters whose types get
//    normalized as per cppstd 8.3.5 para 3; I didn't include
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

  virtual bool visitDeclarator(Declarator *obj);
};

bool DeclTypeChecker::visitDeclarator(Declarator *obj)
{
  if (obj->type != obj->var->type &&
      !(obj->var->flags & (DF_GLOBAL | DF_MEMBER)) &&
      !obj->type->isArrayType()) {
    instances++;
    cout << toString(obj->var->loc) << ": " << obj->var->name
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

  virtual bool visitExpression(Expression *obj)
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
  virtual bool visitExpression(Expression *expr);
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


void handle_xBase(Env &env, xBase &x)
{
  // typically an assertion failure from the tchecker; catch it here
  // so we can print the errors, and something about the location
  env.errors.print(cout);
  cout << x << endl;
  cout << "Failure probably related to code near " << env.locStr() << endl;

  // print all the locations on the scope stack; this is sometimes
  // useful when the env.locStr refers to some template code that
  // was instantiated from somewhere else
  //
  // (unfortunately, env.instantiationLocStack isn't an option b/c
  // it will have been cleared by the automatic invocation of
  // destructors unwinding the stack...)
  cout << "current location stack:\n";
  cout << env.locationStackString();

  // I changed from using exit(4) here to using abort() because
  // that way the multitest.pl script can distinguish them; the
  // former is reserved for orderly exits, whereas signals (like
  // SIGABRT) mean that something went really wrong
  abort();
}


ElsaParse::ElsaParse(StringTable &strTable_, CCLang &lang_)
  : strTable(strTable_),
    lang(lang_),
    printErrorCount(false),
    prettyPrint(false),
    printStringLiterals(false),
    unit(NULL),
    mainFunction(NULL),
    parseTime(0),
    tcheckTime(0),
    integrityTime(0),
    elaborationTime(0)
{}


ElsaParse::~ElsaParse()
{}


void ElsaParse::parse(char const *inputFname)
{
  // dsw: I needed this to persist past typechecking, so I moved it
  // out here.  Feel free to refactor.
  ArrayStack<Variable*> madeUpVariables;
  ArrayStack<Variable*> builtinVars;

  int parseWarnings = 0;
  {
    SectionTimer timer(parseTime);
    SemanticValue treeTop;
    ParseTreeAndTokens tree(lang, treeTop, strTable, inputFname);

    // grab the lexer so we can check it for errors (damn this
    // 'tree' thing is stupid..)
    Lexer *lexer = dynamic_cast<Lexer*>(tree.lexer);
    xassert(lexer);

    CCParse *parseContext = new CCParse(strTable, lang);
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
      return;
    }

    // treeTop is a TranslationUnit pointer
    unit = (TranslationUnit*)treeTop;

    //unit->debugPrint(cout, 0);

    delete parseContext;
    delete tables;
  }

  checkHeap();

  // print abstract syntax tree
  if (tracingSys("printAST")) {
    unit->debugPrint(cout, 0);
  }

  //if (unit) {     // when "-tr trivialActions" it's NULL...
  //  cout << "ambiguous nodes: " << numAmbiguousNodes(unit) << endl;
  //}

  if (tracingSys("stopAfterParse")) {
    return;
  }


  // ---------------- typecheck -----------------
  if (tracingSys("no-typecheck")) {
    cout << "no-typecheck" << endl;
  } else {
    SectionTimer timer(tcheckTime);
    Env env(strTable, lang, m_typeFactory, madeUpVariables, builtinVars, unit);
    try {
      env.tcheckTranslationUnit(unit);
    }
    catch (XUnimp &x) {
      HANDLER();

      // relay to handler in main()
      cout << "in code near " << env.locStr() << ":\n";
      throw;
    }
    catch (x_assert &x) {
      HANDLER();

      if (env.errors.hasFromNonDisambErrors()) {
        if (tracingSys("expect_confused_bail")) {
          cout << "got the expected confused/bail\n";
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
        env.errors.print(cout);
        exit(4);
      }

      if (tracingSys("expect_xfailure")) {
        cout << "got the expected xfailure\n";
        exit(0);
      }

      // if we don't have a basis for reducing severity, pass this on
      // to the normal handler
      handle_xBase(env, x);
    }
    catch (xBase &x) {
      HANDLER();
      handle_xBase(env, x);
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
      numErrors += computeUnitCFG(unit);
    }
    #endif // CFG_EXTENSION

    // print abstract syntax tree annotated with types
    if (tracingSys("printTypedAST")) {
      unit->debugPrint(cout, 0);
    }

    // structural delta thing
    if (tracingSys("structure")) {
      structurePrint(unit);
    }

    if (numErrors==0 && tracingSys("secondTcheck")) {
      // this is useful to measure the cost of disambiguation, since
      // now the tree is entirely free of ambiguities
      traceProgress() << "beginning second tcheck...\n";
      Env env2(strTable, lang, m_typeFactory, madeUpVariables, builtinVars, unit);
      unit->tcheck(env2);
      traceProgress() << "end of second tcheck\n";
    }

    // print errors and warnings
    env.errors.print(cout);

    if (printErrorCount) {
      cout << "typechecking results:\n"
           << "  errors:   " << numErrors << "\n"
           << "  warnings: " << numWarnings << "\n";
    }

    if (numErrors != 0) {
      xfatal("type check error");
    }

    // lookup diagnostic
    if (env.collectLookupResults.length()) {
      // scan AST
      NameChecker nc;
      nc.sb << "collectLookupResults";
      unit->traverse(nc);

      // compare to given text
      if (streq(env.collectLookupResults, nc.sb)) {
        // ok
      }
      else {
        cout << "collectLookupResults do not match:\n"
             << "  source: " << env.collectLookupResults << "\n"
             << "  tcheck: " << nc.sb << "\n"
             ;
        exit(4);
      }
    }

    // For the benefit of the interpreter, get 'main' if it exists.
    {
      StringRef mainName = strTable.add("main");
      Variable *mainVar =
        env.globalScope()->lookupVariable(mainName, env);
      if (mainVar) {
        mainFunction = mainVar->funcDefn;
      }
    }
  }

  // ---------------- integrity checking ----------------
  {
    SectionTimer timer(integrityTime);

    // check AST integrity
    IntegrityVisitor ivis;
    unit->traverse(ivis);

    // check that the AST is a tree *and* that the lowered AST is a
    // tree; only do this *after* confirming that tcheck finished
    // without errors
    if (tracingSys("treeCheck")) {
      long start = getMilliseconds();
      LoweredIsTreeVisitor treeCheckVisitor;
      unit->traverse(treeCheckVisitor.loweredVisitor);
      traceProgress() << "done with tree check 1 ("
                      << (getMilliseconds() - start)
                      << " ms)\n";
    }

    // check an expected property of the annotated AST
    if (tracingSys("declTypeCheck") || getenv("declTypeCheck")) {
      DeclTypeChecker vis;
      unit->traverse(vis.loweredVisitor);
      cout << "instances of type != var->type: " << vis.instances << endl;
    }

    if (tracingSys("stopAfterTCheck")) {
      return;
    }
  }

  // ----------------- elaboration ------------------
  if (tracingSys("no-elaborate")) {
    cout << "no-elaborate" << endl;
  }
  else {
    SectionTimer timer(elaborationTime);

    ElabVisitor vis(strTable, m_typeFactory, unit);

    if (!lang.isCplusplus) {
      // do only the C elaboration activities
      vis.activities = EA_C_ACTIVITIES;
    }

    // if we are going to pretty print, then we need to retain defunct children
    if (prettyPrint) {
      vis.cloneDefunctChildren = true;
    }

    // do elaboration
    unit->traverse(vis.loweredVisitor);

    // print abstract syntax tree annotated with types
    if (tracingSys("printElabAST")) {
      unit->debugPrint(cout, 0);
    }
    if (tracingSys("stopAfterElab")) {
      return;
    }
  }

  // mark "real" (non-template) variables as such
  {
    MarkRealVars markReal;
    visitVarsF(builtinVars, markReal);
    visitRealVarsF(unit, markReal);
  }

  // more integrity checking
  {
    SectionTimer timer(integrityTime);

    // check that the AST is a tree *and* that the lowered AST is a
    // tree (do this *after* elaboration!)
    if (tracingSys("treeCheck")) {
      long start = getMilliseconds();
      LoweredIsTreeVisitor treeCheckVisitor;
      unit->traverse(treeCheckVisitor.loweredVisitor);
      traceProgress() << "done with tree check 2 ("
                      << (getMilliseconds() - start)
                      << " ms)\n";
    }
  }

  if (printStringLiterals) {
    PrintStringLiteralsVisitor visitor;
    LoweredASTVisitor loweredVisitor(&visitor);
    unit->traverse(loweredVisitor);
  }

  if (prettyPrint) {
    OStreamOutStream out0(cout);
    CodeOutStream codeOut(out0);
    CTypePrinter typePrinter(lang);
    PrintEnv env(typePrinter, &codeOut);
    cout << "---- START ----" << endl;
    cout << "// -*-c++-*-" << endl;
    unit->print(env);
    codeOut.finish();
    cout << "---- STOP ----" << endl;
  }

  // test AST cloning
  if (tracingSys("testClone")) {
    TranslationUnit *u2 = unit->clone();

    if (tracingSys("cloneAST")) {
      cout << "------- cloned AST --------\n";
      u2->debugPrint(cout, 0);
    }

    if (tracingSys("cloneCheck")) {
      ArrayStack<Variable*> madeUpVariables2;
      ArrayStack<Variable*> builtinVars2;
      // dsw: I hope you intend that I should use the cloned TranslationUnit
      Env env3(strTable, lang, m_typeFactory, madeUpVariables2, builtinVars2, u2);
      u2->tcheck(env3);

      if (tracingSys("cloneTypedAST")) {
        cout << "------- cloned typed AST --------\n";
        u2->debugPrint(cout, 0);
      }

      if (tracingSys("clonePrint")) {
        OStreamOutStream out0(cout);
        CodeOutStream codeOut(out0);
        CTypePrinter typePrinter(lang);
        PrintEnv penv(typePrinter, &codeOut);
        cout << "---- cloned pretty print ----" << endl;
        u2->print(penv);
        codeOut.finish();
      }
    }
  }

  // test debugPrint but send the output to /dev/null (basically just
  // make sure it doesn't segfault or abort)
  if (tracingSys("testDebugPrint")) {
    ofstream devnull("/dev/null");
    unit->debugPrint(devnull, 0);
  }
}


void ElsaParse::printTimes()
{
  cout << "parse=" << parseTime << "ms"
       << " tcheck=" << tcheckTime << "ms"
       << " integ=" << integrityTime << "ms"
       << " elab=" << elaborationTime << "ms"
       << "\n"
       ;
}


Type *ElsaParse::getGlobalType(char const *typeName_) const
{
  StringRef typeName = strTable.add(typeName_);

  Variable *var = unit->globalScope->rawLookupVariable(typeName);
  if (!var) {
    xfailure(stringb("not in the global scope: " << typeName));
  }
  if (!var->hasFlag(DF_TYPEDEF)) {
    xfailure(stringb("not a type: " << typeName));
  }
  return var->type;
}


Variable *ElsaParse::getGlobalVar(char const *varName_) const
{
  StringRef varName = strTable.add(varName_);

  Variable *var = unit->globalScope->rawLookupVariable(varName);
  if (!var) {
    xfailure(stringb("not in the global scope: " << varName));
  }
  if (var->hasFlag(DF_TYPEDEF)) {
    xfailure(stringb("is a type, not a variable: " << varName));
  }
  return var;
}


// EOF
