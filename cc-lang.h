// cc-lang.h            see license.txt for copyright and terms of use
// Class CCLang.

// a useful reference:
//   Incompatibilities Between ISO C and ISO C++
//   David R. Tribble
//   http://david.tribble.com/text/cdiffs.htm

#ifndef ELSA_CC_LANG_H
#define ELSA_CC_LANG_H

#include "cc-lang-fwd.h"               // forwards for this module

// elsa
#include "type-sizes.h"                // TypeSizes

// smbase
#include "str.h"                       // string


// This type is used for options that nominally either allow or
// disallow some syntax, but can also trigger a warning.  Values of
// this type are intended to be tested like booleans in most places.
enum Bool3 {
  B3_FALSE = 0,      // syntax not allowed
  B3_TRUE = 1,       // accepted silently
  B3_WARN = 2,       // accept with a warning
};


// Language options that the parser (etc.) is sensitive to.
class CCLang {
public:      // data
  // Sizes of scalar types.
  TypeSizes m_typeSizes;

  // catch-call for behaviors that are unique to C++ but aren't
  // enumerated above; these behaviors are candidates for being split
  // out as separate flags, but there currently is no need
  bool isCplusplus;

  // declare the various GNU __builtin functions; see
  // Env::addGNUBuiltins in gnu.cc
  bool declareGNUBuiltins;

  // when this is true, and the parser sees "struct Foo { ... }",
  // it will pretend it also saw "typedef struct Foo Foo;" -- i.e.,
  // the structure (or class) tag name is treated as a type name
  // by itself
  bool tagsAreTypes;

  // when true, recognize C++ keywords in input stream
  bool recognizeCppKeywords;

  // when true, every function body gets an implicit
  //   static char const __func__[] = "function-name";
  // declaration just inside the opening brace, where function-name is
  // the name of the function; this is a C99 feature (section 6.4.2.2)
  bool implicitFuncVariable;

  // behavior of gcc __FUNCTION__ and __PRETTY_FUNCTION__
  // see also
  //   http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Function-Names.html
  //   http://gcc.gnu.org/onlinedocs/gcc-2.95.3/gcc_4.html#SEC101
  enum GCCFuncBehavior {
    GFB_none,              // ordinary symbols
    GFB_string,            // string literal (they concatenate!)
    GFB_variable,          // variables, like __func__
  } gccFuncBehavior;

  // when true, and we see a class declaration inside something,
  // pretend it was at toplevel scope anyway; this also applies to
  // enums, enumerators and typedefs
  //
  // dsw: I find that having boolean variables that are in the
  // negative sense is usually a mistake.  I would reverse the sense
  // of this one.
  //
  // sm: The 'no' is a little misleading.  In the 'false' case,
  // syntax reflects semantics naturally; only in the 'true' case
  // is something unusual going on.  A positive-sense name might be
  // the unwieldy 'turnApparentlyInnerClassesIntoOuterClasses'.
  bool noInnerClasses;

  // when true, an uninitialized global data object is typechecked as
  // a common symbol ("C" in the nm(1) manpage) instead of a bss
  // symbol ("B").  This means that the following is not an error:
  //   int a; int a;
  // gcc seems to operate as if this is true, whereas g++ not.
  //
  // these are the so-called "tentative" definitions of C; the flag
  // is somewhat misnamed
  bool uninitializedGlobalDataIsCommon;

  // when true, if a function has an empty parameter list then it is
  // treated as supplying no parameter information (C99 6.7.5.3 para 14)
  bool emptyParamsMeansNoInfo;

  // When true, allow an incomplete array type as a member of a class.
  bool allowIncompleteArrayTypeMembers;

  // when true, assume arrays with no size are of size 1 and issue a
  // warning
  //
  // TODO: This is not the proper way to handle C's rules for arrays.
  // See C99 6.9.2p2, 6.9.2e5, 6.7p7 and 6.7p16.  What we have now
  // is just a hack for the sake of expedience.
  bool assumeNoSizeArrayHasSizeOne;

  // when true, we allow overloaded function declarations (same name,
  // different signature)
  bool allowOverloading;

  // when true, to every compound type add the name of the type itself
  bool compoundSelfName;

  // when true, allow a function call to a function that has never
  // been declared, implicitly declaring the function in the global
  // scope; this is for C89 (and earlier) support
  Bool3 allowImplicitFunctionDecls;

  // when true, allow function definitions that omit any return type
  // to implicitly return 'int'.
  bool allowImplicitInt;

  // GNU extension: when true, allow local variable arrays to have
  // sizes that are not constant
  bool allowDynamicallySizedArrays;

  // GCC extension: when true, you can say things like 'enum Foo;' and
  // it declares that an enum called Foo will be defined later
  bool allowIncompleteEnums;

  // C language, and GNU extension for C++: allow a class to have a
  // member that has the same name as the class
  bool allowMemberWithClassName;

  // every C++ compiler I have does overload resolution of operator=
  // differently from what is specified in the standard; this flag
  // causes Elsa to do the same
  bool nonstandardAssignmentOperator;

  // permit prototypes to have mismatching exception specs if the
  // function is extern "C" (TODO: provide more documentation)
  bool allowExternCThrowMismatch;

  // allow main() to be declared/defined with an implicit 'int'
  bool allowImplicitIntForMain;

  // when true, "_Bool" is a built-in type keyword (C99)
  bool predefined_Bool;

  // dsw: when true, a function definition with 'extern' and 'inline'
  // keywords is handled specially.  How exactly is a function of the
  // optimization conditions; these are not language conditions so
  // they are handed by a tracing flag rather than by a language flag.
  // The tracing flag is 'handleExternInline-asPrototype'.  If true,
  // then we simply ignore the body of an extern inline.  When false
  // we handle extern inlines as weak static inlines: the 'extern
  // inline' is converted to 'static inline' and if another definition
  // in the translation unit is found, it replaces that of the extern
  // inline.  These two modes seem to reflect the behavior of gcc
  // 3.4.6 when optimizations are off and on respectively.
  bool handleExternInlineSpecially;

  // quarl: whether "inline" implies static linkage.  True in C++ but not in
  // C.
  bool inlineImpliesStaticLinkage;

  // dsw: C99 std 6.4.5p5: "For character string literals, the array
  // elements have type char...."; Cppstd 2.13.4p1: "An ordinary
  // string literal has type "array of const char" and static storage
  // duration"; But empirical results show that even in C++, gcc makes
  // string literals arrays of (nonconst) chars.
  bool stringLitCharsAreConst;

  // 2022-06-07: Removed 'lvalueFlowsThroughCast' since it corresponds
  // to a GCC extension that was removed long ago.

  // when true, 'restrict' is a keyword
  bool restrictIsAKeyword;

  // ---- pedantic diagnostic flags ----
  // These flags control "pedantic" diagnostics, like those of GCC or
  // Clang -pedantic.  The default for all is B3_TRUE, meaning they are
  // allowed without warning or error.

  // When false, an empty struct or union is an error in C.  C11
  // 6.7.2.1p8 says the behavior is undefined.  This has no effect in
  // C++.
  Bool3 m_pedanticAllowEmptyStructsInC;

  // When false, prohibit arrays declared to have zero size.  This is
  // invalid per C11 6.7.6.2p1 and C++14 8.3.4p1.
  Bool3 m_pedanticAllowZeroSizeArrays;

  // When false, prohibit initializing a struct (or class) that contains
  // a flexible array member.
  //
  // TODO: This has no effect.  I started to implement it, but ran into
  // trouble and backed it out.
  Bool3 m_pedanticAllowInitializedStructWithFlexibleArrayMember;

  // ---- bug compatibility flags ----
  // gcc-2 bug compatibility: permit string literals to contain
  // (unescaped) newline characters in them
  bool allowNewlinesInStringLits;

  // MSVC bug compatibility: allow implicit int for operator functions
  Bool3 allowImplicitIntForOperators;

  // gcc bug compatibility: allow qualified member declarations
  Bool3 allowQualifiedMemberDeclarations;

  // gcc bug compatibility: allow typedef names to combine with
  // certain type keywords, e.g., "u32 long", in/gnu/dC0014.c;
  // eventually, once the client codes have been fixed, it would be
  // good to delete this, since it involves some extra grammar
  // productions
  Bool3 allowModifiersWithTypedefNames;

  // gcc/msvc bug/extension compatibility: allow anonymous structs;
  // see doc/anon-structs.txt.
  //
  // This only has an effect for C++.  In C mode, anonymous structs are
  // always allowed.
  Bool3 allowAnonymousStructs;

  // gcc-2 bug compatibility: In gcc-2, namespace "std::" is actually
  // an alias for the global scope.  This flag turns on some hacks
  // to accept some code preprocessed with gcc-2 headers.
  bool gcc2StdEqualsGlobalHacks;

  // more gcc-2 bug compat: The gcc-2 headers contain some invalid
  // syntax.  Conceptually, this flag recognizes the invalid syntax
  // and transforms it into valid syntax for Elsa.  Actually, it just
  // enables some hacks that have similar effect.
  Bool3 allowGcc2HeaderSyntax;

  // gcc C-mode bug compat: accept duplicate type specifier keywords
  // like 'int int'
  Bool3 allowRepeatedTypeSpecifierKeywords;

  // gcc C-mode bug compat: silently allow const/volatile to be
  // applied to function types via typedefs; it's meaningless
  Bool3 allowCVAppliedToFunctionTypes;

  // gcc bug compat: gcc does not enforce the rule that a definition
  // must be in a scope that encloses the declaration
  Bool3 allowDefinitionsInWrongScopes;

  // gcc bug compat: in C++ mode, gcc allows prototype parameters to
  // have the same name (in/gnu/bugs/gb0011.cc)
  Bool3 allowDuplicateParameterNames;

  // gcc bug compat: gcc does not require "template <>" is some
  // cases for explicit specializations (in/gnu/bugs/gb0012.cc)
  Bool3 allowExplicitSpecWithoutParams;

  // quarl: declaring (or defining) a function as static after previously
  // declaring it without 'static'. gcc-3.4 allows with warning; gcc-4.0
  // disallows.
  Bool3 allowStaticAfterNonStatic;

  // gcc permissive compat: In C mode, GCC by default allows returning a
  // pointer from a function declared to return an integer.  Test:
  // in/c/t0010.c.
  Bool3 m_allowReturnPointerAsInteger;

private:     // funcs
  void setAllWarnings(bool enable);

public:      // funcs
  CCLang()
    : m_typeSizes()            // Initially matches host compiler.
  {
    ANSI_C89();
  }

  // Set any B3_TRUE to B3_WARN, except for m_pedanticXXX.
  void enableAllWarnings() { setAllWarnings(true); }

  // Set any B3_WARN to B3_TRUE, except for m_pedanticXXX.
  void disableAllWarnings() { setAllWarnings(false); }

  // the following are additive incremental

  // enable gcc C features
  void GNU_C_extensions();

  // enable C99 features
  void ANSI_C99_extensions();

  // enable MSVC bug compatibility
  void MSVC_bug_compatibility();

  // Set all m_pedanticXXX options to 'value'.  This is meant to emulate
  // GCC and Clang "-pedantic" (warnings) and "-pedantic-errors".
  void setPedantic(Bool3 value);

  // The predefined settings below are something of a best-effort at
  // reasonable starting configurations.  Every function below sets
  // *all* of the flags; they are not incremental.  Users are
  // encouraged to explicitly set fields after activating a predefined
  // setting to get a specific setting.

  void KandR_C();           // settings for K&R C
  void ANSI_C89();          // settings for ANSI C89
  void ANSI_C99();          // settings for ANSI C99
  void GNU_C();             // settings for GNU C
  void GNU_KandR_C();       // GNU 3.xx C + K&R compatibility
  void GNU2_KandR_C();      // GNU 2.xx C + K&R compatibility

  void ANSI_Cplusplus();    // settings for ANSI C++ 98
  void GNU_Cplusplus();     // settings for GNU C++

  // dsw: I regret having to mention all of the flags yet one more
  // place, however I think I need this.
  string toString();
};

bool handleExternInline_asPrototype();
bool handleExternInline_asWeakStaticInline();

#endif // ELSA_CC_LANG_H
