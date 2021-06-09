// main.cc            see license.txt for copyright and terms of use
// entry-point module for a program that parses C++

#include "elsaparse.h"    // ElsaParse

// smbase
#include "ckheap.h"       // malloc_stats


void if_malloc_stats()
{
  if (tracingSys("malloc_stats")) {
    malloc_stats();
  }
}


// When true, print the count of warnings and errors, and the phase times.
static bool verboseOutput = false;

// Defined in cc_tcheck.cc.
extern bool printStringLiterals;


// this is a dumb way to organize argument processing...
static char *myProcessArgs(int argc, char **argv, ElsaParse &elsaParse,
                           char const *additionalInfo)
{
  // remember program name
  char const *progName = argv[0];

  // process args
  while (argc >= 2) {
    if (traceProcessArg(argc, argv)) {
      continue;
    }
    else if (0==strcmp(argv[1], "-xc")) {
      // treat this as a synonym for "-tr c_lang"
      traceAddSys("c_lang");
      argv++;
      argc--;
    }
    else if (0==strcmp(argv[1], "-w")) {
      // synonym for -tr nowarnings
      traceAddSys("nowarnings");
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--quiet")) {
      verboseOutput = false;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--verbose")) {
      verboseOutput = true;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--prettyPrint")) {
      elsaParse.prettyPrint = true;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--printStringLiterals")) {
      printStringLiterals = true;
      argv++;
      argc--;
    }
    else {
      break;     // didn't find any more options
    }
  }

  if (argc != 2) {
    cout << "usage: " << progName << " [options] input-file\n"
            "  options:\n"
            "    -tr <flags>:           turn on given tracing flags (comma-separated)\n"
            "    -xc                    parse input as C rather than C++\n"
            "    -w                     disable warnings\n"
            "    --verbose              print error/warn counts and times\n"
            "    --quiet                opposite of --verbose (and the default)\n"
            "    --prettyPrint          pretty-print the parsed AST as C/C++ syntax\n"
            "    --printStringLiterals  print every decoded string literal\n"
         << (additionalInfo? additionalInfo : "");
    exit(argc==1? 0 : 2);    // error if any args supplied
  }

  return argv[1];
}


static int doit(int argc, char **argv)
{
  // I think this is more noise than signal at this point
  xBase::logExceptions = false;

  if_malloc_stats();

  SourceLocManager mgr;

  // string table for storing parse tree identifiers
  StringTable strTable;

  // parsing language options
  CCLang lang;
  lang.GNU_Cplusplus();

  // Object that manages the parsing process.
  ElsaParse elsaParse(strTable, lang);


  // ------------- process command-line arguments ---------
  char const *inputFname = myProcessArgs
    (argc, argv, elsaParse,
     "\n"
     "  general behavior flags:\n"
     "    c_lang             use C language rules (default is C++)\n"
     "    nohashline         ignore #line when reporting locations\n"
     "\n"
     "  options to stop after a certain stage:\n"
     "    stopAfterParse     stop after parsing\n"
     "    stopAfterTCheck    stop after typechecking\n"
     "    stopAfterElab      stop after semantic elaboration\n"
     "\n"
     "  output options:\n"
     "    parseTree          make a parse tree and print that, only\n"
     "    printAST           print AST after parsing\n"
     "    printTypedAST      print AST with type info\n"
     "    printElabAST       print AST after semantic elaboration\n"
     "\n"
     "  debugging output:\n"
     "    malloc_stats       print malloc stats every so often\n"
     "    env                print as variables are added to the environment\n"
     "    error              print as errors are accumulated\n"
     "    overload           print details of overload resolution\n"
     "\n"
     "  (grep in source for \"trace\" to find more obscure flags)\n"
     "");

  if (verboseOutput) {
    traceAddSys("progress");
  }

  if (tracingSys("printAsML")) {
    Type::printAsML = true;
  }

  // FIX: dsw: couldn't we put dashes or something in here to break up
  // the word?
  if (tracingSys("nohashline")) {
    sourceLocManager->useHashLines = false;
  }

  if (tracingSys("tolerateHashlineErrors")) {
    sourceLocManager->tolerateHashlineErrors = true;
  }

  if (tracingSys("no-orig-offset")) {
    sourceLocManager->useOriginalOffset = false;
  }

  if (tracingSys("ansi")) {
    lang.ANSI_Cplusplus();
  }

  if (tracingSys("ansi_c")) {
    lang.ANSI_C89();
  }

  if (tracingSys("ansi_c99")) {
    lang.ANSI_C99();
  }

  if (tracingSys("c_lang")) {
    lang.GNU_C();
  }

  if (tracingSys("gnu_c89")) {
    lang.ANSI_C89();
    lang.GNU_C_extensions();
  }

  if (tracingSys("gnu_kandr_c_lang")) {
    lang.GNU_KandR_C();
    #ifndef KANDR_EXTENSION
      xfatal("gnu_kandr_c_lang option requires the K&R module (./configure -kandr=yes)");
    #endif
  }

  if (tracingSys("gnu2_kandr_c_lang")) {
    lang.GNU2_KandR_C();
    #ifndef KANDR_EXTENSION
      xfatal("gnu2_kandr_c_lang option requires the K&R module (./configure -kandr=yes)");
    #endif
  }

  if (tracingSys("test_xfatal")) {
    xfatal("this is a test error message");
  }

  if (tracingSys("msvcBugs")) {
    lang.MSVC_bug_compatibility();
  }

  if (!tracingSys("nowarnings")) {
    lang.enableAllWarnings();
  }

  if (tracingSys("templateDebug")) {
    // predefined set of tracing flags I've been using while debugging
    // the new templates implementation
    traceAddSys("template");
    traceAddSys("error");
    traceAddSys("scope");
    traceAddSys("templateParams");
    traceAddSys("templateXfer");
    elsaParse.prettyPrint = true;
    traceAddSys("topform");
  }

  // dump out the lang settings if the user wants them
  if (tracingSys("printLang")) {
    cout << "language settings:\n";
    cout << lang.toString();
    cout << endl;
  }
  if (tracingSys("printTracers")) {
    cout << "tracing flags:\n\t";
    printTracers(std::cout, "\n\t");
    cout << endl;
  }

  // Run the parser.
  elsaParse.printErrorCount = verboseOutput;
  elsaParse.parse(inputFname);
  if (verboseOutput) {
    elsaParse.printTimes();
  }

  //traceProgress() << "cleaning up...\n";

  //malloc_stats();

  // delete the tree
  // (currently this doesn't do very much because FakeLists are
  // non-owning, so I won't pretend it does)
  //delete unit;

  strTable.clear();

  //checkHeap();
  //malloc_stats();

  return 0;
}


int main(int argc, char **argv)
{
  try {
    return doit(argc, argv);
  }
  catch (XUnimp &x) {
    HANDLER();
    cout << x << endl;

    // don't consider this the same as dying on an assertion failure;
    // I want to have tests in regrtest that are "expected" to fail
    // for the reason that they use unimplemented language features
    return 10;
  }
  catch (XFatal &x) {
    HANDLER();

    // similar to XUnimp
    cout << x << endl;
    return 10;
  }
  catch (xBase &x) {
    HANDLER();
    cout << x << endl;
    return 4;
  }
}
