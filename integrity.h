// integrity.h
// IntegrityVisitor class.

#ifndef INTEGRITY_H
#define INTEGRITY_H

#include "astvisit.h"                  // ASTVisitorEx

// Integrity checks:
//
//   - The AST must be unambiguous.
//   - No dependent types appear in concrete code.
//   - Other ad-hoc constraints.
//
// These checks only make sense after the type checker has done its job
// and not found any errors.
//
// When a check fails, it throws an assertion failure exception.
//
class IntegrityVisitor : public ASTVisitorEx {
private:     // funcs
  void checkNontemplateType(Type *t);

public:      // funcs
  // The integrity checks depend on whether we are in an uninstantiated
  // template, which must be passed as the 'inTemplate' parameter.  When
  // true, it means the topmost visited AST node is a descendant of
  // TD_func, TD_decl, or TD_tmember.
  //
  // If this visitor is being launched from another visitor, pass the
  // outer visitor's 'inTemplate'.  Otherwise, one would usually pass
  // 'false'.
  explicit IntegrityVisitor(bool inTemplate);

  // ASTVisitorEx functions
  virtual void foundAmbiguous(void *obj, void **ambig, char const *kind);

  // ASTVisitor functions
  bool visitTypeSpecifier(TypeSpecifier *typeSpecifier) override;
  bool visitDeclarator(Declarator *obj) override;
  bool visitExpression(Expression *obj) override;
};

// Run the checks on an entire TU.
void integrityCheckTU(TranslationUnit *tu);


#endif // INTEGRITY_H
