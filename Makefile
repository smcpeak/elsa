# Makefile
# Makefile for Elsa, the Elkhound-based C++ Parser.

#temporary: iptree iptparse cipart smin

# main target: a C++ parser
all: cc.ast.gen.h tlexer ccparse quicktest packedword_test

# work in progress..
#iptree smin cipart


# ------------------------- Configuration --------------------------
# ---- Running other programs ----
# Directories of other software.
SMBASE    = ../smbase
SMFLEXDIR = ../smflex
AST       = ../ast
ELKHOUND  = ../elkhound

# C++ preprocessor, compiler, and linker.
CXX = g++

# Flags to control generation of debug info.
DEBUG_FLAGS = -g

# Flags to enable dependency generation of .d files.
GENDEPS_FLAGS = -MMD

# Flags to control optimization.
#
# TODO: Things are broken when optimization is enabled.
OPTIMIZATION_FLAGS =

# Flags to control compiler warnings.
WARNING_FLAGS = -Woverloaded-virtual

# Flags for C or C++ standard to use.
C_STD_FLAGS   = -std=c99
CXX_STD_FLAGS = -std=c++11

# -D flags to pass to preprocessor.
DEFINES =

# -I flags to pass to preprocessor.
INCLUDES = -I$(SMBASE) -I$(AST) -I$(ELKHOUND)

# Preprocessing flags.
CPPFLAGS = $(INCLUDES) $(DEFINES)

# Flags for the C and C++ compilers (and preprocessor).
#
# Note: $(GENDEPS_FLAGS) are not included because these flags are used
# for linking too, and if that used $(GENDEPS_FLAGS) then the .d files
# for .o files would be overwritten with info for .exe files.
CXXFLAGS = $(DEBUG_FLAGS) $(OPTIMIZATION_FLAGS) $(WARNING_FLAGS) $(CXX_STD_FLAGS) $(CPPFLAGS)

# How to enable coverage.
GCOV_OPTS = -fprofile-arcs -ftest-coverage

# Libraries to link with when creating executables.
LIBSMBASE   = $(SMBASE)/libsmbase.a
LIBAST      = $(AST)/libast.a
LIBELKHOUND = $(ELKHOUND)/libelkhound.a
LIBS = $(LIBELKHOUND) $(LIBAST) $(LIBSMBASE)

# Flags to add to a link command *in addition* to either $(CFLAGS) or
# $(CXXFLAGS), depending on whether C++ modules are included.
LDFLAGS =

# Some other tools.
PERL   = perl
AR     = ar
RANLIB = ranlib
DEP    = $(PERL) depend.pl
SMFLEX = $(SMFLEXDIR)/smflex -b


# ---- Options within this Makefile ----
# Modules to compile with coverage info, for example 'cc_tcheck'.
#
# I do not build them all with coverage info because it takes about 25%
# longer to compile for each module with coverage info.
GCOV_MODS =

# Active language extensions.
USE_GNU   = 1
USE_KANDR = 1


# ---- Automatic Configuration ----
# Pull in settings from ./configure.  They override the defaults above,
# and are in turn overridden by personal.mk, below.
ifeq ($(wildcard config.mk),)
  $(error The file 'config.mk' does not exist.  Run './configure' before 'make'.)
endif
include config.mk


# ---- Customization ----
# Allow customization of the above variables in a separate file.  Just
# create personal.mk with desired settings.
#
# Common things to set during development:
#
#   WERROR = -Werror
#   WARNING_FLAGS = -Wall $(WERROR)
#   OPTIMIZATION_FLAGS =
#
-include personal.mk


# ----------------------------- Rules ------------------------------
# Eliminate all implicit rules.
.SUFFIXES:

# Delete a target when its recipe fails.
.DELETE_ON_ERROR:

# Do not remove "intermediate" targets.
.SECONDARY:


# List of files to remove in 'clean' (etc.) targets.  They are added to
# below, next to the rules that generate the files.
TOCLEAN =
TOTOOLCLEAN =
TODISTCLEAN =

# Compile .cc to .o, also generating .d dependency files.
TOCLEAN += *.o *.d
%.o: %.cc
	$(CXX) -c -o $@ $(if $(findstring $*,$(GCOV_MODS)),$(GCOV_OPTS) )$(GENDEPS_FLAGS) $(CXXFLAGS) $<

# compile a special module; -O0 will override any earlier setting
#
# TODO: Why did I need this?
notopt.o: notopt.cc
	$(CXX) -c -o $@ $(GENDEPS_FLAGS) $(CXXFLAGS) -O0 $<


# Remove 'config.mk' with 'distclean'.
TODISTCLEAN += config.mk

# dependencies upon automatically-generated files
#
# TODO: This is broken right now.
extradep.mk:
	$(PERL) $(ELKHOUND)/find-extra-deps *.d >$@

-include extradep.mk

# Clean files related to coverage analysis.
TOCLEAN += *.bb *.bbg *.da


# --------------------- extension modules ----------------------
# base modules
LEXER_MODS  := cc.lex
TOK_MODS    := cc_tokens.tok
CC_AST_MODS := cc.ast
CC_GR_MODS  := cc.gr
EXT_OBJS    :=

# type checker
CC_AST_MODS += cc_tcheck.ast

# pretty printer
CC_AST_MODS += cc_print.ast

# control flow graph
CC_AST_MODS += cfg.ast

# elaboration pass
CC_AST_MODS += cc_elaborate.ast


# optional: GNU language extension
ifeq ($(USE_GNU),1)
  LEXER_MODS  += gnu.lex
  TOK_MODS    += gnu_ext.tok
  CC_AST_MODS += gnu.ast
  CC_GR_MODS  += gnu.gr
  EXT_OBJS    += gnu.o
endif


# optional: K&R language extension
ifeq ($(USE_KANDR),1)
  CC_AST_MODS += kandr.ast
  CC_GR_MODS  += kandr.gr
  EXT_OBJS    += kandr.o
endif


# ------------------- Running smflex ------------------
# TODO: I would like to call these ".lex.gen.*".
TOCLEAN += *.yy.cc *.yy.h lex.backup
%.yy.h %.yy.cc: %.lex
	$(SMFLEX) -o$*.yy.cc $*.lex


# ------------------------ tlexer -------------------
LEXER_OBJS := \
  cc_lang.o \
  baselexer.o \
  lexer.o \
  lexer.yy.o \
  cc_tokens.o
-include $(LEXER_OBJS:.o=.d)
-include tlexer.d

# program to test the lexer alone
TOCLEAN += tlexer
tlexer: tlexer.o $(LEXER_OBJS) $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# ------------------------ packedword_test -------------------
# program to test packedword
TOCLEAN += packedword_test
packedword_test: packedword_test.o $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# ------------------------- xml serialization / de-serialization ---------------------

#### single-source of lexing token definitions:

# basic, file, and typesystem xml lexer files generated by token.pl;
# the ast tokens are generated by astgen elsewhere

TOKENS_FILES :=
TOKENS_FILES += xml_basic.tokens
TOKENS_FILES += xml_file.tokens
TOKENS_FILES += xml_type.tokens
TOKENS_FILES += xml_ast.gen.tokens

XML_TYPE_LEXER :=
XML_TYPE_LEXER += xml_enum_1.gen.h
XML_TYPE_LEXER += xml_lex_1.gen.lex
XML_TYPE_LEXER += xml_name_1.gen.cc

# TODO: This multi-target non-pattern rule is unsafe.
TOCLEAN += $(XML_TYPE_LEXER)
$(XML_TYPE_LEXER): $(TOKENS_FILES)
	rm -f $@
	$(PERL) token.pl $^


#### LEX: flex-generated lexer

# ast xml lexer/parser files generated by astgen
XML_ASTGEN :=
XML_ASTGEN += xml_ast.gen.tokens
XML_ASTGEN += xml_ast_reader_0decl.gen.h
XML_ASTGEN += xml_ast_reader_1defn.gen.cc
XML_ASTGEN += xml_ast_reader_2ctrc.gen.cc
XML_ASTGEN += xml_ast_reader_3regc.gen.cc

TOCLEAN += $(XML_ASTGEN)
$(XML_ASTGEN): $(CC_AST_MODS) $(AST)/astgen.exe
	rm -f $(XML_ASTGEN)
	$(AST)/astgen.exe -tr no_ast.gen,xmlParser $(CC_AST_MODS)
	chmod a-w $(XML_ASTGEN)

# generate .lex file
TOCLEAN += xml_lex.gen.lex
xml_lex.gen.lex: xml_lex_0top.lex xml_lex_1.gen.lex xml_lex_2bot.lex
	rm -f $@
	cat $^ > $@
	chmod a-w $@

# when building the ast xml lexer, delete the methods that would
# otherwise conflict with methods in lexer.yy.cc; they have identical
# implementations
#
# TODO: Do I still need NO_YYFLEXLEXER_METHODS?
xml_lexer.yy.o: xml_lex.gen.yy.cc
	$(CXX) -c -o $@ -DNO_YYFLEXLEXER_METHODS $(GENDEPS_FLAGS) $(CXXFLAGS) $<
# We do not need this now but we will if we end up re-arranging the
# build process like I had to in oink so I put it here as a reminder.
# -include xml_lexer.yy.d
#
# TODO: Why is it not needed?


#### CC client code

# all the xml-related .o files
XML_OBJS :=
# lex
XML_OBJS += xml_lexer.yy.o
XML_OBJS += xml_lexer.o
# generic parse
XML_OBJS += xml_reader.o
XML_OBJS += xml_writer.o
# specific parse
XML_OBJS += xml_file_reader.o
XML_OBJS += xml_file_writer.o
XML_OBJS += xml_type_reader.o
XML_OBJS += xml_type_writer.o
XML_OBJS += xml_ast_reader.o
XML_OBJS += id_obj_dict.o
# final client
XML_OBJS += xml_do_read.o

# I don't think I need this line:
# -include $(CCPARSE_OBJS:.o=.d) main.d
# because of this line:
# -include $(XML_OBJS:.o=.d)


# ------------------------- ccparse ---------------------
# combine base lexer description and extensions
TOCLEAN += lexer.lex
lexer.lex: $(LEXER_MODS) merge-lexer-exts.pl
	rm -f $@
	$(PERL) merge-lexer-exts.pl $(LEXER_MODS) >$@
	chmod a-w $@


# Dependencies on generated code.
#
# TODO: Rehability find-extra-deps, which will obviate this.
tlexer.o: lexer.yy.h
main.o: xml_enum_1.gen.h
main.o: xml_lex.gen.yy.h

# generate token lists
#
# TODO: This multi-target non-pattern rule is unsafe.
TOK_FILES := cc_tokens.h cc_tokens.cc cc_tokens.ids
TOCLEAN += $(TOK_FILES)
$(TOK_FILES): $(TOK_MODS) make-token-files
	rm -f $(TOK_FILES)
	$(PERL) make-token-files $(TOK_MODS)
	chmod a-w $(TOK_FILES)


# run astgen to generate the AST implementation
#
# TODO: This multi-target non-pattern rule is unsafe.
CC_AST_GEN_FILES := cc.ast.gen.h cc.ast.gen.cc
TOCLEAN += $(CC_AST_GEN_FILES)
$(CC_AST_GEN_FILES): $(CC_AST_MODS) $(AST)/astgen.exe
	rm -f $(CC_AST_GEN_FILES)
	$(AST)/astgen.exe -occ.ast.gen $(CC_AST_MODS)
	chmod a-w $(CC_AST_GEN_FILES)


# run elkhound to generate the parser
#
# TODO: This multi-target non-pattern rule is unsafe.
CC_GR_GEN_FILES := cc.gr.gen.h cc.gr.gen.cc cc.gr.gen.out
TOCLEAN += $(CC_GR_GEN_FILES)
$(CC_GR_GEN_FILES): $(CC_GR_MODS) cc_tokens.ids $(ELKHOUND)/elkhound.exe
	rm -f $(CC_GR_GEN_FILES)
	$(ELKHOUND)/elkhound.exe -v -tr lrtable -o cc.gr.gen $(CC_GR_MODS)
	chmod a-w $(CC_GR_GEN_FILES)

# list of modules needed for the parser; ideally they're in an order
# that finds serious compilation problems earliest (it's ok to
# rearrange as different parts of the code are in flux)
#
# TODO: Reformat as a series of += assignments.
CCPARSE_OBJS := \
  mtype.o \
  integrity.o \
  astvisit.o \
  template.o \
  notopt.o \
  cc_env.o \
  cc_tcheck.o \
  const_eval.o \
  implint.o \
  serialno.o \
  cc_scope.o \
  cc_elaborate.o \
  ast_build.o \
  $(LEXER_OBJS) \
  $(XML_OBJS) \
  $(EXT_OBJS) \
  builtinops.o \
  cfg.o \
  sprint.o \
  mangle.o \
  cc_err.o \
  cc_type.o \
  stdconv.o \
  implconv.o \
  overload.o \
  typelistiter.o \
  cc.ast.gen.o \
  cc.gr.gen.o \
  parssppt.o \
  cc_flags.o \
  cc_print.o \
  cc_ast_aux.o \
  variable.o \
  lookupset.o \
  ccparse.o
-include $(CCPARSE_OBJS:.o=.d) main.d

# Parser as a library.
TOCLEAN += libelsa.a
libelsa.a: $(CCPARSE_OBJS)
	rm -f $@
	$(AR) -r $@ $(CCPARSE_OBJS)
	-$(RANLIB) $@


# parser binary
TOCLEAN += ccparse
ccparse: main.o libelsa.a $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# run the binary; the 'quicktest' file is so we don't run it if
# 'ccparse' hasn't recently changed
TOCLEAN += quicktest
quicktest: ccparse
	./ccparse in/t0001.cc
	@touch quicktest
	@echo BUILD FINISHED


# -------------------- semgrep --------------------
TOCLEAN += semgrep
semgrep: $(CCPARSE_OBJS) semgrep.o $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# --------------------- iptree --------------------
TOCLEAN += iptree
iptree: iptree.cc $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) -DTEST_IPTREE $(LDFLAGS) $^


# -------------------- iptparse -------------------
TOCLEAN += iptparse
iptparse: iptparse.cc iptree.o iptparse.yy.o $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) -DTEST_IPTPARSE $(LDFLAGS) $^


# ---------------------- smin ---------------------
# TODO: Replace this with find-extra-deps.
iptparse.yy.o: iptparse.h

# TODO: Use += assignments.
SMIN_OBJS := \
  iptparse.o \
  iptparse.yy.o \
  iptree.o \
  smin.o
-include $(SMIN_OBJS:.o=.d)

TOCLEAN += smin
smin: $(SMIN_OBJS) $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# --------------------- cipart --------------------
-include cipart.yy.d

TOCLEAN += cipart
cipart: cipart.yy.o $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# ---------------------- misc ---------------------
# rule to decompress one of the big examples
TODISTCLEAN += in/big/*.i
in/big/%.i: in/big/gz/%.i.gz
	gunzip -c <$^ >$@
# comment out line directives
	$(PERL) -i -lpe 's|^\W*(#.*)$$|//$$1|' $@
# this line is illegal C++ and for now we just comment it out
	$(PERL) -i -lpe 's|^(\W*)(char ip_opts\[40\];)\W*$$|$$1//$$2|' $@
# change "((void *)0)" to "0" to compensate for a bad NULL macro somewhere...
	$(PERL) -i -lpe 's|\(\(void \*\)0\)|0|' $@
# define bad_alloc, don't just declare it
	$(PERL) -i -lpe 's|class bad_alloc;|class bad_alloc {};|' $@

# decompress all of them which haven't already been decompressed
.PHONY: in/big
in/big: $(patsubst in/big/gz/%.i.gz,in/big/%.i,$(wildcard in/big/gz/*.gz))
	@echo made $@


# ------------------ documentation ------------------
gendoc:
	mkdir gendoc

gendoc/configure.txt: configure
	./configure --help >$@

# The list of excluded header files is somewhat arbitrary.  Basically,
# I look at the generated .ps file, and if it is too cluttered, I pick
# some header that is uninteresting or ubiquitous and exclude it.  The
# intent is to see the forest by excluding some of the trees, but
# there's some judgment involved in picking the trees to see.
#
# Also, I say the target is .PHONY because I don't want to actually
# list all the dependencies (essentially, every source file is a
# dependency), but if no dependencies are listed then 'make' won't
# build it if it exists.
.PHONY: gendoc/dependencies.dot
gendoc/dependencies.dot:
	$(PERL) $(SMBASE)/scan-depends.pl -r \
	  -Xcc_env.h=1 -Xcc_type.h=1 -Xcc_flags.h=1 -Xcc_ast.h=1 -Xvariable.h=1 \
          -Xcc_print.h -Xsprint.h -Xxml_type_reader.h -Xxml_type_writer.h -Xxml_ast_reader.h \
          -Xxml_do_read.h \
	  -Xgeneric_aux.h -Xcc_ast_aux.h -Xcc_lang.h=1 \
	  main.cc cc_tcheck.cc >$@

gendoc/3.4.5.dot: ccparse in/std/3.4.5.cc
	./ccparse -tr printHierarchies in/std/3.4.5.cc | \
	$(PERL) chop_out "--- E ---" "--- F ---" >$@

# because of the above dependency on ccparse, if someone does 'make doc'
# without first building Elsa, they get an error about libsmbase.a; so
# this is an attempt to deal with that more gracefully
$(SMBASE)/libsmbase.a:
	@echo "You have to build smbase, ast and elkhound first."
	@exit 2

# check to see if they have dot
.PHONY: dot
dot:
	@if ! which dot >/dev/null; then \
	  echo "You don't have the 'dot' tool.  It is part of graphviz, available at:"; \
	  echo "http://www.research.att.com/sw/tools/graphviz/"; \
	  exit 2; \
	fi

# use 'dot' to lay out the graph
%.ps: %.dot dot
	dot -Tps <$*.dot >$@

# use 'convert' to make a PNG image with resolution not to exceed
# 1200 in X or 1000 in Y ('convert' will preserve aspect ratio); this
# also antialiases, so it looks very nice (it's hard to reproduce
# this using 'gs' alone)
%.png: %.ps
	convert -geometry 1200x1000 $^ $@

# 3.4.5 is smaller
gendoc/3.4.5.png: gendoc/3.4.5.ps
	convert -geometry 300x400 $^ $@

.PHONY: doc
doc: gendoc gendoc/configure.txt gendoc/dependencies.png gendoc/3.4.5.png
	@echo "built documentation"

#TOCLEAN += TAGS
#TAGS:
#	$(ETAGS) *.cc *.h


# -------------------- count source lines -------------------
# dsw: This should give the right answer even after a "make all",
# since we filter the generated files.
#
# sm: I haven't carefully inspected the set of files counted,
# and it appears to not count cc_tokens.tok (which it should).
# I don't care about fixing right now it though.

GENREGEX := '\.gen\.\|lexer\.yy\|cc_tokens'
.PHONY: count-loc
count-loc:
	@echo
	@echo "Count of lines of source code in this directory by file type."
	@echo "C++, C, and headers:"
# 	@ls *.cc *.c *.h | grep -v $(GENREGEX) | xargs wc -l | grep total
	@ls *.cc *.h | grep -v $(GENREGEX) | xargs wc -l | grep total
	@echo "tok, lex, gr, and ast:"
# 	@ls *_ext.tok *_ext.lex *.gr *.ast | grep -v $(GENREGEX) | xargs wc -l | grep total
	@ls *_ext.tok *.gr *.ast | grep -v $(GENREGEX) | xargs wc -l | grep total
	@echo "sum of those:"
# 	@ls *.cc *.c *.h *_ext.tok *_ext.lex *.gr *.ast
	@ls *.cc *.h *_ext.tok *.gr *.ast \
          | grep -v $(GENREGEX) | xargs wc -l | grep total
	@echo
	@echo "Makefiles:"
	@ls Makefile.in *.mk | xargs wc -l | grep total


# -------------------- clean, etc. -------------------
clean:
	rm -f $(TOCLEAN) gmon.out
	cd outdir && ls | grep -v CVS | xargs rm -f

distclean: clean
	rm -f $(TODISTCLEAN)
	rm -rf gendoc

toolclean: clean
	rm -f $(TOTOOLCLEAN)

# Certain failing multi-tests leave behind error files.
TOCLEAN += in/*.error*

check: semgrep all
	./packedword_test
	MAKE=$(MAKE) ./regrtest
	@echo ""
	@echo "Regression tests passed."

# run the xml commutative diagram tests
.PHONY: checkxml
checkxml:
	$(MAKE) -f xml_test.mk
	@echo ""
	@echo "XML tests passed."

# run all the tests
.PHONY: check-full
check-full: check checkxml
