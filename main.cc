// main.cc            see license.txt for copyright and terms of use
// entry-point module for a program that parses C++

// elsa
#include "ast_build.h"                 // test_astbuild
#include "elsaparse.h"                 // ElsaParse
#include "strip-comments.h"            // strip_comments_unit_tests

// smbase
#include "objcount.h"                  // CheckObjectCount
#include "strutil.h"                   // prefixEquals
#include "trace.h"                     // tracingSys


enum TargetPlatform {
  TF_BUILD,
  TF_LINUX64,
  TF_WIN64,
  TF_WIN32,
};

// Platform to emulate.
static TargetPlatform targetPlatform = TF_BUILD;

// When true, print the count of warnings and errors, and the phase times.
static bool verboseOutput = false;

// When true, the elaboration pass is run after parsing.
static bool enableElaboration = true;


// Decode the --target argument.
static TargetPlatform decodeTargetPlatform(char const *target)
{
  if (streq(target, "build")) {
    return TF_BUILD;
  }
  else if (streq(target, "linux64")) {
    return TF_LINUX64;
  }
  else if (streq(target, "win64")) {
    return TF_WIN64;
  }
  else if (streq(target, "win32")) {
    return TF_WIN32;
  }
  else {
    xfatal("Unknown target \"" << target << "\".");
    return TF_BUILD;     // Not reached.
  }
}


static void runUnitTests()
{
  strip_comments_unit_tests();
  cout << "unit tests passed\n";
}


// this is a dumb way to organize argument processing...
static char const *myProcessArgs(int argc, char **argv, ElsaParse &elsaParse,
                                 char const *additionalInfo)
{
  // remember program name
  char const *progName = argv[0];

  // True if the language has been specified with "-x".
  bool specifiedLanguage = false;

  // State of -pedantic or -pedantic-errors.
  Bool3 allowPedanticViolations = B3_TRUE;

  // process args
  while (argc >= 2) {
    if (traceProcessArg(argc, argv)) {
      continue;
    }
    else if (prefixEquals(argv[1], "-x")) {
      string lang(argv[1]+2);
      if (!elsaParse.setDashXLanguage(lang)) {
        xfatal(stringb("Unrecognized -x argument: \"" << lang << "\"."));
      }

      specifiedLanguage = true;
      argv++;
      argc--;
    }
    else if (0==strcmp(argv[1], "-w")) {
      // synonym for -tr nowarnings
      traceAddSys("nowarnings");
      argv++;
      argc--;
    }
    else if (streq(argv[1], "-pedantic")) {
      allowPedanticViolations = B3_WARN;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "-pedantic-errors")) {
      allowPedanticViolations = B3_FALSE;
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
    else if (streq(argv[1], "--pretty-print")) {
      elsaParse.m_prettyPrint = true;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--no-pp-comments")) {
      elsaParse.m_prettyPrintComments = false;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--print-isc")) {
      elsaParse.m_prettyPrintISC = true;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--print-string-literals")) {
      elsaParse.m_printStringLiterals = true;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--target")) {
      if (argc == 2) {
        xfatal("--target option requires an argument");
      }
      targetPlatform = decodeTargetPlatform(argv[2]);
      argv += 2;
      argc -= 2;
    }
    else if (streq(argv[1], "--no-elaborate")) {
      enableElaboration = false;
      argv++;
      argc--;
    }
    else if (streq(argv[1], "--unit-tests")) {
      runUnitTests();
      exit(0);
    }
    else if (streq(argv[1], "--test-print-astbuild")) {
      test_print_astbuild();
      exit(0);
    }
    else if (argv[1][0] == '-') {
      xfatal("unknown option: " << argv[1]);
    }
    else {
      break;     // didn't find any more options
    }
  }

  if (argc != 2) {
    cout << "usage: " << progName << " [options] input-file\n"
            "  options:\n"
            "    -tr <flags>              turn on given tracing flags (comma-separated)\n"
            "    -x<lang>                 parse input as <lang>: \"c\" or \"c++\"\n"
            "                             (default is based on file extension, like gcc)\n"
            "    --target <target>        options: build (default), linux64, win64, win32\n"
            "    -w                       disable warnings\n"
            "    -pedantic                Warn for certain technical violations.\n"
            "    -pedantic-errors         Error on certain technical violations.\n"
            "    --verbose                print error/warn counts and times\n"
            "    --quiet                  opposite of --verbose (and the default)\n"
            "    --pretty-print           pretty-print the parsed AST as C/C++ syntax\n"
            "    --no-pp-comments         suppress details comments in pretty-print\n"
            "    --print-isc              print implicit standard conversion\n"
            "    --print-string-literals  print every decoded string literal\n"
            "    --no-elaborate           disable elaboration pass\n"
            "    --unit-tests             run internal unit tests\n"
         << (additionalInfo? additionalInfo : "");
    exit(argc==1? 0 : 2);    // error if any args supplied
  }

  char const *inputFname = argv[1];

  // ------ choose overall language ------
  CCLang &lang = elsaParse.m_lang;

  if (!specifiedLanguage) {
    elsaParse.setDefaultLanguage(inputFname);
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

  // ------ choose language options ----
  lang.setPedantic(allowPedanticViolations);

  if (tracingSys("msvcBugs")) {
    lang.MSVC_bug_compatibility();
  }

  if (!tracingSys("nowarnings")) {
    lang.enableAllWarnings();
  }

  return inputFname;
}


static int doit(int argc, char **argv)
{
  // I think this is more noise than signal at this point
  xBase::logExceptions = false;

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
     "    env                print as variables are added to the environment\n"
     "    error              print as errors are accumulated\n"
     "    overload           print details of overload resolution\n"
     "\n"
     "  (grep in source for \"trace\" or \"TRACE\" to find more obscure flags)\n"
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

  if (targetPlatform == TF_LINUX64) {
    lang.m_typeSizes.set_linux_x86_64();
  }
  else if (targetPlatform == TF_WIN64) {
    lang.m_typeSizes.set_windows_x86_64();
  }
  else if (targetPlatform == TF_WIN32) {
    lang.m_typeSizes.set_windows_x86();
  }

  if (!enableElaboration) {
    elsaParse.m_elabActivities = EA_NONE;
  }

  if (tracingSys("test_xfatal")) {
    xfatal("this is a test error message");
  }

  if (tracingSys("templateDebug")) {
    // predefined set of tracing flags I've been using while debugging
    // the new templates implementation
    traceAddSys("template");
    traceAddSys("error");
    traceAddSys("scope");
    traceAddSys("templateParams");
    traceAddSys("templateXfer");
    elsaParse.m_prettyPrint = true;
    traceAddSys("topform");
  }

  // dump out the lang settings if the user wants them
  if (tracingSys("printLang")) {
    cout << "language settings:\n";
    cout << lang.toString();
  }
  if (tracingSys("printTracers")) {
    cout << "tracing flags:\n\t";
    printTracers(std::cout, "\n\t");
    cout << endl;
  }

  // Run the parser.
  elsaParse.m_printErrorCount = verboseOutput;
  if (!elsaParse.parse(inputFname)) {
    // The input had syntax errors, which have been printed.
    return 2;
  }
  if (verboseOutput) {
    elsaParse.printTimes();
  }

  // Turn this testing on all the time by default.  Its effect on
  // end to end speed is less than what I can easily measure.
  if (!tracingSys("notest_astbuild")) {
    test_astbuild(elsaParse);
  }

  //traceProgress() << "cleaning up...\n";

  // delete the tree
  // (currently this doesn't do very much because FakeLists are
  // non-owning, so I won't pretend it does)
  //delete unit;

  strTable.clear();

  return 0;
}


int main(int argc, char **argv)
{
  try {
    try {
      return doit(argc, argv);
    }
    catch (...) {
      // If we die by exception, leaks are routine, and the message can
      // misdirect attention.
      CheckObjectCount::s_suppressLeakReports = true;
      throw;
    }
  }
  catch (XUnimp &x) {
    HANDLER();
    cerr << x << endl;

    // don't consider this the same as dying on an assertion failure;
    // I want to have tests in regrtest that are "expected" to fail
    // for the reason that they use unimplemented language features
    return 10;
  }
  catch (XFatal &x) {
    HANDLER();

    // similar to XUnimp
    cerr << x << endl;
    return 10;
  }
  catch (xBase &x) {
    HANDLER();
    cerr << x << endl;
    return 4;
  }
}
