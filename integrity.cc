// integrity.cc
// code for integrity.h

#include "integrity.h"                 // this module

#include "vector-util.h"               // vecBackOr, vecContains, vecPopCheck


IntegrityVisitor::IntegrityVisitor(CCLang const &lang, bool inTemplate)
  : ASTVisitorEx(),
    m_enclosingSyntaxStack(),
    m_lang(lang),
    m_checkVariableScopes(true),
    m_requireExpressionTypes(false)
{
  m_inTemplate = inTemplate;
}


void IntegrityVisitor::checkTU(TranslationUnit *tu)
{
  tu->traverse(*this);
  xassert(getEnclosingSyntax() == IntegrityVisitor::ES_NONE);
}


void IntegrityVisitor::foundAmbiguous(void *obj, void **ambig, char const *kind)
{
  // 2005-06-29: I have so far been unable to provoke this error by
  // doing simple error seeding, because it appears to be masked by a
  // check in the elaboration visitor regarding visiting certain lists
  // more than once.  Among other things, that means if there is a bug
  // along these lines, a user will discover it by seeing the
  // unfriendly list-visit assertion failure instead of the message
  // here.  But, that stuff is wrapped up in the Daniel's lowered
  // visitor mechanism, which I don't want to mess with now.  Anyway,
  // I'm reasonably confident that this check will work properly.
  xfatal(toString(loc) << ": internal error: found ambiguous " << kind);
}


IntegrityVisitor::EnclosingSyntax IntegrityVisitor::getEnclosingSyntax() const
{
  return vecBackOr(m_enclosingSyntaxStack, ES_NONE);
}

bool IntegrityVisitor::withinSyntax(EnclosingSyntax es) const
{
  return vecContains(m_enclosingSyntaxStack, es);
}

void IntegrityVisitor::pushSyntax(EnclosingSyntax es)
{
  m_enclosingSyntaxStack.push_back(es);
}

void IntegrityVisitor::popSyntax(EnclosingSyntax es)
{
  vecPopCheck(m_enclosingSyntaxStack, es);
}


bool IntegrityVisitor::visitTopForm(TopForm *topForm)
{
  if (!ASTVisitorEx::visitTopForm(topForm)) {
    return false;
  }

  if (topForm->isTF_namespaceDefn()) {
    pushSyntax(ES_NAMESPACE);
  }

  return true;
}


void IntegrityVisitor::postvisitTopForm(TopForm *topForm)
{
  ASTVisitorEx::postvisitTopForm(topForm);

  if (topForm->isTF_namespaceDefn()) {
    popSyntax(ES_NAMESPACE);
  }
}


bool IntegrityVisitor::visitFunction(Function *func)
{
  if (!ASTVisitorEx::visitFunction(func)) {
    return false;
  }

  pushSyntax(ES_FUNCTION);

  return true;
}


void IntegrityVisitor::postvisitFunction(Function *func)
{
  ASTVisitorEx::postvisitFunction(func);

  popSyntax(ES_FUNCTION);
}


// The various annotations added by the type checker should all be
// present.
bool IntegrityVisitor::visitTypeSpecifier(TypeSpecifier *typeSpecifier)
{
  if (!ASTVisitorEx::visitTypeSpecifier(typeSpecifier)) {
    return false;
  }

  ASTSWITCHC(TypeSpecifier, typeSpecifier) {
    ASTCASEC(TS_name, tsn) {
      xassert(tsn->var != NULL);
    }

    ASTNEXTC(TS_elaborated, tse) {
      xassert(tse->atype != NULL);

      // In all cases, the 'typedefVar' of a NamedAtomicType should not
      // be NULL.
      xassert(tse->atype->typedefVar != NULL);
    }

    ASTNEXTC(TS_classSpec, tsc) {
      xassert(tsc->ctype != NULL);
      checkDefinitionTypedefVar(tsc->ctype->typedefVar);

      pushSyntax(ES_CLASS);
    }

    ASTNEXTC(TS_enumSpec, tse) {
      xassert(tse->etype != NULL);
      checkDefinitionTypedefVar(tse->etype->typedefVar);
    }

    ASTENDCASECD
  }

  return true;
}


// TODO: Currently this is only done for typedef Variables, but I
// would like to extend it to cover all Variables.
void IntegrityVisitor::checkDefinitionTypedefVar(Variable const *var)
{
  xassert(var != NULL);

  if (!m_checkVariableScopes) {
    return;
  }

  // The permanent scope the variable says it is in, if any.  The scope
  // might not actually have a record of the variable, for example if
  // the variable is anonymous, or we are in C and this is a C++
  // implicit typedef, but 'm_containingScope' says where it would have
  // gone.
  Scope *scope = var->m_containingScope;
  if (scope) {
    xassert(scope->isPermanentScope());
  }

  if (getEnclosingSyntax() == ES_NONE) {
    // We're not inside anything.

    if (scope) {
      if (scope->isGlobalScope()) {
        shouldBeGlobal(var);
      }
      else if (scope->isNamespace() || scope->isClassScope()) {
        // The variable is something that was declared inside a class
        // or namespace but then defined outside it.
        shouldNotBeGlobal(var);
      }
      else {
        xfailure("Unknown permanent scope kind.");
      }
    }
    else {
      xfailure("Variable at file scope missing m_containingScope.");
    }
  }

  else if (m_lang.noInnerClasses &&    // i.e., we are in C
           var->isType() &&            // currently always true
           var->name != NULL &&        // anons don't go to global
           withinSyntax(ES_CLASS) &&
           !withinSyntax(ES_FUNCTION)) {
    // In C, if a type is defined inside a struct, it is as if it was
    // declared at global scope.  But if all of that is inside a
    // function, then the type is local to the function, not global.
    shouldBeGlobal(var);
  }

  else {
    // We're inside something that gives rise to a scope, so it should
    // *not* be global.
    //
    // This would be wrong for friends, but you can't define a type in a
    // friend declaration, so at the moment this won't trip for that.
    shouldNotBeGlobal(var);
  }
}


void IntegrityVisitor::shouldBeGlobal(Variable const *var)
{
  xassert(var->inGlobalScope());

  Scope *scope = var->m_containingScope;
  xassert(scope);
  xassert(scope->isGlobalScope());
}

void IntegrityVisitor::shouldNotBeGlobal(Variable const *var)
{
  xassert(!var->inGlobalScope());

  Scope *scope = var->m_containingScope;
  xassert(!( scope && scope->isGlobalScope() ));
}


void IntegrityVisitor::postvisitTypeSpecifier(TypeSpecifier *typeSpecifier)
{
  ASTVisitorEx::postvisitTypeSpecifier(typeSpecifier);

  if (typeSpecifier->isTS_classSpec()) {
    popSyntax(ES_CLASS);
  }
}


bool IntegrityVisitor::visitIDeclarator(IDeclarator *idecl)
{
  if (!ASTVisitorEx::visitIDeclarator(idecl)) {
    return false;
  }

  if (idecl->isD_func()) {
    pushSyntax(ES_PARAMETER_LIST);
  }

  return true;
}


void IntegrityVisitor::postvisitIDeclarator(IDeclarator *idecl)
{
  ASTVisitorEx::postvisitIDeclarator(idecl);

  if (idecl->isD_func()) {
    popSyntax(ES_PARAMETER_LIST);
  }
}


bool IntegrityVisitor::visitDeclarator(Declarator *obj)
{
  if (!ASTVisitorEx::visitDeclarator(obj)) {
    return false;
  }

  // make sure the type is not a DQT if we are not in a template
  if (!m_inTemplate) {
    checkNontemplateType(obj->var->type);
    checkNontemplateType(obj->type);
  }

  obj->var->checkInvariants();

  return true;
}

void IntegrityVisitor::checkNontemplateType(Type *t)
{
  xassert(t);
  if (t->containsGeneralizedDependent()) {
    xfatal(toString(loc) << ": internal error: found dependent type '"
                         << t->toString() << "' in non-template (0a257264-c6ec-4983-95d0-fcd6aa48a6ce)");
  }
}


bool IntegrityVisitor::visitExpression(Expression *obj)
{
  if (!ASTVisitorEx::visitExpression(obj)) {
    return false;
  }

  // 2005-08-18: I started to do this, then realized that these might
  // survive in template bodies.
  //
  // TODO: Make a way for ASTVisitorEx to communicate to visitors
  // whether they are in template bodies or not.
  //
  // 2006-05-30: Um, what was I thinking?  Why is 'm_inTemplate' not
  // sufficient?
  #if 0
  if (obj->isE_grouping()) {
    xfatal(toString(loc) << ": internal error: found E_grouping after tcheck");
  }
  if (obj->isE_arrow()) {
    xfatal(toString(loc) << ": internal error: found E_arrow after tcheck");
  }
  #endif // 0

  // why was I not doing this before?  detects problem in/t0584.cc
  if (!m_inTemplate) {
    if (m_requireExpressionTypes || obj->type) {
      checkNontemplateType(obj->type);
    }
  }

  return true;
}


void integrityCheckTU(CCLang const &lang, TranslationUnit *tu)
{
  IntegrityVisitor ivis(lang, false /*inTemplate*/);
  ivis.checkTU(tu);
}


// EOF
