// const-eval.cc
// code for const-eval.h

#include "const-eval.h"     // this module
#include "cc-ast.h"         // C++ AST
#include "cc-env.h"         // Env
#include "stdconv.h"        // applyIntegralPromotions, etc.


enum {
  // The value of CHAR_BIT that Elsa provides as a C++ implementation.
  //
  // This is not necessarily the same as the value provided by the C++
  // compiler used to compile Elsa.
  //
  // There are many places that just use 8 directly, but I want to start
  // factoring that.
  ELSA_CHAR_BIT = 8,
};


// ----------------------- CValue ------------------------
bool CValue::operator== (CValue const &obj) const
{
  if (EMEMB(type)) {
    switch (classify(type)) {
      case K_SIGNED:         return EMEMB(si);
      case K_UNSIGNED:       return EMEMB(ui);
      case K_FLOAT:          return EMEMB(f);
      case K_ERROR:          return *(this->why) == *(obj.why);
      case K_DEPENDENT:      return true;
    }
  }
  return false;
}


STATICDEF CValue::Kind CValue::classify(SimpleTypeId t)
{
  switch (t) {
    case ST_CHAR:
    case ST_SIGNED_CHAR:
    case ST_SHORT_INT:
    case ST_WCHAR_T:
    case ST_INT:
    case ST_LONG_INT:
    case ST_LONG_LONG:
      return K_SIGNED;

    case ST_BOOL:
    case ST_UNSIGNED_CHAR:
    case ST_UNSIGNED_SHORT_INT:
    case ST_UNSIGNED_INT:
    case ST_UNSIGNED_LONG_INT:
    case ST_UNSIGNED_LONG_LONG:
      return K_UNSIGNED;

    case ST_FLOAT:
    case ST_DOUBLE:
    case ST_LONG_DOUBLE:
      return K_FLOAT;

    case ST_DEPENDENT:
      return K_DEPENDENT;

    default:
      return K_ERROR;
  }
}


void CValue::dup(CValue const &obj)
{
  type = obj.type;
  switch (classify(type)) {
    case K_SIGNED:    si = obj.si;     break;
    case K_UNSIGNED:  ui = obj.ui;     break;
    case K_FLOAT:     f = obj.f;       break;
    case K_ERROR:     why = obj.why;   break;
    case K_DEPENDENT:                  break;
  }
}


bool CValue::isZero() const
{
  switch (kind()) {
    default: // silence warning
    case K_SIGNED:    return si == 0;
    case K_UNSIGNED:  return ui == 0;
    case K_FLOAT:     return f == 0.0;
  }
}


bool CValue::isIntegral() const
{
  Kind k = kind();
  return k==K_SIGNED || k==K_UNSIGNED;
}

int CValue::getIntegralValue() const
{
  xassert(isIntegral());
  if (kind() == K_SIGNED) {
    return si;
  }
  else {
    return (int)ui;
  }
}


void CValue::setSigned(SimpleTypeId t, long v)
{
  type = t;
  xassert(isSigned());
  si = v;
}


void CValue::setUnsigned(SimpleTypeId t, unsigned long v)
{
  type = t;
  xassert(isUnsigned());
  ui = v;
}


void CValue::setFloat(SimpleTypeId t, float v)
{
  type = t;
  xassert(isFloat());
  f = v;
}


void CValue::setError(rostring w)
{
  type = ST_ERROR;

  // TOOD: This is leaked!
  why = new string(w);
}


void CValue::setDependent()
{
  type = ST_DEPENDENT;
  si = 0;
}


void CValue::performIntegralPromotions(ConstEval &env)
{
  convertToType(env, applyIntegralPromotions(type));
}

void CValue::convertToType(ConstEval &env, SimpleTypeId newType)
{
  Kind oldKind = classify(type);
  Kind newKind = classify(newType);
  if (newKind != oldKind) {
    // neither source nor dest should be sticky
    xassert(oldKind != K_ERROR && oldKind != K_DEPENDENT &&
            newKind != K_ERROR && newKind != K_DEPENDENT);

    xassert((unsigned)oldKind < 8u);    // for my representation trick

    // sort of like ML-style case analysis on a pair of values
    switch (newKind*8 + oldKind) {
      default:
        xfailure("unexpected type conversion scenario in const-eval");
        break;                   // silence warning

      case K_SIGNED*8 + K_UNSIGNED:
        si = (long)ui;           // convert unsigned to signed
        break;

      case K_SIGNED*8 + K_FLOAT:
        si = (long)f;
        break;

      case K_UNSIGNED*8 + K_SIGNED:
        ui = (unsigned long)si;
        break;

      case K_UNSIGNED*8 + K_FLOAT:
        ui = (unsigned long)f;
        break;

      case K_FLOAT*8 + K_SIGNED:
        f = (float)si;
        break;

      case K_FLOAT*8 + K_UNSIGNED:
        f = (float)ui;
        break;
    }
  }

  type = newType;

  // truncate the value based on the final type
  int reprSize = simpleTypeReprSize(env.m_typeSizes, type);
  if (reprSize >= 4) {
    // do no truncation
  }
  else {
    // 2021-06-03: I do not understand the code below.  Normally when
    // we truncate, we do modular reduction, but this code is taking
    // the maximum value.  I also don't understand how either case works
    // to do that; in the signed case we use the largest *unsigned*
    // value, and in the unsigned case, what is the point of the cast?

    int maxValue = (1 << 8*reprSize) - 1;
    switch (kind()) {
      case K_SIGNED:     if (si > maxValue) { si=maxValue; }  break;
      case K_UNSIGNED:   if (ui > (unsigned)maxValue) { ui=(unsigned)maxValue; }  break;
      default: break;   // ignore
    }
  }
}


void CValue::applyUnary(ConstEval &env, UnaryOp op)
{
  if (isSticky()) { return; }

  switch (op) {
    default: xfailure("bad code");

    case UNY_PLUS:
      // 5.3.1p6
      performIntegralPromotions(env);
      break;

    case UNY_MINUS:
      // 5.3.1p7
      performIntegralPromotions(env);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:    si = -si;   break;
        case K_UNSIGNED:  ui = -ui;   break;
        case K_FLOAT:     f = -f;     break;
      }
      break;

    case UNY_NOT:
      // 5.3.1p8
      ui = isZero()? 1u : 0u;
      type = ST_BOOL;
      break;

    case UNY_BITNOT:
      // 5.3.1p9
      performIntegralPromotions(env);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:    si = ~si;   break;
        case K_UNSIGNED:  ui = ~ui;   break;
        case K_FLOAT:     setError("cannot apply ~ to float types"); break;
      }
  }
}


void CValue::applyUsualArithmeticConversions(ConstEval &env, CValue &other)
{
  SimpleTypeId finalType = usualArithmeticConversions(type, other.type);
  this->convertToType(env, finalType);
  other.convertToType(env, finalType);
}


void CValue::addOffset(int offset)
{
  if (isSigned()) {
    si += offset;
  }
  else if (isUnsigned()) {
    ui += (unsigned long)offset;
  }
  else if (isFloat()) {
    f += (float)offset;     // questionable...
  }
  else {
    // other cases: leave as-is
  }
}


void CValue::applyBinary(ConstEval &env, BinaryOp op, CValue other)
{
  if (isSticky()) { return; }

  if (other.isSticky()) {
    *this = other;
    return;
  }

  switch (op) {
    default:
      setError(stringc << "cannot const-eval binary operator '"
                       << toString(op) << "'");
      return;

    // ---- 5.6 ----
    case BIN_MULT:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     si = si * other.si;    break;
        case K_UNSIGNED:   ui = ui * other.ui;    break;
        case K_FLOAT:      f = f * other.f;       break;
      }
      break;

    case BIN_DIV:
      applyUsualArithmeticConversions(env, other);
      if (other.isZero()) {
        setError("division by zero");
        return;
      }
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     si = si / other.si;    break;
        case K_UNSIGNED:   ui = ui / other.ui;    break;
        case K_FLOAT:      f = f / other.f;       break;
      }
      break;

    case BIN_MOD:
      applyUsualArithmeticConversions(env, other);
      if (other.isZero()) {
        setError("mod by zero");
        return;
      }
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     si = si % other.si;    break;
        case K_UNSIGNED:   ui = ui % other.ui;    break;
        case K_FLOAT:      setError("mod applied to floating-point args"); return;
      }
      break;

    // ---- 5.7 ----
    case BIN_PLUS:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     si = si + other.si;    break;
        case K_UNSIGNED:   ui = ui + other.ui;    break;
        case K_FLOAT:      f = f + other.f;       break;
      }
      break;

    case BIN_MINUS:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     si = si - other.si;    break;
        case K_UNSIGNED:   ui = ui - other.ui;    break;
        case K_FLOAT:      f = f - other.f;       break;
      }
      break;

    // ---- gcc <? and >? ----
    // guessing that <? and >? behave approximately like '+' in
    // terms of the types
    case BIN_MINIMUM:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case CValue::K_SIGNED:     si = ((si < other.si) ? si : other.si);   break;
        case CValue::K_UNSIGNED:   ui = ((ui < other.ui) ? ui : other.ui);   break;
        case CValue::K_FLOAT:      f = ((f < other.f) ? f : other.f);        break;
      }
      break;

    case BIN_MAXIMUM:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case CValue::K_SIGNED:     si = ((si > other.si) ? si : other.si);   break;
        case CValue::K_UNSIGNED:   ui = ((ui > other.ui) ? ui : other.ui);   break;
        case CValue::K_FLOAT:      f = ((f > other.f) ? f : other.f);        break;
      }
      break;

    // ---- 5.8 ----
    case BIN_LSHIFT:
    case BIN_RSHIFT: {
      this->performIntegralPromotions(env);
      other.performIntegralPromotions(env);

      if (kind() == K_FLOAT || other.kind() == K_FLOAT) {
        setError("cannot shift with float types");
        return;
      }

      // get shift amount
      int shamt = 0;
      switch (other.kind()) {
        default: // silence warning
        case K_SIGNED:     shamt = other.si;    break;
        case K_UNSIGNED:   shamt = other.ui;    break;
      }

      if (shamt < 0) {
        // C++14 5.8/1
        setError(stringb("Shifting by a negative value (" << shamt <<
                         ") is undefined"));
        return;
      }

      int leftOperandBits =
        simpleTypeReprSize(env.m_typeSizes, type) * ELSA_CHAR_BIT;
      if (shamt >= leftOperandBits) {
        // C++14 5.8/1
        setError(stringb(
          "Shifting by a value (" << shamt << ") greater than or equal "
          "to the length in bits of the promoted left operand (" <<
          leftOperandBits << ") is undefined."));
        return;
      }

      // apply it
      if (op == BIN_LSHIFT) {
        switch (kind()) {
          default: // silence warning
          case K_SIGNED: {
            if (si < 0) {
              // C++14 5.8/2
              setError(stringb(
                "Left-shifting a negative left operand value (" <<
                si << ") is undefined."));
              return;
            }

            long orig_si = si;
            si <<= shamt;

            // The result of the shift might be outside the range of
            // what 'type' can represent.
            //
            // TODO: This is all quite sketchy because I am relying on
            // the host compiler's 'long' to be big enough to perform
            // the calculations, which it often will not be.
            long maxSignedValue = (long)((1 << (leftOperandBits - 1UL)) - 1);
            if (si > maxSignedValue) {
              unsigned long maxUnsignedValue = (maxSignedValue << 1UL) + 1;
              if ((unsigned long)si <= maxUnsignedValue) {
                // The result is implementation-defined per C++14 4.7/3.
                // GCC does modular reduction, so I will too.
                si = si - maxUnsignedValue - 1;
              }
              else {
                // C++14 5.8/2
                setError(stringb(
                  "Left-shifting " << orig_si << " by " << shamt <<
                  "bits yields " << si << ", which is too large to "
                  "represent as the unsigned type corresponding to '" <<
                  simpleTypeName(type) << "', hence undefined."));
                return;
              }
            }

            break;
          }

          case K_UNSIGNED:
            // TODO: There are a number of cases to consider here, just
            // like the K_SIGNED case.
            ui <<= shamt;
            break;
        }
      }
      else {
        switch (kind()) {
          default: // silence warning
          case K_SIGNED:     si >>= shamt;    break;
          case K_UNSIGNED:   ui >>= shamt;    break;
        }
      }

      break;
    }

    // ---- 5.9 ----
    case BIN_LESS:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     ui = si < other.si;    break;
        case K_UNSIGNED:   ui = ui < other.ui;    break;
        case K_FLOAT:      ui = f < other.f;       break;
      }
      type = ST_BOOL;
      break;

    case BIN_GREATER:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     ui = si > other.si;    break;
        case K_UNSIGNED:   ui = ui > other.ui;    break;
        case K_FLOAT:      ui = f > other.f;       break;
      }
      type = ST_BOOL;
      break;

    case BIN_LESSEQ:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     ui = si <= other.si;    break;
        case K_UNSIGNED:   ui = ui <= other.ui;    break;
        case K_FLOAT:      ui = f <= other.f;       break;
      }
      type = ST_BOOL;
      break;

    case BIN_GREATEREQ:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     ui = si >= other.si;    break;
        case K_UNSIGNED:   ui = ui >= other.ui;    break;
        case K_FLOAT:      ui = f >= other.f;       break;
      }
      type = ST_BOOL;
      break;

    // ---- 5.10 ----
    case BIN_EQUAL:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     ui = si == other.si;    break;
        case K_UNSIGNED:   ui = ui == other.ui;    break;
        case K_FLOAT:      ui = f == other.f;       break;
      }
      type = ST_BOOL;
      break;

    case BIN_NOTEQUAL:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     ui = si != other.si;    break;
        case K_UNSIGNED:   ui = ui != other.ui;    break;
        case K_FLOAT:      ui = f != other.f;       break;
      }
      type = ST_BOOL;
      break;

    // ---- 5.11 ----
    case BIN_BITAND:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     si = si & other.si;    break;
        case K_UNSIGNED:   ui = ui & other.ui;    break;
        case K_FLOAT:      setError("cannot apply bitand to float types"); break;
      }
      break;

    // ---- 5.12 ----
    case BIN_BITXOR:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     si = si ^ other.si;    break;
        case K_UNSIGNED:   ui = ui ^ other.ui;    break;
        case K_FLOAT:      setError("cannot apply bitxor to float types"); break;
      }
      break;

    // ---- 5.13 ----
    case BIN_BITOR:
      applyUsualArithmeticConversions(env, other);
      switch (kind()) {
        default: // silence warning
        case K_SIGNED:     si = si | other.si;    break;
        case K_UNSIGNED:   ui = ui | other.ui;    break;
        case K_FLOAT:      setError("cannot apply bitor to float types"); break;
      }
      break;

    // ---- 5.14 ----
    case BIN_AND:
      // Note: short-circuit behavior is handled by the caller of
      // this function; by the time we get here, both args have
      // already been evaluated
      this->convertToType(env, ST_BOOL);
      other.convertToType(env, ST_BOOL);
      ui = ui && other.ui;
      break;

    // ---- 5.15 ----
    case BIN_OR:
      this->convertToType(env, ST_BOOL);
      other.convertToType(env, ST_BOOL);
      ui = ui || other.ui;
      break;
  }
}


string CValue::asString() const
{
  switch (kind()) {
    case K_SIGNED:      return stringc << toString(type) << ": " << si;
    case K_UNSIGNED:    return stringc << toString(type) << ": " << ui;
    case K_FLOAT:       return stringc << toString(type) << ": " << f;
    case K_ERROR:       return stringc << "error: " << *why;
    default:    // silence warning
    case K_DEPENDENT:   return "dependent";
  }
}


// ----------------------- ConstEval ------------------------
ConstEval::ConstEval(TypeSizes const &typeSizes, Variable *d)
  : m_typeSizes(typeSizes),
    dependentVar(d)
{}

ConstEval::~ConstEval()
{}


// external interface
CValue Expression::constEval(ConstEval &env) const
{
  #if 1       // production
    return iconstEval(env);
  #else       // debugging
    #warning debugging code is enabled
    CValue ret = iconstEval(env);
    cout << "const-eval of '" << exprToString() << "' is " << ret.asString() << endl;
    return ret;
  #endif
}


CValue Expression::iconstEval(ConstEval &env) const
{
  if (ambiguity) {
    // 2005-05-26: This used to be cause for an assertion failure, but
    // what can happen (in/c/dC0018.c) is that a user-error here or
    // higher up causes the tcheck to prune, thereby leaving an
    // ambiguity link.  What I will do is make this a user-error
    // condition also, under the premise that (due to disambiguation
    // selecting a different interpretation) the user will never see
    // it; but by adding the error, I ensure this interpretation will
    // not part of a successful parse.
    return CValue("ambiguous expr being const-eval'd (user should not see this)");
  }

  if (type->isError()) {
    // don't try to const-eval an expression that failed
    // to typecheck
    return CValue("failed to tcheck");
  }

  ASTSWITCHC(Expression, this) {
    // Handle this idiom for finding a member offset:
    // &((struct scsi_cmnd *)0)->b.q
    ASTCASEC(E_addrOf, eaddr)
      return eaddr->expr->constEvalAddr(env);

    ASTNEXTC(E_boolLit, b)
      CValue ret;
      ret.setBool(b->b);
      return ret;

    ASTNEXTC(E_intLit, i)
      CValue ret;
      SimpleTypeId id = type->asSimpleTypeC()->type;
      switch (CValue::classify(id)) {
        default: xfailure("unexpected intlit type");
        case CValue::K_SIGNED:     ret.setSigned(id, (long)i->i);              break;
        case CValue::K_UNSIGNED:   ret.setUnsigned(id, (unsigned long)i->i);   break;
      }
      return ret;

    ASTNEXTC(E_floatLit, f)
      CValue ret;
      SimpleTypeId id = type->asSimpleTypeC()->type;
      ret.setFloat(id, (float)f->d);
      return ret;

    ASTNEXTC(E_charLit, c)
      CValue ret;
      SimpleTypeId id = type->asSimpleTypeC()->type;
      switch (CValue::classify(id)) {
        default: xfailure("unexpected charlit type");
        case CValue::K_SIGNED:     ret.setSigned(id, (long)c->c);              break;
        case CValue::K_UNSIGNED:   ret.setUnsigned(id, (unsigned long)c->c);   break;
      }
      return ret;

    ASTNEXTC(E_variable, v)
      return env.evaluateVariable(v->var);

    ASTNEXTC(E_constructor, c)
      if (type->isIntegerType()) {
        // allow it; should only be 1 arg, and that will be value
        return fl_first(c->args)->constEval(env);
      }
      else {
        return CValue("can only const-eval E_constructors for integer types");
      }

    ASTNEXTC(E_sizeof, s)
      // 5.3.3p6: result is of type 'size_t'; most systems (including my
      // elsa/include/stddef.h header) make that the same as 'unsigned';
      // in any case, it must be an unsigned integer type (c99, 7.17p2)
      CValue ret;
      ret.setUnsigned(ST_UNSIGNED_INT, s->size);
      return ret;

    ASTNEXTC(E_unary, u)
      CValue ret = u->expr->constEval(env);
      ret.applyUnary(env, u->op);
      return ret;

    ASTNEXTC(E_binary, b)
      if (b->op == BIN_COMMA) {
        // avoid trying to eval the LHS
        return b->e2->constEval(env);
      }

      CValue v1 = b->e1->constEval(env);
      if (b->op == BIN_AND && v1.isZero()) {
        CValue ret;
        ret.setBool(false);   // short-circuit: propagate false
        return ret;
      }
      if (b->op == BIN_OR && !v1.isZero()) {
        CValue ret;
        ret.setBool(true);    // short-circuit: propagate false
        return ret;
      }

      CValue v2 = b->e2->constEval(env);

      v1.applyBinary(env, b->op, v2);
      return v1;

    ASTNEXTC(E_cast, c)
      return constEvalCast(env, c->ctype, c->expr);

    ASTNEXTC(E_keywordCast, c)
      if (c->key == CK_DYNAMIC) {
        return CValue("cannot const-eval a keyword_cast");
      }
      else {
        // assume the other three work like C casts
        return constEvalCast(env, c->ctype, c->expr);
      }

    ASTNEXTC(E_cond, c)
      CValue v = c->cond->constEval(env);
      if (v.isSticky()) {
        return v;
      }

      if (!v.isZero()) {
        return c->th->constEval(env);
      }
      else {
        return c->el->constEval(env);
      }

    ASTNEXTC(E_sizeofType, s)
      if (s->atype->getType()->isGeneralizedDependent()) {
        return CValue(ST_DEPENDENT);
      }
      CValue ret;
      ret.setUnsigned(ST_UNSIGNED_INT, s->size);
      return ret;

    ASTNEXTC(E_grouping, e)
      return e->expr->constEval(env);

    ASTNEXTC(E_implicitStandardConversion, e)
      return e->expr->constEval(env);

    ASTNEXTC(E_offsetof, e)
      if (CompoundType *ct = e->atype->getType()->asRval()->ifCompoundType()) {
        CValue val;
        val.setUnsigned(env.m_typeSizes.m_type_of_size_t,
          ct->getDataMemberOffset(env.m_typeSizes, e->field));
        return val;
      }
      else {
        return CValue("invalid operand to offsetof, must be a compound type");
      }

    ASTDEFAULTC
      return extConstEval(env);

    ASTENDCASEC
  }
}

// The intent of this function is to provide a hook where extensions
// can handle the 'constEval' message for their own AST syntactic
// forms, by overriding this function.  The non-extension nodes use
// the switch statement above, which is more compact.
CValue Expression::extConstEval(ConstEval &env) const
{
  return CValue(stringc << kindName() << " is not constEval'able");
}


CValue ConstEval::evaluateVariable(Variable *var)
{
  if (var->isEnumerator()) {
    CValue ret;
    ret.setSigned(ST_INT, var->getEnumeratorValue());
    return ret;
  }

  if (var->type->isCVAtomicType() &&
      (var->type->asCVAtomicTypeC()->cv & CV_CONST) &&
      var->value) {
    // const variable
    return var->value->constEval(*this);
  }

  if (var->type->isGeneralizedDependent() &&
      var->value) {
    return CValue(ST_DEPENDENT);
  }

  if (var->isTemplateParam()) {
    return CValue(ST_DEPENDENT);
  }

  if (var == dependentVar) {
    // value-dependent expression
    return CValue(ST_DEPENDENT);
  }

  return CValue(stringc
    << "can't const-eval non-const variable '" << var->name << "'");
}


// TODO: This function is basically a stub; it does not compute
// the right result in almost any circumstance.
CValue Expression::constEvalAddr(ConstEval &env) const
{
  // FIX: I'm sure there are cases missing from this
  ASTSWITCHC(Expression, this) {
    // These two are dereferences, so they recurse back to constEval().
    ASTCASEC(E_deref, e)
      return e->ptr->constEval(env);

    ASTNEXTC(E_arrow, e)
      return e->obj->constEval(env);

    ASTNEXTC(E_fieldAcc, e) {
      // 2006-06-03: Trying to make this work for in/c/k0014.c.
      CValue val = e->obj->constEvalAddr(env);
      CompoundType *ct = e->obj->type->asRval()->ifCompoundType();
      if (!ct) {
        // probably something weird like a template
      }
      else {
        val.addOffset(ct->getDataMemberOffset(env.m_typeSizes, e->field));
      }
      return val;
    }

    // These just recurse on constEvalAddr().
    ASTNEXTC(E_cast, e)
      return e->expr->constEvalAddr(env);

    ASTNEXTC(E_keywordCast, e)
      // FIX: haven't really thought about these carefully
      switch (e->key) {
        default:
          xfailure("bad CastKeyword");

        case CK_DYNAMIC:
          return CValue("can't const-eval dynamic_cast");

        case CK_STATIC:
        case CK_REINTERPRET:
        case CK_CONST:
          return e->expr->constEvalAddr(env);
      }
      break;

    ASTNEXTC(E_grouping, e)
      return e->expr->constEvalAddr(env);

    ASTDEFAULTC
      return CValue("unhandled case in constEvalAddr");

    ASTENDCASEC
  }
}


CValue Expression::constEvalCast(ConstEval &env, ASTTypeId const *ctype,
                                 Expression const *expr) const
{
  CValue ret = expr->constEval(env);
  if (ret.isSticky()) {
    return ret;
  }

  Type *t = ctype->getType();
  if (t->isSimpleType()) {
    ret.convertToType(env, t->asSimpleTypeC()->type);
  }
  else if (t->isEnumType() ||
           t->isPointer()) {        // for Linux kernel
    ret.convertToType(env, ST_INT);
  }
  else {
    // TODO: this is probably not the right rule..
    return CValue(stringc
      << "in constant expression, can only cast to arithmetic or pointer types, not '"
      << t->toString() << "'");
  }

  return ret;
}


// EOF
