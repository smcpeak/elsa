// gnu.cc
// tcheck and print routines for gnu.ast/gnu.gr extensions

#include "cc-ast.h"                    // contains declarations for this module

// elsa
#include "astvisit.h"                  // ASTVisitorEx
#include "cc-env.h"                    // Env
#include "cc-print.h"                  // olayer, PrintEnv
#include "generic_amb.h"               // resolveAmbiguity, etc.
#include "generic_aux.h"               // C++ AST, and genericPrintAmbiguities, etc.
#include "stdconv.h"                   // usualArithmeticConversions
#include "ubermods-attrspec.h"         // aslPrependAS, aslAppendASL

// smbase
#include "strutil.h"                   // streq

// libc++
#include <algorithm>                   // std::max

// libc
#include <string.h>                    // strcmp, strncmp


// fwd in this file
SimpleTypeId constructFloatingType(int prec, int axis);


bool streq_GAN(StringRef astName, char const *attrName)
{
  if (streq(astName, attrName)) {
    return true;
  }

  size_t len = strlen(attrName);
  if (astName[0] == '_' &&
      astName[1] == '_' &&
      0==strncmp(astName+2, attrName, len) &&
      astName[len+2] == '_' &&
      astName[len+3] == '_' &&
      astName[len+4] == 0) {
    return true;
  }

  return false;
}


// --------------------------- Env ---------------------------------
// Caveat: All of the uses of GNU builtin functions arise from
// preprocessing with the gcc compiler's headers.  Strictly speaking,
// this is inappropriate, as Elsa is a different implementation and
// has its own compiler-specific headers (in the include/ directory).
// But in practice people don't often seem to be willing to adjust
// their build process enough to actually use Elsa's headers, and
// insist on using the gcc headers since that's what (e.g.) gcc -E
// finds by default.  Therefore Elsa makes a best-effort attempt to
// accept the resulting files, even though they are gcc-specific (and
// sometimes specific to a particular *version* of gcc).  This
// function is part of that effort.
//
// See  http://gcc.gnu.org/onlinedocs/gcc-3.1/gcc/Other-Builtins.html
// also http://gcc.gnu.org/onlinedocs/gcc-3.4.3/gcc/Other-Builtins.html
void Env::addGNUBuiltins()
{
  Type *t_void = getSimpleType(ST_VOID);
  Type *t_ellipsis_ptr = makePtrType(getSimpleType(ST_ELLIPSIS));
//    Type *t_voidconst = getSimpleType(SL_INIT, ST_VOID, CV_CONST);
//    Type *t_voidptr = makePtrType(t_void);
//    Type *t_voidconstptr = makePtrType(SL_INIT, t_voidconst);

  Type *t_int = getSimpleType(ST_INT);
//    Type *t_unsigned_int = getSimpleType(ST_UNSIGNED_INT);
  Type *t_char = getSimpleType(ST_CHAR);
  Type *t_charconst = getSimpleType(ST_CHAR, CV_CONST);
  Type *t_charptr = makePtrType(t_char);
  Type *t_charconstptr = makePtrType(t_charconst);

  // dsw: This is a form, not a function, since it takes an expression
  // AST node as an argument; however, I need a function that takes no
  // args as a placeholder for it sometimes.
  var__builtin_constant_p = declareSpecialFunction("__builtin_constant_p");

  // typedef void *__builtin_va_list;
  Variable *var__builtin_va_list =
    makeVariable(SL_INIT, str("__builtin_va_list"),
                 // dsw: in Oink, it really helps if the type is
                 // ST_ELLIPSIS instead of void*; explanation upon
                 // request; UPDATE: ok, that doesn't let d0125.cc
                 // typecheck so how about a pointer to an ST_ELLIPSIS
//                  t_voidptr, DF_TYPEDEF | DF_BUILTIN);
                 t_ellipsis_ptr, DF_TYPEDEF | DF_BUILTIN);
  addVariable(var__builtin_va_list);
  env.builtinVars.push(var__builtin_va_list);

  // void __builtin_stdarg_start(__builtin_va_list __list, char const *__format);
  // trying this instead:
  // void __builtin_stdarg_start(__builtin_va_list __list, void const *__format);
  // nope; see in/d0120.cc.  It doesn't work if the arg to '__format' is an int.
  // ironically, making it vararg does work
  declareFunction1arg(t_void, "__builtin_stdarg_start",
                      var__builtin_va_list->type, "__list",
//                        t_charconstptr, "__format",
//                        t_voidconstptr, "__format",
                      FF_VARARGS, NULL);


  // varargs; dsw: I think that we should make all of these their own
  // AST node, I just don't want to deal with the parsing ambiguity
  // with E_funCall right now
  // void __builtin_va_start(__builtin_va_list __list, ...);
  declareFunction1arg(t_void, "__builtin_va_start",
                      var__builtin_va_list->type, "__list",
                      FF_VARARGS, NULL);
  // void __builtin_va_copy(__builtin_va_list dest, __builtin_va_list src);
  declareFunction2arg(t_void, "__builtin_va_copy",
                      var__builtin_va_list->type, "dest",
                      var__builtin_va_list->type, "src",
                      FF_NONE, NULL);
  // void __builtin_va_end(__builtin_va_list __list);
  declareFunction1arg(t_void, "__builtin_va_end",
                      var__builtin_va_list->type, "__list");


  // // void *__builtin_alloca(unsigned int __len);
  // declareFunction1arg(t_voidptr, "__builtin_alloca",
  //                     t_unsigned_int, "__len");

  // char *__builtin_strchr(char const *str, int ch);
  declareFunction2arg(t_charptr, "__builtin_strchr",
                      t_charconstptr, "str",
                      t_int, "ch",
                      FF_NONE, NULL);

  // char *__builtin_strpbrk(char const *str, char const *accept);
  declareFunction2arg(t_charptr, "__builtin_strpbrk",
                      t_charconstptr, "str",
                      t_charconstptr, "accept",
                      FF_NONE, NULL);

  // char *__builtin_strchr(char const *str, int ch);
  declareFunction2arg(t_charptr, "__builtin_strrchr",
                      t_charconstptr, "str",
                      t_int, "ch",
                      FF_NONE, NULL);

  // char *__builtin_strstr(char const *haystack, char const *needle);
  declareFunction2arg(t_charptr, "__builtin_strstr",
                      t_charconstptr, "haystack",
                      t_charconstptr, "needle",
                      FF_NONE, NULL);

  // we made some attempts to get accurate prototypes for the above
  // functions, but at some point just started using "int ()(...)"
  // as the type; the set below all get this generic type

  static char const * const arr[] = {
    // ------------------------------------------------
    // group 1: "Outside strict ISO C mode ..."

    // this set is from the 3.1 list

    // quarl 2006-09-07
    //    The above prototype causes linking to fail.  Use this until
    //    builtin-declarations.h is used.

    "alloca",              // prototyped above
    "bcmp",
    "bzero",
    "index",
    "rindex",
    "ffs",
    "fputs_unlocked",
    "printf_unlocked",
    "fprintf_unlocked",

    // this set is from the 3.4.3 list; the commented lines are
    // either prototyped above or in the 3.1 list
    "_exit",
    //"alloca",
    //"bcmp",
    //"bzero",
    "dcgettext",
    "dgettext",
    "dremf",
    "dreml",
    "drem",
    "exp10f",
    "exp10l",
    "exp10",
    "ffsll",
    "ffsl",
    //"ffs",
    //"fprintf_unlocked",
    //"fputs_unlocked",
    "gammaf",
    "gammal",
    "gamma",
    "gettext",
    //"index",
    "j0f",
    "j0l",
    "j0",
    "j1f",
    "j1l",
    "j1",
    "jnf",
    "jnl",
    "jn",
    "mempcpy",
    "pow10f",
    "pow10l",
    "pow10",
    //"printf_unlocked",
    //"rindex",
    "scalbf",
    "scalbl",
    "scalb",
    "significandf",
    "significandl",
    "significand",
    "sincosf",
    "sincosl",
    "sincos",
    "stpcpy",
    "strdup",
    "strfmon",
    "y0f",
    "y0l",
    "y0",
    "y1f",
    "y1l",
    "y1",
    "ynf",
    "ynl",
    "yn",

    // ------------------------------------------------
    // group 2: "The ISO C99 functions ..."

    // this is the 3.1 list
    "conj",
    "conjf",
    "conjl",
    "creal",
    "crealf",
    "creall",
    "cimag",
    "cimagf",
    "cimagl",
    "llabs",
    "imaxabs",

    // this is the 3.4.3 list, with those from 3.1 commented
    "_Exit",
    "acoshf",
    "acoshl",
    "acosh",
    "asinhf",
    "asinhl",
    "asinh",
    "atanhf",
    "atanhl",
    "atanh",
    "cabsf",
    "cabsl",
    "cabs",
    "cacosf",
    "cacoshf",
    "cacoshl",
    "cacosh",
    "cacosl",
    "cacos",
    "cargf",
    "cargl",
    "carg",
    "casinf",
    "casinhf",
    "casinhl",
    "casinh",
    "casinl",
    "casin",
    "catanf",
    "catanhf",
    "catanhl",
    "catanh",
    "catanl",
    "catan",
    "cbrtf",
    "cbrtl",
    "cbrt",
    "ccosf",
    "ccoshf",
    "ccoshl",
    "ccosh",
    "ccosl",
    "ccos",
    "cexpf",
    "cexpl",
    "cexp",
    //"cimagf",
    //"cimagl",
    //"cimag",
    //"conjf",
    //"conjl",
    //"conj",
    "copysignf",
    "copysignl",
    "copysign",
    "cpowf",
    "cpowl",
    "cpow",
    "cprojf",
    "cprojl",
    "cproj",
    //"crealf",
    //"creall",
    //"creal",
    "csinf",
    "csinhf",
    "csinhl",
    "csinh",
    "csinl",
    "csin",
    "csqrtf",
    "csqrtl",
    "csqrt",
    "ctanf",
    "ctanhf",
    "ctanhl",
    "ctanh",
    "ctanl",
    "ctan",
    "erfcf",
    "erfcl",
    "erfc",
    "erff",
    "erfl",
    "erf",
    "exp2f",
    "exp2l",
    "exp2",
    "expm1f",
    "expm1l",
    "expm1",
    "fdimf",
    "fdiml",
    "fdim",
    "fmaf",
    "fmal",
    "fmaxf",
    "fmaxl",
    "fmax",
    "fma",
    "fminf",
    "fminl",
    "fmin",
    "hypotf",
    "hypotl",
    "hypot",
    "ilogbf",
    "ilogbl",
    "ilogb",
    //"imaxabs",
    "lgammaf",
    "lgammal",
    "lgamma",
    //"llabs",
    "llrintf",
    "llrintl",
    "llrint",
    "llroundf",
    "llroundl",
    "llround",
    "log1pf",
    "log1pl",
    "log1p",
    "log2f",
    "log2l",
    "log2",
    "logbf",
    "logbl",
    "logb",
    "lrintf",
    "lrintl",
    "lrint",
    "lroundf",
    "lroundl",
    "lround",
    "nearbyintf",
    "nearbyintl",
    "nearbyint",
    "nextafterf",
    "nextafterl",
    "nextafter",
    "nexttowardf",
    "nexttowardl",
    "nexttoward",
    "remainderf",
    "remainderl",
    "remainder",
    "remquof",
    "remquol",
    "remquo",
    "rintf",
    "rintl",
    "rint",
    "roundf",
    "roundl",
    "round",
    "scalblnf",
    "scalblnl",
    "scalbln",
    "scalbnf",
    "scalbnl",
    "scalbn",
    "snprintf",
    "tgammaf",
    "tgammal",
    "tgamma",
    "truncf",
    "truncl",
    "trunc",
    "vfscanf",
    "vscanf",
    "vsnprintf",
    "vsscanf",

    // ------------------------------------------------
    // group 3: "There are also built-in versions of the ISO C99 functions ..."

    // 3.1 list
    "cosf",
    "cosl",
    "fabsf",
    "fabsl",
    "sinf",
    "sinl",
    "sqrtf",
    "sqrtl",

    // 3.4.3 list with 3.1 elements commented
    "acosf",
    "acosl",
    "asinf",
    "asinl",
    "atan2f",
    "atan2l",
    "atanf",
    "atanl",
    "ceilf",
    "ceill",
    //"cosf",
    "coshf",
    "coshl",
    //"cosl",
    "expf",
    "expl",
    //"fabsf",
    //"fabsl",
    "floorf",
    "floorl",
    "fmodf",
    "fmodl",
    "frexpf",
    "frexpl",
    "ldexpf",
    "ldexpl",
    "log10f",
    "log10l",
    "logf",
    "logl",
    "modfl",
    "modf",
    "powf",
    "powl",
    //"sinf",
    "sinhf",
    "sinhl",
    //"sinl",
    //"sqrtf",
    //"sqrtl",
    "tanf",
    "tanhf",
    "tanhl",
    "tanl",

    // gcc-3.4.3 seems to have this, though it is not documented
    "modff",

    // same for these
    "huge_val",
    "huge_valf",
    "huge_vall",
    "nan",

    // ------------------------------------------------
    // group 4: "The ISO C90 functions ..."

    // this is the 3.1 list, with things prototyped above commented
    "abs",
    "cos",
    "fabs",
    "fprintf",
    "fputs",
    "labs",
    "memcmp",
    "memcpy",
    "memset",
    "printf",
    "sin",
    "sqrt",
    "strcat",
    //"strchr",
    "strcmp",
    "strcpy",
    "strcspn",
    "strlen",
    "strncat",
    "strncmp",
    "strncpy",
    //"strpbrk",
    //"strrchr",
    "strspn",
    //"strstr",

    // this is the 3.4.3 list, with things either prototyped above or
    // in the 3.1 list commented
    "abort",
    //"abs",
    "acos",
    "asin",
    "atan2",
    "atan",
    "calloc",
    "ceil",
    "cosh",
    //"cos",
    "exit",
    "exp",
    //"fabs",
    "floor",
    "fmod",
    //"fprintf",
    //"fputs",
    "frexp",
    "fscanf",
    //"labs",
    "ldexp",
    "log10",
    "log",
    "malloc",
    //"memcmp",
    //"memcpy",
    //"memset",
    "modf",
    "pow",
    "powi",
    "powif",
    "powil",
    //"printf",
    "putchar",
    "puts",
    "scanf",
    "sinh",
    //"sin",
    "snprintf",
    "sprintf",
    //"sqrt",
    "sscanf",
    //"strcat",
    //"strchr",
    //"strcmp",
    //"strcpy",
    //"strcspn",
    //"strlen",
    //"strncat",
    //"strncmp",
    //"strncpy",
    //"strpbrk",
    //"strrchr",
    //"strspn",
    //"strstr",
    "tanh",
    "tan",
    "vfprintf",
    "vprintf",
    "vsprintf",

    // ------------------------------------------------
    // group 5: "... ISO C99 floating point comparison macros ..."

    // this is the 3.1 list, which is identical to the 3.4.3 list
    "isgreater",
    "isgreaterequal",
    "isless",
    "islessequal",
    "islessgreater",
    "isunordered",

    // ------------------------------------------------
    // group 6: miscellaneous compiler interrogations/hints

    // types_compatible_p: not yet implemented in Elsa
    // choose_expr: not yet implemented in Elsa
    // constant_p: implemented elsewhere
    // expect: implemented elsewhere
    "prefetch",

    // ------------------------------------------------
    // group 7: low-level arithmetic stuff

    // full prototypes:
    //   float __builtin_nanf (const char *str);
    //   long double __builtin_nanl (const char *str);
    //   double __builtin_nans (const char *str);
    //   float __builtin_nansf (const char *str);
    //   long double __builtin_nansl (const char *str);
    //   int __builtin_ffs (unsigned int x);
    //   int __builtin_clz (unsigned int x);
    //   int __builtin_ctz (unsigned int x);
    //   int __builtin_popcount (unsigned int x);
    //   int __builtin_parity (unsigned int x);
    //   int __builtin_ffsl (unsigned long);
    //   int __builtin_clzl (unsigned long);
    //   int __builtin_ctzl (unsigned long);
    //   int __builtin_popcountl (unsigned long);
    //   int __builtin_parityl (unsigned long);
    //   int __builtin_ffsll (unsigned long long);
    //   int __builtin_clzll (unsigned long long);
    //   int __builtin_ctzll (unsigned long long);
    //   int __builtin_popcountll (unsigned long long);
    //   int __builtin_parityll (unsigned long long);

    // just the names, but those that appear above are commented
    "nanf",
    "nanl",
    "nans",
    "nansf",
    "nansl",
    //"ffs",
    "clz",
    "ctz",
    "popcount",
    "parity",
    //"ffsl",
    "clzl",
    "ctzl",
    "popcountl",
    "parityl",
    //"ffsll",
    "clzll",
    "ctzll",
    "popcountll",
    "parityll",
  };

  for (int i=0; i < TABLESIZE(arr); i++) {
    Variable *v = makeImplicitDeclFuncVar(str(stringc << "__builtin_" << arr[i]));
    env.builtinVars.push(v);
  }

  // initialize 'complexComponentFields'
  for (int axis=0; axis<=1; axis++) {
    for (int prec=0; prec<=2; prec++) {
      StringRef n = axis==0? string_realSelector : string_imagSelector;
      Type *t = env.getSimpleType(constructFloatingType(prec, axis));
      Variable *v = makeVariable(SL_INIT, n, t, DF_BUILTIN);
      complexComponentFields[axis][prec] = v;
    }
  }
}


// -------------------- tcheck --------------------
ASTTypeof *ASTTypeof::tcheck(Env &env, DeclFlags dflags)
{
  if (!ambiguity) {
    mid_tcheck(env, dflags);
    return this;
  }

  return resolveAmbiguity(this, env, "ASTTypeof", false /*priority*/, dflags);
}

void ASTTypeof::mid_tcheck(Env &env, DeclFlags &dflags)
{
  type = itcheck(env, dflags);
}


Type *TS_typeof_expr::itcheck(Env &env, DeclFlags dflags)
{
  // FIX: dflags discarded?
  expr->tcheck(env);
  // FIX: check the asRval(); A use in kernel suggests it should be
  // there as otherwise you get "error: cannot create a pointer to a
  // reference" when used to specify the type in a declarator that
  // comes from a de-reference (which yeilds a reference).
  return expr->getType()->asRval();
}


Type *TS_typeof_type::itcheck(Env &env, DeclFlags dflags)
{
  ASTTypeId::Tcheck tc(DF_NONE /*dflags don't apply to this type*/,
                       DC_TS_TYPEOF_TYPE);
  atype = atype->tcheck(env, tc);
  Type *t = atype->getType();
  return t;
}


Type *TS_typeof::itcheck(Env &env, Tcheck &tc)
{
  atype = atype->tcheck(env, tc.m_dflags);
  return atype->type;
}


void S_function::itcheck(Env &env)
{
  env.setLoc(loc);
  f->tcheck(env);
}


void S_rangeCase::itcheck(Env &env)
{
  exprLo->tcheck(env, exprLo);
  exprHi->tcheck(env, exprHi);
  s = s->tcheck(env);

  // compute case label values
  exprLo->constEval(env, labelValLo);
  exprHi->constEval(env, labelValHi);
}

void S_computedGoto::itcheck(Env &env)
{
  target->tcheck(env, target);

  // The GCC manual seems to imply it wants 'target' to have type
  // 'void*'.  It seems pointless to specifically require void* as
  // opposed to some other pointer type, since any other pointer type
  // can be implicitly converted to void*.  Even so, EDG does in fact
  // enforce that the arg is exactly void*.  GCC itself does not
  // appear to enforce any restrictions on the type (!).
  Type *t = target->type->asRval();
  if (!t->isPointer()) {
    env.error(t, stringc
      << "type of expression in computed goto must be a pointer, not '"
      << t->toString() << "'");
  }
}


void AD_gnu::tcheck(Env &env)
{
  text->tcheck_strlit(env);

  FOREACH_ASTLIST_NC(GNUAsmOperand, outputOperands, iter) {
    iter.data()->tcheck(env, false /*isInputOperand*/);
  }
  FOREACH_ASTLIST_NC(GNUAsmOperand, inputOperands, iter) {
    iter.data()->tcheck(env, true /*isInputOperand*/);
  }
  FOREACH_ASTLIST_NC(E_stringLit, clobbers, iter) {
    iter.data()->tcheck_strlit(env);
  }
}


void GNUAsmOperand::tcheck(Env &env, bool isInputOperand)
{
  constraint->tcheck_strlit(env);
  expr->tcheck(env, expr);

  if (!isInputOperand) {
    Type *t = expr->type;
    if (!t->isLval()) {
      env.error(t, stringb(
        "asm output operand '" << expr->asString() <<
        "' must be an lvalue"));
    }
  }
}


Type *E_compoundLit::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_COMPOUNDLIT);

  // typechedk the type only once, and isolated from ambiguities
  if (!tcheckedType) {
    InstantiationContextIsolator isolate(env, env.loc());
    tcheckedType = true;

    stype = stype->tcheck(env, tc);
  }

  Type *type = stype->getType();
  m_semanticInit = init->tcheck(env, type);
  finalizeSemanticInitializer(env, m_semanticInit);

  type = legacyTypeNC(refineTypeFromInitializer(m_semanticInit, type));

  // dsw: Scott says: "The gcc manual says nothing about whether a
  // compound literal is an lvalue.  But, compound literals are now
  // part of C99 (6.5.2.5), which says they are indeed lvalues (but
  // says nothing about being const)."
  return env.makeReferenceType(type);
}


Type *E___builtin_constant_p::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);

//    // TODO: this will fail an assertion if someone asks for the
//    // size of a variable of template-type-parameter type..
//    // dsw: If this is turned back on, be sure to catch the possible
//    // XReprSize exception and add its message to the env.error-s
//    size = expr->type->asRval()->reprSize(env.lang.m_typeSizes);
//    TRACE("sizeof", "sizeof(" << expr->exprToString() <<
//                    ") is " << size);

  // dsw: the type of a __builtin_constant_p is an int:
  // http://gcc.gnu.org/onlinedocs/gcc-3.2.2/gcc/Other-Builtins.html#Other%20Builtins
  // TODO: is this right?
  return expr->type->isError()?
           expr->type : env.getSimpleType(ST_UNSIGNED_INT);
}


Type *E___builtin_va_arg::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_BUILTIN_VA_ARG);
  expr->tcheck(env, expr);
  atype = atype->tcheck(env, tc);
  return atype->getType();
}


Type *E_alignofType::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_ALIGNOFTYPE);
  atype = atype->tcheck(env, tc);
  Type *t = atype->getType();

  // just assume that the type's size is its alignment; this may
  // be a little conservative for 'double', and will be wrong for
  // large structs, but at the moment it does not seem worthwhile
  // to delve into the details of accurately computing this
  return env.sizeofType(t, alignment, NULL /*expr*/);
}


Type *E_alignofExpr::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);

  // as above, assume size=alignment
  return env.sizeofType(expr->type, alignment, expr);
}


Type *E_statement::itcheck_x(Env &env, Expression *&replacement)
{
  // An E_statement can contain declarations, and tchecking a
  // declaration modifies the environment.  But expressions can occur
  // in ambiguous contexts, and hence their tcheck should not modify
  // the environment.
  //
  // Since the E_statements are themselves interpreted independently
  // of such contexts, tcheck each E_statement exactly once.  Each
  // ambiguous alternative will use the same interpretation.
  //
  // This avoids problems with e.g. in/gnu/c0001.c
  if (!tchecked) {

    // having committed to tchecking here, isolate these actions
    // from the context
    InstantiationContextIsolator isolate(env, env.loc());

    s = s->tcheck(env)->asS_compound();

    tchecked = true;
  }

  if (s->stmts.isNotEmpty()) {
    Statement *last = s->stmts.last();
    if (last->isS_expr()) {
      return last->asS_expr()->expr->getType();
    }
  }

  return env.getSimpleType(ST_VOID, CV_NONE);
}


Type *E_gnuCond::itcheck_x(Env &env, Expression *&replacement)
{
  cond->tcheck(env, cond);
  el->tcheck(env, el);

  // presumably the correct result type is some sort of intersection
  // of the 'cond' and 'el' types?

  return el->type;
}


Type *E_addrOfLabel::itcheck_x(Env &env, Expression *&replacement)
{
  // TODO: check that the label exists in the function

  // type is void*
  return env.makePtrType(env.getSimpleType(ST_VOID));
}


// decompose a real/imaginary/complex type:
//   prec: -1=integral, 0=float, 1=double, 2=longdouble
//   axis: 0=real, 1=imag, 2=complex
// return false if not among the nine floating types
bool dissectFloatingType(int &prec, int &axis, Type *t)
{
  t = t->asRval();

  if (!t->isSimpleType()) {
    return false;
  }
  SimpleTypeId id = t->asSimpleTypeC()->type;

  switch (id) {
    case ST_FLOAT:                  prec=0; axis=0; return true;
    case ST_DOUBLE:                 prec=1; axis=0; return true;
    case ST_LONG_DOUBLE:            prec=2; axis=0; return true;

    case ST_FLOAT_IMAGINARY:        prec=0; axis=1; return true;
    case ST_DOUBLE_IMAGINARY:       prec=1; axis=1; return true;
    case ST_LONG_DOUBLE_IMAGINARY:  prec=2; axis=1; return true;

    case ST_FLOAT_COMPLEX:          prec=0; axis=2; return true;
    case ST_DOUBLE_COMPLEX:         prec=1; axis=2; return true;
    case ST_LONG_DOUBLE_COMPLEX:    prec=2; axis=2; return true;

    default:
      if (isIntegerType(id)) {
        prec = -1;
        axis = 0;
        return true;
      }
      else {
        return false;
      }
  }
}

SimpleTypeId constructFloatingType(int prec, int axis)
{
  static SimpleTypeId const map[3/*axis*/][3/*prec*/] = {
    { ST_FLOAT, ST_DOUBLE, ST_LONG_DOUBLE },
    { ST_FLOAT_IMAGINARY, ST_DOUBLE_IMAGINARY, ST_LONG_DOUBLE_IMAGINARY },
    { ST_FLOAT_COMPLEX, ST_DOUBLE_COMPLEX, ST_LONG_DOUBLE_COMPLEX }
  };

  xassert((unsigned)axis < 3);
  xassert((unsigned)prec < 3);

  return map[axis][prec];
}


Type *E_fieldAcc::itcheck_complex_selector(Env &env, LookupFlags flags,
                                           LookupSet &candidates)
{
  int isImag = fieldName->getName()[2] == 'i';

  int prec, axis;
  if (!dissectFloatingType(prec, axis, obj->type) ||
      prec == -1/*integral*/ ||
      axis != 2/*complex*/) {
    return env.error(stringc << "can only apply " << fieldName->getName()
                             << " to complex types, not '"
                             << obj->type->toString() << "'");
  }

  field = env.complexComponentFields[isImag][prec];
  return env.tfac.makeReferenceType(field->type);
}


Type *E_binary::itcheck_complex_arith(Env &env)
{
  int prec1, axis1;
  int prec2, axis2;
  if (!dissectFloatingType(prec1, axis1, e1->type) ||
      !dissectFloatingType(prec2, axis2, e2->type)) {
    return env.error(stringc << "invalid complex arithmetic operand types '"
                             << e1->type->toString() << "' and '"
                             << e2->type->toString() << "'");
  }

  // NOTE: The following computations have not been thoroughly tested.

  // result precision: promote to larger
  int prec = std::max(prec1, prec2);

  // result axis
  int axis;
  switch (op) {
    case BIN_EQUAL:
    case BIN_NOTEQUAL:
    case BIN_LESS:
    case BIN_GREATER:
    case BIN_LESSEQ:
    case BIN_GREATEREQ:
      return env.getBooleanOperatorResultType();

    case BIN_PLUS:
    case BIN_MINUS:
      if (axis1 == axis2) {
        axis = axis1;
      }
      else {
        axis = 2/*complex*/;
      }
      break;

    case BIN_MULT:
    case BIN_DIV:
      if (axis1 + axis2 == 0) {
        axis = 0/*real*/;     // but then how'd we reach this code anyway?
      }
      else if (axis1 + axis2 == 1) {
        axis = 1/*imag*/;
      }
      else {
        axis = 2/*complex*/;
      }
      break;

    default:
      // who the heck knows
      axis = 2/*complex*/;
      break;
  }

  // result id
  return env.getSimpleType(constructFloatingType(prec, axis));
}


// Advance 'iter' by as many elements as would be consumed in order to
// initialize 'type'.  This must always advance it at least once,
// otherwise there is a risk of the caller going into an infinite loop,
// but do not advance it not beyond the done point of course.
//
// TODO: This replicates logic in cc-tcheck.cc, initializeAggregate().
// Ideally these should be folded together.  The problem at the moment
// is 'initializeAggregate' is kind of broken (I think), so I'm sort of
// experimenting with implementing it again (here) in hopes of learning
// how to do this properly.  But the job here, of just advancing the
// iterator, is only a portion of what 'initializeAggregate' does, so
// the insight gained is limited...
//
static void advanceInitIterForType(Env &env,
  ASTListIter<Initializer> &iter, Type const *type)
{
  xassert(!iter.isDone());

  if (iter.data()->isIN_compound()) {
    // A brace-enclosed initializer matches 'type' exactly.
    iter.adv();
    return;
  }

  if (ArrayType const *arrayType = type->ifArrayTypeC()) {
    int size = arrayType->getSize();
    if (size == 0) {
      env.error("Nested array type has zero size.");
      size = 1;      // Advance at least once.
    }

    // This loop deliberately runs without limit on 'i' if 'size' is
    // negative (unspecified).
    for (int i=0; i != size && !iter.isDone(); i++) {
      advanceInitIterForType(env, iter, arrayType->eltType);
    }
  }

  else if (CompoundType const *ct = type->ifCompoundTypeC()) {
    if (ct->isAggregate()) {
      if (ct->dataMembers.isEmpty()) {
        env.error("Nested compound type has no members.");
        iter.adv();    // Advance at least once.
      }
      else {
        SFOREACH_OBJLIST(Variable, ct->dataMembers, membIter) {
          advanceInitIterForType(env, iter, membIter.data()->type);
          if (iter.isDone()) {
            break;
          }
        }
      }
    }
    else {
      // Non-"aggregate" classes have a constructor and consume one
      // initializer.
      iter.adv();
    }
  }

  else {
    // Assume the value is initialized by one initializer.
    iter.adv();
  }
}


// ------------------ const-eval, etc. -------------------
CValue E_alignofType::extConstEval(ConstEval &env) const
{
  CValue ret;
  ret.setUnsigned(ST_UNSIGNED_INT, alignment);
  return ret;
}


CValue E_alignofExpr::extConstEval(ConstEval &env) const
{
  CValue ret;
  ret.setUnsigned(ST_UNSIGNED_INT, alignment);
  return ret;
}


CValue E_gnuCond::extConstEval(ConstEval &env) const
{
  CValue v = cond->constEval(env);
  if (v.isSticky()) {
    return v;
  }

  if (!v.isZero()) {
    return v;
  }
  else {
    return el->constEval(env);
  }
}

bool E_gnuCond::extHasUnparenthesizedGT()
{
  return hasUnparenthesizedGT(cond) ||
         hasUnparenthesizedGT(el);
}


// ------------------------ print --------------------------
void TS_typeof::iprint(PrintEnv &env) const
{
  env << "typeof(";

  ASTSWITCH(ASTTypeof, atype) {
    ASTCASE(TS_typeof_expr, e) {
      e->expr->print(env);
    }

    ASTNEXT(TS_typeof_type, t) {
      t->atype->print(env);
    }

    ASTENDCASED
  }

  env << ")";
}


void ASTTypeof::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "TypeSpecifier", os, indent);

  // sm: what was this here for?
  //genericCheckNexts(this);
}


void ASTTypeof::addAmbiguity(ASTTypeof *alt)
{
  //genericAddAmbiguity(this, alt);

  // insert 'alt' at the head of the 'ambiguity' list
  xassert(alt->ambiguity == NULL);
  alt->ambiguity = ambiguity;
  ambiguity = alt;
}


void S_function::iprint(PrintEnv &env, StatementContext) const
{
  f->print(env);
}


void S_rangeCase::iprint(PrintEnv &env, StatementContext) const
{
  env << env.und << "case ";
  exprLo->print(env, OPREC_LOWEST);
  env << " ... ";
  exprHi->print(env, OPREC_LOWEST);
  env << ":" << env.br;

  s->print(env, SC_RANGE_CASE);
}


void S_computedGoto::iprint(PrintEnv &env, StatementContext) const
{
  env << "goto *";
  target->print(env, OPREC_PREFIX);
  env << ";";
}


void AD_gnu::print(PrintEnv &env) const
{
  env << env.asmKeywordSpelling();
  if (cv != CV_NONE) {
    env << " " << toString(cv) << " ";
  }
  env << "(";

  env.begin(0 /*indent*/, true /*consistentBreaks*/);

  text->print(env, OPREC_LOWEST);

  int ct;

  ct=0;
  env << env.br << ":";
  FOREACH_ASTLIST(GNUAsmOperand, outputOperands, iter) {
    env << (ct++? ", " : " ");
    iter.data()->print(env);
  }

  if (inputOperands.isNotEmpty() || clobbers.isNotEmpty()) {
    ct=0;
    env << env.br << ":";
    FOREACH_ASTLIST(GNUAsmOperand, inputOperands, iter) {
      env << (ct++? ", " : " ");
      iter.data()->print(env);
    }

    if (clobbers.isNotEmpty()) {
      ct=0;
      env << env.br << ":";
      FOREACH_ASTLIST(E_stringLit, clobbers, iter) {
        env << (ct++? ", " : " ");
        iter.data()->print(env, OPREC_COMMA);
      }
    }
  }

  env.end();

  env << ");";
}


void GNUAsmOperand::print(PrintEnv &env) const
{
  if (asmSymbolicName) {
    env << "[" << asmSymbolicName << "] ";
  }
  constraint->print(env, OPREC_COMMA);
  env << " (";
  expr->print(env, OPREC_LOWEST);
  env << ")";
}


void E_compoundLit::iprint(PrintEnv &env) const
{
  {
    env << "(";
    stype->print(env);
    env << ")";
  }

  bool const outermost = true;
  env.selectInitializer(init, m_semanticInit)->print(env, outermost);
}

OperatorPrecedence E_compoundLit::getPrecedence() const
{
  // I'm not sure about this one.
  return OPREC_PREFIX;
}


void E___builtin_constant_p::iprint(PrintEnv &env) const
{
  env << "__builtin_constant_p(";
  expr->print(env, OPREC_LOWEST);
  env << ")";
}

OperatorPrecedence E___builtin_constant_p::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E___builtin_va_arg::iprint(PrintEnv &env) const
{
  env << "__builtin_va_arg(";
  expr->print(env, OPREC_LOWEST);
  env << ", ";
  atype->print(env);
  env << ")";
}

OperatorPrecedence E___builtin_va_arg::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_alignofType::iprint(PrintEnv &env) const
{
  env << "__alignof__(";
  atype->print(env);
  env << ")";
}

OperatorPrecedence E_alignofType::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_alignofExpr::iprint(PrintEnv &env) const
{
  env << "__alignof__(";
  expr->print(env, OPREC_LOWEST);
  env << ")";
}

OperatorPrecedence E_alignofExpr::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_statement::iprint(PrintEnv &env) const
{
  env << "(";
  s->iprint(env, SC_STMT_EXPR);
  env << ")";
}

OperatorPrecedence E_statement::getPrecedence() const
{
  return OPREC_HIGHEST;
}


void E_gnuCond::iprint(PrintEnv &env) const
{
  cond->print(env, this->getPrecedence());
  env << " ?: ";
  el->print(env, this->getPrecedence());
}

OperatorPrecedence E_gnuCond::getPrecedence() const
{
  return OPREC_ASSIGN;
}


void E_addrOfLabel::iprint(PrintEnv &env) const
{
  env << "&&" << labelName;
}

OperatorPrecedence E_addrOfLabel::getPrecedence() const
{
  return OPREC_PREFIX;
}


// ------------------------ cfg --------------------------

// WARNING: The control flow graph will show that the statement before
// the S_function flows into the S_function and that the S_function
// flows into the next statement.  If you know that an S_function is
// just a function definition and does nothing at run time, this is
// harmless, but it is a little odd, as in reality control would jump
// over the S_function.  The only way to prevent this that I can see
// would be for cfg.cc:Statement::computeCFG() to know about
// S_function which would eliminate the usefulness of having it in the
// gnu extension, or for S_function::icfg to go up and do some surgery
// on edges that have already been added, which I consider to be too
// weird.
//
// Scott says: "the entire S_function::icfg can be empty, just like
// S_skip."
void S_function::icfg(CFGEnv &env) {}

void S_rangeCase::icfg(CFGEnv &env)
{
  env.connectEnclosingSwitch(this, "'case'");
  s->computeCFG(env);
}


void S_computedGoto::icfg(CFGEnv &env)
{
  // The CFG mechanism is not really prepared to deal with computed
  // gotos, so I will do nothing here (so, the CFG will look like
  // control simply flows to the next statement).  It will fall to the
  // client to realize that this is a computed goto, and try to do
  // something appropriate.
}


// -------------------------- Declarator -------------------------------
void Declarator::ext_pre_print_gnu(PrintEnv &env) const
{
  if (decl->m_attrSpecList &&
      decl->isD_grouping()) {
    // A top-level grouping declarator would not normally print a pair
    // of parentheses because they would be redundant according to the
    // standard grammar.  But if there are attributes associated with
    // the D_grouping, and it is the first of multiple declarators in a
    // declaration, the parentheses are required in order to keep the
    // attribute associated with just that declarator.  See
    // test/pprint/gnu-attr-in-declarator.c.
    //
    // This also fixes a minor potential issue with achieving a
    // pretty-printing fixpoint, since even with only one declarator, if
    // we drop the parens and therefore associate the attribute with the
    // declaration, it prints in a different location.
    env << "(";
  }
}

void Declarator::ext_post_print_gnu(PrintEnv &env) const
{
  if (decl->m_attrSpecList &&
      decl->isD_grouping()) {
    env << ")";
  }
}


// -------------------------- IDeclarator ------------------------------
void IDeclarator::prependASL(AttributeSpecifierList *list)
{
  m_attrSpecList = aslAppendASL(list, m_attrSpecList);
}

void IDeclarator::appendASL(AttributeSpecifierList * /*nullable*/ list)
{
  m_attrSpecList = aslAppendASL(m_attrSpecList, list);
}


void IDeclarator::ext_print_attrSpecList(PrintEnv &env) const
{
  if (m_attrSpecList) {
    if (this->isD_grouping()) {
      // We don't need additional space in front because the parent
      // will have added space in anticipation of following syntax,
      // and the grouping doesn't print any itself.
    }
    else {
      env << env.sp;
    }

    m_attrSpecList->print(env);

    if (this->isD_array() || this->isD_func()) {
      // For suffix declarators, we don't need the extra space.
    }
    else if (this->isD_name() || this->isD_bitfield()) {
      // Also don't need it for these since the declarator is over.
    }
    else {
      env << env.sp;
    }
  }
}


void IDeclarator::ext_tcheck_attrSpecList(Env &env, Declarator::Tcheck &tc)
{
  // tcheck the attribute list.
  if (m_attrSpecList) {
    AttributeSpecifierList::Tcheck asltc(tc.type);
    m_attrSpecList->tcheck(env, asltc);

    if (asltc.m_gnuAliasTarget) {
      tc.gnuAliasTarget = asltc.m_gnuAliasTarget;
    }
  }
}


// --------------------- AttributeSpecifierList ------------------------
void AttributeSpecifierList::print(PrintEnv &env) const
{
  spec->print(env);

  if (next) {
    env << env.sp;
    next->print(env);
  }
}


class AttributeDisambiguator : public ASTVisitorEx {
public:
  virtual void foundAmbiguous(void *obj, void **ambig, char const *kind) override;
};

void AttributeDisambiguator::foundAmbiguous(void *obj, void **ambig, char const *kind)
{
  // I have virtually no basis for doing actual disambiguation of
  // __attribute__ arguments, and no expectation that I will need any.
  // I will just arbitrarily throw away all of the alternatives beyond
  // the first.
  *ambig = NULL;
}


void AttributeSpecifierList::tcheck(Env &env,
  AttributeSpecifierList::Tcheck &tc)
{
  // "disambiguate" the attribute list
  AttributeDisambiguator dis;
  this->traverse(dis);

  // True if we see 'transparent_union'.
  bool transparentUnion = false;

  // Argument to 'mode'.
  StringRef modeDesignator = NULL;

  // Argument to 'alias'.
  StringRef foundAlias = NULL;

  // Scan the attributes for things we recognize.
  for (AttributeSpecifierList *l = this; l; l = l->next) {
    for (AttributeSpecifier *s = l->spec; s; s = s->next) {
      if (AT_word *w = s->attr->ifAT_word()) {
        if (streq_GAN(w->w, "transparent_union")) {
          transparentUnion = true;
        }
      }
      else if (AT_func *f = s->attr->ifAT_func()) {
        if (streq_GAN(f->f, "mode") && f->args) {
          Expression *e = fl_first(f->args)->expr;
          if (e->isE_variable()) {
            // The argument to 'mode' is an identifier, which my
            // parser classifies as a "variable".
            modeDesignator = e->asE_variable()->name->getName();
          }
        }

        if (streq_GAN(f->f, "alias")) {
          if (foundAlias) {
            env.error("More than one alias attribute.");
          }
          else {
            if (fl_count(f->args) != 1) {
              env.error("Too many arguments to alias attribute.");
            }
            else {
              Expression *&expr = fl_first(f->args)->expr;
              if (!expr->isE_stringLit()) {
                env.error("The argument to the alias attribute must be "
                          "a string literal.");
              }
              else {
                // Compute the string denoted by the literal.
                expr->tcheck(env, expr);

                // Store it in the string table.
                E_stringLit *strlit = expr->asE_stringLit();
                foundAlias = env.str.add(
                  (char const *)strlit->m_stringData.getDataC());
              }
            }
          }
        }
      }
    }
  }

  // Now, apply the recognized attribute semantics.

  // Apply '__mode__'.
  if (modeDesignator) {
    if (!tc.m_type) {
      env.error("'mode' attribute applied to something that is not a type.");
    }
    else if (tc.m_type->isSimpleType()) {
      // get details about current type
      SimpleTypeId existingId = tc.m_type->asSimpleTypeC()->type;
      CVFlags existingCV = tc.m_type->getCVFlags();
      bool uns = isExplicitlyUnsigned(existingId);

      // Interpret the mode designator code.
      SimpleTypeId id = ST_ERROR;    // means mode was not recognized
      if (streq_GAN(modeDesignator, "QI") ||
          streq_GAN(modeDesignator, "byte")) {
        id = uns? ST_UNSIGNED_CHAR : ST_SIGNED_CHAR;
      }
      else if (streq_GAN(modeDesignator, "HI")) {
        id = uns? ST_UNSIGNED_SHORT_INT : ST_SHORT_INT;
      }
      else if (streq_GAN(modeDesignator, "SI") ||
               streq_GAN(modeDesignator, "word")) {     // maybe?
        id = uns? ST_UNSIGNED_INT : ST_INT;
      }
      else if (streq_GAN(modeDesignator, "DI") ||
               streq_GAN(modeDesignator, "pointer")) {  // probably not right
        id = uns? ST_UNSIGNED_LONG_LONG : ST_LONG_LONG;
      }

      // change the type according to the mode
      if (id != ST_ERROR) {
        tc.m_type = env.getSimpleType(id, existingCV);
      }
      else {
        env.error(stringb(
          "Unrecognized __mode__ attribute argument: '" <<
          modeDesignator << "'"));
      }
    }
    else if (tc.m_type->isEnumType()) {
      // GCC allows it, and maybe even does something with it?
      env.warning("Ignoring mode attribute applied to an enum.");
    }
    else {
      env.error(tc.m_type, stringb(
        "__mode__ attribute must be applied to a fundamental scalar "
        "type, not '" << tc.m_type->toString() << "'"));
    }
  }

  // Apply '__transparent_union__'.
  if (transparentUnion) {
    if (!tc.m_type) {
      env.error("'transparent_union' attribute applied to something that is not a type.");
    }
    else if (tc.m_type->isUnionType()) {
      CompoundType *u = tc.m_type->asCompoundType();
      u->m_isTransparentUnion = true;
    }
    else {
      env.error(tc.m_type, stringb(
        "__transparent_union__ attribute must be applied to a union "
        "type, not '" << tc.m_type->toString() << "'"));
    }
  }

  // Apply '__alias__'.
  if (foundAlias) {
    if (!tc.m_type) {
      env.error("'alias' attribute applied to something that cannot be an alias.");
    }
    else {
      // Look up the name.
      Variable *target = env.lookupVariable(foundAlias);
      if (target) {
        tc.m_gnuAliasTarget = target;
      }
      else {
        env.error(tc.m_type, stringb(
          "__alias__ attribute target not found: '" << foundAlias << "'"));
      }
    }
  }
}


// ---------------------- AttributeSpecifier -------------------------
void AttributeSpecifier::print(PrintEnv &env) const
{
  env.begin();
  env << "__attribute__((";

  attr->print(env);

  AttributeSpecifier *p = next;
  while (p) {
    env << "," << env.sp;

    // We don't recursively invoke p->print() here because that would
    // print extra "__attribute__" keywords.
    p->attr->print(env);

    p = p->next;
  }

  env << "))";
  env.end();
}


// -------------------------- TypeSpecifier ----------------------------
void TypeSpecifier::prependASL(AttributeSpecifierList *list)
{
  m_attrSpecList = aslAppendASL(list, m_attrSpecList);
}


void TypeSpecifier::appendASL(AttributeSpecifierList * /*nullable*/ list)
{
  m_attrSpecList = aslAppendASL(m_attrSpecList, list);
}


void TypeSpecifier::prependElaboratedASL(AttributeSpecifierList *list)
{
  m_elaboratedAttrSpecList =
    aslAppendASL(list, m_elaboratedAttrSpecList);
}


void TypeSpecifier::ext_preprint_gnu(PrintEnv &env) const
{
  if (m_attrSpecList) {
    m_attrSpecList->print(env);
    env << env.sp;
  }
}


void TypeSpecifier::ext_printAfterClassKey_gnu(PrintEnv &env) const
{
  // These attributes go between the ClassKey and the type name.
  if (m_elaboratedAttrSpecList) {
    m_elaboratedAttrSpecList->print(env);
    env << env.sp;
  }
}


void TypeSpecifier::ext_tcheck_attrSpecList(Env &env, Tcheck &tc,
                                            Type *& /*INOUT*/ ret)
{
  // tcheck the attribute list.
  if (m_attrSpecList) {
    AttributeSpecifierList::Tcheck asltc(ret);
    m_attrSpecList->tcheck(env, asltc);

    if (asltc.m_gnuAliasTarget) {
      tc.m_gnuAliasTarget = asltc.m_gnuAliasTarget;
    }
  }
}


// ---------------------- Attribute -------------------------
void AT_empty::print(PrintEnv &env) const
{
  // Nothing to print.
}


void AT_word::print(PrintEnv &env) const
{
  env << w;
}


void AT_func::print(PrintEnv &env) const
{
  env.begin();

  env << f << "(";

  int ct = 0;
  FAKELIST_FOREACH(ArgExpression, args, argExpr) {
    if (ct++ > 0) {
      env << "," << env.sp;
    }
    argExpr->expr->print(env, OPREC_COMMA);
  }

  env << ")";

  env.end();
}


// ---------------------------- Statement ------------------------------
void Statement::ext_tcheck_gnu(Env &env)
{
  if (S_label *lbl = this->ifS_label()) {
    lbl->tcheckLabelAttributes(env);
  }
}


// ----------------------------- S_label -------------------------------
void S_label::appendASL(AttributeSpecifierList *list)
{
  m_attrSpecList = aslAppendASL(m_attrSpecList, list);
}


void S_label::tcheckLabelAttributes(Env &env)
{
  if (m_attrSpecList) {
    Type *dummy = NULL;
    AttributeSpecifierList::Tcheck asltc(dummy);
    m_attrSpecList->tcheck(env, asltc);
  }
}


void S_label::ext_print_gnu(PrintEnv &env, StatementContext) const
{
  if (m_attrSpecList) {
    env << env.sp;
    m_attrSpecList->print(env);
  }
}


// EOF
