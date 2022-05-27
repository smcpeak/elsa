// integrity.h
// IntegrityVisitor class.

#ifndef INTEGRITY_H
#define INTEGRITY_H

#include "astvisit.h"                  // ASTVisitorEx
#include "cc-lang.h"                   // CCLang

#include <vector>                      // std::vector


// Integrity checks:
//
//   - The AST must be unambiguous.
//   - No dependent types appear in concrete code.
//   - Typedef Variables are marked DF_GLOBAL or not as appropriate.
//   - Other ad-hoc constraints.
//
// These checks only make sense after the type checker has done its job
// and not found any errors.
//
// When a check fails, it throws an assertion failure exception.
//
// There are flags to control whether certain checks are performed.  The
// default set of checks is appropriate for use after Elsa has parsed
// some source code.  But if, say, AST is being constructed as part of a
// source-to-source transformation, some of the checks might need to be
// turned off.
//
class IntegrityVisitor : public ASTVisitorEx {
public:      // types
  // Kinds of possible enclosing syntax.
  enum EnclosingSyntax {
    ES_NONE,
    ES_FUNCTION,
    ES_CLASS,
    ES_NAMESPACE,
    ES_PARAMETER_LIST,

    NUM_ENCLOSING_SYNTAXES
  };

private:     // data
  // Stack of kinds of enclosing syntax, from outermost to innermost.
  std::vector<EnclosingSyntax> m_enclosingSyntaxStack;

public:      // data
  // Language rules in effect when translation unit was parsed.
  CCLang const &m_lang;

  // When true (which is the default), check Variable scopes and
  // DF_GLOBAL flag.
  bool m_checkVariableScopes;

private:     // funcs
  // Check the typedef Variable of a class or enum type whose definition
  // we have encountered.
  void checkDefinitionTypedefVar(Variable const *var);

  // Assert that 'var' is marked as a global.
  void shouldBeGlobal(Variable const *var);

  // Assert that 'var' is marked as a non-global.
  void shouldNotBeGlobal(Variable const *var);

  void checkNontemplateType(Type *t);

public:      // funcs
  // The integrity checks depend on whether we are in an uninstantiated
  // template (see comments on 'm_inTemplate' in cc-tcheck.ast for
  // exactly what that means), which must be passed as the 'inTemplate'
  // parameter.
  //
  // If this visitor is being launched from another visitor, pass the
  // outer visitor's 'inTemplate'.  Otherwise, one would usually pass
  // 'false'.
  IntegrityVisitor(CCLang const &lang, bool inTemplate);

  // Run the checks on 'tu'.  This is the main entry point after
  // constructing the visitor.
  void checkTU(TranslationUnit *tu);

  // Innermost enclosing syntax, or ES_NONE.
  EnclosingSyntax getEnclosingSyntax() const;

  // Return true if 'es' is anywhere on the syntax stack.
  bool withinSyntax(EnclosingSyntax es) const;

  // Push 'es' onto the stack.
  void pushSyntax(EnclosingSyntax es);

  // Pop 'es' off of the stack.
  void popSyntax(EnclosingSyntax es);

  // ASTVisitorEx functions
  void foundAmbiguous(void *obj, void **ambig, char const *kind) override;

  // ASTVisitor functions
  bool visitTopForm(TopForm *topForm) override;
  void postvisitTopForm(TopForm *topForm) override;
  bool visitFunction(Function *func) override;
  void postvisitFunction(Function *func) override;
  bool visitTypeSpecifier(TypeSpecifier *typeSpecifier) override;
  void postvisitTypeSpecifier(TypeSpecifier *typeSpecifier) override;
  bool visitDeclarator(Declarator *obj) override;
  bool visitIDeclarator(IDeclarator *idecl) override;
  void postvisitIDeclarator(IDeclarator *idecl) override;
  bool visitExpression(Expression *obj) override;
};

// Run the checks on an entire TU.
void integrityCheckTU(CCLang const &lang, TranslationUnit *tu);


#endif // INTEGRITY_H
