// ast_build.h
// Class ElsaASTBuild.

#ifndef AST_BUILD_H
#define AST_BUILD_H

#include "cc_ast.h"      // C++ AST


// Interface for ElsaASTBuild to obtain a location.
class SourceLocProvider {
public:
  // The default implementation returns SL_UNKNOWN.
  virtual SourceLoc provideLoc() const;
};


// Methods for constructing fragments of the C++ AST.
//
// This is meant for use during semantic elaboration and for source
// to source instrumentation.
//
// Currently, some methods do not create AST that satisfies all of the
// invariants that the type checker does, since I am using it primarily
// for source-to-source where that isn't needed.
//
class ElsaASTBuild {
public:      // data
  // Table for making StringRefs for the AST.
  StringTable &m_stringTable;

  // For making new Type objects.
  TypeFactory &m_typeFactory;

  // Used when an AST node needs a location.
  SourceLocProvider &m_locProvider;

public:      // methods
  ElsaASTBuild(StringTable &stringTable, TypeFactory &tfac,
               SourceLocProvider &locProvider);

  SourceLoc loc() const
    { return m_locProvider.provideLoc(); }

  // wrap an expression in a list
  FakeList<ArgExpression> *makeExprList1(Expression *e);

  // wrap a pair of expressions in a list
  FakeList<ArgExpression> *makeExprList2(Expression *e1, Expression *e2);

  // Return an ASTTypeId denoting 'type'.
  ASTTypeId *makeASTTypeId(Type *type);

  // Make an E_intLit.
  E_intLit *makeE_intLit(int n);

  // Make a PQ_name after turning 'name' into a StringRef.
  PQ_name *makePQ_name(char const *name);
};


#endif // AST_BUILD_H
