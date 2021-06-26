# Makefile
# Makefile for Elsa, the Elkhound-based C++ Parser.

# Default target.
all: cc.ast.gen.h tlexer.exe packedword_test.exe semgrep.exe smin.exe ccparse.exe


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
OPTIMIZATION_FLAGS = -O2

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
PERL    = perl
PYTHON3 = python3
AR      = ar
RANLIB  = ranlib
DEP     = $(PERL) depend.pl
SMFLEX  = $(SMFLEXDIR)/smflex -b


# ---- Options within this Makefile ----
# Modules to compile with coverage info, for example 'cc-tcheck'.
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

# Remove 'config.mk' with 'distclean'.
TODISTCLEAN += config.mk

# Clean files related to coverage analysis.
TOCLEAN += *.bb *.bbg *.da


# Compile .cc to .o, also generating .d dependency files.
TOCLEAN += *.o *.d *.exe
%.o: %.cc
	$(CXX) -c -o $@ $(if $(findstring $*,$(GCOV_MODS)),$(GCOV_OPTS) )$(GENDEPS_FLAGS) $(CXXFLAGS) $<

# Pull in any dependency files we have.
-include *.d


# ----------------- extra dependencies -----------------
# These dependencies ensure that automatically-generated code is
# created in time to be used by other build processes which need it.

# Arguments to find-extra-deps.py.
EXTRADEPS_ARGS := *.d

.PHONY: remake-extradep
remake-extradep:
	$(PYTHON3) $(SMBASE)/find-extra-deps.py $(EXTRADEPS_ARGS) >extradep.mk

include extradep.mk

# Ordinarily I want to validate extradep, but if I have just added a new
# header then it spuriously considers it as "extra", so I need a way to
# ignore that.
ifneq ($(IGNORE_EXTRADEP),1)
check: validate-extradep
endif

.PHONY: validate-extradep
validate-extradep: all
	$(PYTHON3) $(SMBASE)/find-extra-deps.py $(EXTRADEPS_ARGS) >extradep.tmp
	@echo diff extradep.mk extradep.tmp
	@if diff extradep.mk extradep.tmp; then true; else \
	  echo "extradep.mk needs updating; run 'make remake-extradep'"; \
	  exit 2; \
	fi
	rm extradep.tmp


# --------------------- extension modules ----------------------
# base modules
LEXER_MODS  := cc.lex
TOK_MODS    := cc-tokens.tok
CC_AST_MODS := cc.ast
CC_GR_MODS  := cc.gr
EXT_OBJS    :=

# type checker
CC_AST_MODS += cc-tcheck.ast

# pretty printer
CC_AST_MODS += cc-print.ast

# control flow graph
CC_AST_MODS += cfg.ast

# elaboration pass
CC_AST_MODS += cc-elaborate.ast


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
TOCLEAN += *.yy.cc *.yy.h lex.backup
%.yy.h %.yy.cc: %.lex
	$(SMFLEX) -o$*.yy.cc $*.lex


# ------------------------ tlexer -------------------
LEXER_OBJS :=
LEXER_OBJS += cc-lang.o
LEXER_OBJS += type-sizes.o
LEXER_OBJS += baselexer.o
LEXER_OBJS += lexer.o
LEXER_OBJS += lexer.yy.o
LEXER_OBJS += cc-tokens.o

# program to test the lexer alone
tlexer.exe: tlexer.o $(LEXER_OBJS) $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# ------------------------ packedword_test -------------------
# program to test packedword
packedword_test.exe: packedword_test.o $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# ------------------------- ccparse ---------------------
# combine base lexer description and extensions
TOCLEAN += lexer.lex
lexer.lex: $(LEXER_MODS) merge-lexer-exts.pl
	rm -f $@
	$(PERL) merge-lexer-exts.pl $(LEXER_MODS) >$@
	chmod a-w $@


# Generate token lists.
#
# This is written as a pattern rule because multi-target non-pattern
# rules are broken.  It is only to be used when "%" is "cc".
TOCLEAN += cc-tokens.h cc-tokens.cc cc-tokens.ids
%-tokens.h %-tokens.cc %-tokens.ids: %-dummy.txt $(TOK_MODS) make-token-files
	test "x$*" = "xcc"
	rm -f cc-tokens.h cc-tokens.cc cc-tokens.ids
	$(PERL) make-token-files $(TOK_MODS)
	chmod a-w cc-tokens.h cc-tokens.cc cc-tokens.ids


# Run astgen to generate the AST implementation.
#
# This is written as a pattern rule because multi-target non-pattern
# rules are broken.  It is only to be used when "%" is "cc".
TOCLEAN += *.ast.gen.h *.ast.gen.cc
%.ast.gen.h %.ast.gen.cc: %-dummy.txt $(CC_AST_MODS) $(AST)/astgen.exe
	test "x$*" = "xcc"
	rm -f cc.ast.gen.h cc.ast.gen.cc
	$(AST)/astgen.exe -occ.ast.gen $(CC_AST_MODS)
	chmod a-w cc.ast.gen.h cc.ast.gen.cc


# run elkhound to generate the parser
#
# This is written as a pattern rule because multi-target non-pattern
# rules are broken.  It is only to be used when "%" is "cc".
TOCLEAN += cc.gr.gen.h cc.gr.gen.cc cc.gr.gen.out
%.gr.gen.h %.gr.gen.cc %.gr.gen.out: $(CC_GR_MODS) %-tokens.ids $(ELKHOUND)/elkhound.exe
	test "x$*" = "xcc"
	rm -f cc.gr.gen.h cc.gr.gen.cc cc.gr.gen.out
	$(ELKHOUND)/elkhound.exe -v -tr lrtable -o cc.gr.gen $(CC_GR_MODS)
	chmod a-w cc.gr.gen.h cc.gr.gen.cc cc.gr.gen.out

# list of modules needed for the parser; ideally they're in an order
# that finds serious compilation problems earliest (it's ok to
# rearrange as different parts of the code are in flux)
ELSA_OBJS :=
ELSA_OBJS += elsaparse.o
ELSA_OBJS += mtype.o
ELSA_OBJS += integrity.o
ELSA_OBJS += astvisit.o
ELSA_OBJS += template.o
ELSA_OBJS += cc-env.o
ELSA_OBJS += cc-tcheck.o
ELSA_OBJS += const_eval.o
ELSA_OBJS += implint.o
ELSA_OBJS += serialno.o
ELSA_OBJS += cc-scope.o
ELSA_OBJS += cc-elaborate.o
ELSA_OBJS += ast_build.o
ELSA_OBJS += $(LEXER_OBJS)
ELSA_OBJS += $(EXT_OBJS)
ELSA_OBJS += builtinops.o
ELSA_OBJS += cfg.o
ELSA_OBJS += sprint.o
ELSA_OBJS += mangle.o
ELSA_OBJS += cc-err.o
ELSA_OBJS += cc-type.o
ELSA_OBJS += cc-type-visitor.o
ELSA_OBJS += stdconv.o
ELSA_OBJS += implconv.o
ELSA_OBJS += overload.o
ELSA_OBJS += typelistiter.o
ELSA_OBJS += cc.ast.gen.o
ELSA_OBJS += cc.gr.gen.o
ELSA_OBJS += parssppt.o
ELSA_OBJS += cc-flags.o
ELSA_OBJS += type-sizes.o
ELSA_OBJS += cc-print.o
ELSA_OBJS += type-printer.o
ELSA_OBJS += cc-ast-aux.o
ELSA_OBJS += variable.o
ELSA_OBJS += lookupset.o
ELSA_OBJS += ccparse.o

# Parser as a library.
TOCLEAN += libelsa.a
libelsa.a: $(ELSA_OBJS)
	rm -f $@
	$(AR) -r $@ $(ELSA_OBJS)
	-$(RANLIB) $@


# Modules that are part of 'ccparse' but not 'libelsa'.  These have
# the 'main()' function and some unit test modules.
CCPARSE_OBJS :=
CCPARSE_OBJS += main.o
CCPARSE_OBJS += test-astbuild.o

# parser binary
ccparse.exe: $(CCPARSE_OBJS) libelsa.a $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^
	./ccparse.exe in/t0001.cc


# -------------------- semgrep --------------------
semgrep.exe: semgrep.o libelsa.a $(LIBS)
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
# This is an experiment I never finished.

SMIN_OBJS :=
SMIN_OBJS += iptparse.o
SMIN_OBJS += iptparse.yy.o
SMIN_OBJS += iptree.o
SMIN_OBJS += smin.o

smin.exe: $(SMIN_OBJS) $(LIBS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^


# --------------------- cipart --------------------
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
	  -Xcc-env.h=1 -Xcc-type.h=1 -Xcc-flags.h=1 -Xcc-ast.h=1 -Xvariable.h=1 \
          -Xcc-print.h -Xsprint.h \
	  -Xgeneric_aux.h -Xcc-ast-aux.h -Xcc-lang.h=1 \
	  main.cc cc-tcheck.cc >$@

gendoc/3.4.5.dot: ccparse.exe in/std/3.4.5.cc
	./ccparse.exe -tr printHierarchies in/std/3.4.5.cc | \
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
# and it appears to not count cc-tokens.tok (which it should).
# I don't care about fixing right now it though.

GENREGEX := '\.gen\.\|lexer\.yy\|cc-tokens'
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
#	'outdir' is used by 'idemcheck'
	cd outdir && ls | xargs rm -f
	$(MAKE) -C test clean

distclean: clean
	rm -f $(TODISTCLEAN)
	rm -rf gendoc

toolclean: clean
	rm -f $(TOTOOLCLEAN)

# Certain failing multi-tests leave behind error files.
TOCLEAN += in/*.error*

check: test-check

# Run 'make check' in test/.
test-check: all
	$(MAKE) -C test check

check: all
	./packedword_test.exe
	MAKE=$(MAKE) ./regrtest
	@echo ""
	@echo "Regression tests passed."


# EOF
