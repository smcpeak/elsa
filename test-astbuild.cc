// test-astbuild.cc
// Tests for ast_build.h.

#include "ast_build.h"                 // module under test

#include "cc_ast.h"                    // ModificationASTVisitor
#include "cc_print.h"                  // PrintEnv, etc.
#include "elsaparse.h"                 // ElsaParse
#include "integrity.h"                 // IntegrityVisitor

#include "sm-test.h"                   // EXPECT_EQ


// Visitor that tests ElsaASTBuild at certain nodes.
class TestASTBuildVisitor : public ASTVisitor {
public:      // data
  // Parsed AST, etc.
  ElsaParse &m_elsaParse;

  // The builder to test.
  ElsaASTBuild &m_astBuild;

public:
  explicit TestASTBuildVisitor(ElsaParse &elsaParse, ElsaASTBuild &astBuild)
    : m_elsaParse(elsaParse),
      m_astBuild(astBuild)
  {}

  string astTypeIdToString(ASTTypeId *astTypeId);

  // ASTVisitor methods.
  bool visitDeclaration(Declaration *declaration) override;
  bool visitASTTypeId(ASTTypeId *astTypeId) override;
};


string TestASTBuildVisitor::astTypeIdToString(ASTTypeId *astTypeId)
{
  CTypePrinter typePrinter(m_elsaParse.m_lang);
  stringBuilder sb;
  StringBuilderOutStream sbos(sb);
  CodeOutStream cos(sbos);
  PrintEnv env(typePrinter, &cos);

  astTypeId->print(env);

  return sb.str();
}


bool TestASTBuildVisitor::visitDeclaration(Declaration *declaration)
{
  // The main test logic is in 'visitASTTypeId'.  So, for each
  // declarator, build an ASTTypeId with the type specifier as this
  // declaration and use that for testing.
  FAKELIST_FOREACH_NC(Declarator, declaration->decllist, iter) {
    ASTTypeId tmpId(declaration->spec, iter);
    visitASTTypeId(&tmpId);

    // Nullify the members of 'tmpId' so they do not get destroyed.
    tmpId.spec = NULL;
    tmpId.decl = NULL;
  }
  return true;
}


bool TestASTBuildVisitor::visitASTTypeId(ASTTypeId *originalId)
{
  static bool verbose = tracingSys("test_astbuild_verbose");

  // First print the original syntax.
  string originalString = astTypeIdToString(originalId);
  if (verbose) {
    cout << "originalString: " << originalString << endl;
  }

  try {
    // Now build a new one.
    PQName *newName = m_astBuild.makePQName(originalId->decl->var);
    ASTTypeId *newId =
      m_astBuild.makeASTTypeId(originalId->decl->type, newName,
                               originalId->decl->context);

    // Print that to a string as well.
    string newString = astTypeIdToString(newId);
    if (verbose) {
      cout << "newString: " << newString << endl;
    }

    // This does not work.  The 'originalString' string may refer to
    // typedefs, while the 'newString' never will.  So, I guess this test
    // will just be to exercise the machinery.
    //EXPECT_EQ(newString, originalString);

    // Run the integrity checker on the new fragment.
    IntegrityVisitor ivis(m_inTemplate);
    newId->traverse(ivis);

    delete newId;
  }
  catch (XUnimp &x) {
    if (verbose) {
      cout << x.why() << endl;
    }
    // The code for template types is not written.  Ignore.
  }

  // Visit children.
  return true;
}


void test_astbuild(ElsaParse &elsaParse)
{
  SourceLocProvider locProvider;
  ElsaASTBuild astBuild(elsaParse.m_stringTable, elsaParse.m_typeFactory,
                        locProvider);
  TestASTBuildVisitor visitor(elsaParse, astBuild);

  elsaParse.m_translationUnit->traverse(visitor);
}


// EOF
