// elsaparse.h
// Main entry point for Elsa as a library.

#ifndef ELSA_ELSAPARSE_H
#define ELSA_ELSAPARSE_H

#include "elsaparse-fwd.h"             // forwards for this module

// elsa
#include "cc-ast.h"                    // TranslationUnit, Function
#include "cc-lang.h"                   // CCLang
#include "elab-activities.h"           // ElabActivities

// smbase
#include "strtable.h"                  // StringTable


// Class to contain the parameters to and results of parsing.
//
// TODO: This interface is not very well designed, consisting of a
// diverse set of public data members, a one main method ('parse', which
// is fine), and a handful of ad-hoc queries of the result.  The data
// members should be organized into meaningful units, and the ad-hoc
// queries moved to some class that can better serve as the result of
// parsing rather than the machinery to perform it.
//
// Regarding the data members, it's troubling that, in order to use this
// class, several supporting objects are required (like StringTable and
// CCLang), so each client has to instantiate them all.
//
class ElsaParse {
  NO_OBJECT_COPIES(ElsaParse);

public:      // data
  // String table for identifiers.
  StringTable &m_stringTable;

  // Way to make types.
  BasicTypeFactory m_typeFactory;

  // Language options.
  CCLang &m_lang;

  // If true, print the counts of errors and warnings at the end.  This
  // is initially false.
  bool m_printErrorCount;

  // If true, print warnings.  Initially true.
  bool m_printWarnings;

  // If true, pretty-print the parsed AST as C/C++ syntax after parsing.
  // Initially false.
  bool m_prettyPrint;

  // If true, the pretty-printed AST contains some comments with
  // additional, invisible details.  Initially true.
  bool m_prettyPrintComments;

  // If true, the pretty-printed AST includes implicit standard
  // conversions.  Initially false.
  bool m_prettyPrintISC;

  // If true, print decoded string literals after parsing.  Initially
  // false.
  bool m_printStringLiterals;

  // Parameters to the elaborator.  By default, we do full elaboration
  // and do not clone defunct children.  However, setting
  // 'm_prettyPrint' causes 'EA_REMOVE_DEFUNCT_CHILDREN' to be changed
  // to false during parsing.
  ElabActivities m_elabActivities;

  // The parsed TU.  This may be nullptr after parsing, for example if
  // "-tr parseTree" is passed.  It is never nullptr if
  // 'm_tcheckCompleted' is true.
  TranslationUnit *m_translationUnit;

  // The 'main' function definition, or nullptr if there was not a
  // 'main' function in the translation unit.
  Function *m_mainFunction;

  // Time in milliseconds for various phases.
  long m_parseTime;
  long m_tcheckTime;
  long m_integrityTime;
  long m_elaborationTime;

  // True if we ran and completed the type check phase.  There are some
  // ad-hoc tracing flags, such as "-tr stopAfterParse", that cause
  // 'parse()' to return true without having done type checking, and
  // consumers may need to adjust their behavior accordingly.
  bool m_tcheckCompleted;

public:      // methods
  ElsaParse(StringTable &stringTable, CCLang &lang);
  ~ElsaParse();

  // Configure for the language 'lang', which is treated as a string
  // that could be passed to "gcc -x".  Return false if it is not
  // recognized.
  bool setDashXLanguage(string const &lang);

  // Configure for the default language based on the extension of
  // 'inputFname'.  This uses the GCC command line extension rules.
  //
  // Return true if the extension is recognized.  If not, leave the
  // language settings as they already are and return false.
  bool setDefaultLanguage(char const *inputFname);

  // Parse 'inputFname' according to the language preferences in 'lang'.
  //
  // In the case of syntax errors in the input file, print messages to
  // stderr and return false.  But note that, depending on how the data
  // members are configured, we might stop before doing a full
  // type-check.
  //
  // There are also some "intended" exceptions that can be thrown, the
  // main (possibly only) one being for failure to open the input file.
  //
  // Beyond that, this can throw exceptions or exit(4) due to internal
  // assertion failures, and there are a few special cases that lead
  // to exit(0), but clients generally shouldn't have to be concerned
  // with any of that.
  //
  bool parse(char const *inputFname);

  // If 'm_prettyPrint', pretty-print the AST.
  void maybePrettyPrint();

  // Print the phase times.
  void printTimes();

  // Search the global scope for a type with the given name.  Throw if
  // it is not found.
  Type *getGlobalType(char const *typeName) const;

  // Same for variables, and also for typedefs.
  Variable *getGlobalVar(char const *varName) const;

  // Variant that returns NULL if not found.
  Variable *getGlobalVarOpt(char const *varName) const;

  // Search within a Variable whose type is a CompoundType for a
  // field with the given name.
  Variable *getFieldOf(Variable *container, char const *fieldName) const;
};


#endif // ELSA_ELSAPARSE_H
