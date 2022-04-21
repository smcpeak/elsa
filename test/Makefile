# elsa/test/Makefile
# Makefile-based tests.

# I'm transitioning away from 'regrtest' in favor of 'make'-based tests
# since the latter are easier to run individually and parallelize.

# Default rule.
all: check


# -------------------------- configuration -----------------------------
# Other repos, expressed relative to the *parent* directory.
SMBASE  = ../smbase

# Locations relative to *this* directory.
SMBASE_FROM_HERE = ../$(SMBASE)

# External tools.
PYTHON3 = python3
PERL    = perl

# How to invoke run-compare-expect.py.
RUN_COMPARE_EXPECT = $(PYTHON3) $(SMBASE_FROM_HERE)/run-compare-expect.py

# Name of parser binary.
CCPARSE = ../ccparse.exe

# Pull in automatic and manual configuration adjustments.
include ../config.mk
-include ../personal.mk


# ------------------------------ rules ---------------------------------
# Eliminate all implicit rules.
.SUFFIXES:

# Delete a target when its recipe fails.
.DELETE_ON_ERROR:

# Do not remove "intermediate" targets.
.SECONDARY:

# List of tests.  This gets added to below.
TESTS :=

# If we are missing an expect file, just make an empty one.
%.expect:
	touch $@


# ------------------------------ pprint --------------------------------
# Pretty-print tests run with --pretty-print and compare the output to
# a "known good" expected output.

TESTS += pprint/class.cc
TESTS += pprint/class2.cc
TESTS += pprint/conversion-op.cc
TESTS += pprint/forward.cc
TESTS += pprint/longlines.cc
TESTS += pprint/pp-decl.cc
TESTS += pprint/pp-enum.cc
TESTS += pprint/pp-syntax.cc
TESTS += pprint/precedence.cc
TESTS += pprint/template1.cc
TESTS += pprint/throwspec.cc
TESTS += pprint/vararg-promote.cc

# Run a C++ pretty-print test.
out/pprint/%.cc.ok: pprint/%.cc pprint/%.cc.expect $(CCPARSE)
	@mkdir -p $(dir $@)idem
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.cc.actual \
	  --expect pprint/$*.cc.expect \
	  $(CCPARSE) --pretty-print $<
#	The 'cpp' here is to get rid of the template instantiations,
#	which are bracketed by #if 0/#endif.
	../chop_out < out/pprint/$*.cc.actual | cpp > out/pprint/$*.cc.trimmed
	$(CCPARSE) out/pprint/$*.cc.trimmed
	g++ -c -oout/pprint/$*.cc.trimmed.o -xc++ out/pprint/$*.cc.trimmed
	../idemcheck --elsadir .. -d $(dir $@)idem -q $<
	touch $@


TESTS += pprint/array-plus-int.c
TESTS += pprint/attr-may-alias.c
TESTS += pprint/big-for-loop.c
TESTS += pprint/bitfield.c
TESTS += pprint/for-loop-no-init.c
TESTS += pprint/func.c
TESTS += pprint/funcptr.c
TESTS += pprint/if-without-else.c
TESTS += pprint/init-struct-with-string.c
TESTS += pprint/init-struct-with-string2.c
TESTS += pprint/long-decl.c
TESTS += pprint/longlines2.c
TESTS += pprint/longlines3.c
TESTS += pprint/nested-block.c
TESTS += pprint/range-case.c
TESTS += pprint/stdconv.c
TESTS += pprint/stringlit.c
TESTS += pprint/typedef.c
TESTS += pprint/vararg-promote.c

# Run a C pretty-print test.
#
# For these I also disable the elaborator because it does not do much
# in C and the things it does do (mainly about structure assignment)
# are things I do not want, at least as part of a pprint test.
out/pprint/%.c.ok: pprint/%.c pprint/%.c.expect pprint/%.c.isc.expect $(CCPARSE)
	@mkdir -p $(dir $@)idem
#	Main pretty-print.
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.c.actual \
	  --expect pprint/$*.c.expect \
	  $(CCPARSE) -xc --pretty-print --no-elaborate $<
#	Test that output can be parsed by Elsa and GCC.
	../chop_out < out/pprint/$*.c.actual > out/pprint/$*.c.trimmed
	$(CCPARSE) -xc out/pprint/$*.c.trimmed
	gcc -c -oout/pprint/$*.c.trimmed.o -xc out/pprint/$*.c.trimmed
	../idemcheck --elsadir .. -d $(dir $@)idem -xc --no-elaborate -q $<
#	Pretty-print with ISC enabled.
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.c.isc.actual \
	  --expect pprint/$*.c.isc.expect \
	  $(CCPARSE) -xc --pretty-print --no-elaborate --print-isc $<
	touch $@


# Test without passing the result into GCC.
out/pprint/%.c.nogcc.ok: pprint/%.c pprint/%.c.expect $(CCPARSE)
	@mkdir -p $(dir $@)idem
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.c.actual \
	  --expect pprint/$*.c.expect \
	  $(CCPARSE) -xc --pretty-print $<
	touch $@

check: out/pprint/chained-labels.c.nogcc.ok


# ------------------------------ strlit --------------------------------
# String literal tests run with --print-string-literals and compare the
# output to expected output.

TESTS += strlit/strlit1.cc

# Run a C++ string literal test.
out/strlit/%.cc.ok: strlit/%.cc strlit/%.cc.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/strlit/$*.cc.actual \
	  --expect strlit/$*.cc.expect \
	  $(CCPARSE) --print-string-literals $<
	touch $@


TESTS += strlit/multiline.c

# Run a C string literal test.
out/strlit/%.c.ok: strlit/%.c strlit/%.c.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/strlit/$*.c.actual \
	  --expect strlit/$*.c.expect \
	  $(CCPARSE) -xc --print-string-literals $<
	touch $@


# ------------------------------ pqname --------------------------------
# pqname tests run with -tr test-astbuild-pqname.

TESTS += pqname/pqname1.c

out/pqname/%.cc.ok: pqname/%.cc pqname/%.cc.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pqname/$*.cc.actual \
	  --expect pqname/$*.cc.expect \
	  $(CCPARSE) -tr test-astbuild-pqname $<
	touch $@


TESTS += pqname/pqname2.cc

out/pqname/%.c.ok: pqname/%.c pqname/%.c.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pqname/$*.c.actual \
	  --expect pqname/$*.c.expect \
	  $(CCPARSE) -xc -tr test-astbuild-pqname $<
	touch $@


# ------------------------------ tcheck --------------------------------
# Type-check tests simply run ccparse and expect it to do its own
# internal validation with internal testing hooks.

TESTS += tcheck/conv-void-ptr.c
TESTS += tcheck/expr-type.c
TESTS += tcheck/func-ret-func.c
TESTS += tcheck/func-void-param.c
TESTS += tcheck/hello-world.c
TESTS += tcheck/make-typedef.c
TESTS += tcheck/ptr-plus-equals.c

# Run a C tcheck test
out/tcheck/%.c.ok: tcheck/%.c $(CCPARSE)
	@mkdir -p $(dir $@)
	$(PERL) ../multitest.pl $(CCPARSE) -xc tcheck/$*.c
	touch $@


# ---------------------------- unit tests ------------------------------
out/unit-tests.ok: $(CCPARSE)
	@mkdir -p $(dir $@)
	./$(CCPARSE) --unit-tests
	touch $@

check: out/unit-tests.ok


# ---------------------------- check rule ------------------------------
check: $(patsubst %,out/%.ok,$(TESTS))
	@echo "All tests in test/ passed."


# ----------------------------- cleanup --------------------------------
clean:
	rm -rf out


# EOF
