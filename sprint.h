// sprint.h
// structure printer, for structural delta
// this module is experimental

#ifndef SPRINT_H
#define SPRINT_H

#include "cc-ast.h"     // C++ AST, visitor, etc.


// walk the AST, printing info about syntactic and
// static semantic structure
// (sm: This should not inherit from LoweredASTVisitor.)
// NOT Intended to be used with LoweredASTVisitor
class StructurePrinter : public ASTVisitor {
private:     // data
  // current syntactic nesting level; 0 means toplevel
  int depth;

  // true if we just began the current nesting level
  bool begin;

  // for each depth level, at what char offset did the current
  // entity begin?
  //ArrayStack<int> begins;

private:     // funcs
  ostream &ind();
  bool in(SourceLoc loc);
  void out();
  void digStmt(Statement *s);

public:      // funcs
  StructurePrinter()
    : depth(0),
      begin(false)
  {
    //begins.push(0);    // depth 0 starts at 0
  }

  bool visitTopForm(TopForm *tf)      override ;//{ return in(tf->loc); }
  void postvisitTopForm(TopForm*)     override { out(); }
  bool visitStatement(Statement *s)   override ;//{ return in(s->loc); }
  void postvisitStatement(Statement*) override { out(); }
  bool visitMember(Member *m)         override { return in(m->loc); }
  void postvisitMember(Member*)       override { out(); }
};


// convenient entry point
void structurePrint(TranslationUnit *unit);


#endif // SPRINT_H
