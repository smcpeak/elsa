# elsa/test/Makefile
# Makefile-based tests.

# I'm transitioning away from 'regrtest' in favor of 'make'-based tests
# since the latter are easier to run individually and parallize.

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
# Pretty-print tests run with --prettyPrint and compare the output to
# a "known good" expected output.

TESTS += pprint/forward.cc
TESTS += pprint/pp-decl.cc
TESTS += pprint/pp-syntax.cc
TESTS += pprint/precedence.cc
TESTS += pprint/vararg-promote.cc

# Run a C++ pretty-print test.
out/pprint/%.cc.ok: pprint/%.cc pprint/%.cc.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.cc.actual \
	  --expect pprint/$*.cc.expect \
	  $(CCPARSE) --prettyPrint $<
	touch $@


TESTS += pprint/func.c
TESTS += pprint/typedef.c
TESTS += pprint/vararg-promote.c

# Run a C pretty-print test.
out/pprint/%.c.ok: pprint/%.c pprint/%.c.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.c.actual \
	  --expect pprint/$*.c.expect \
	  $(CCPARSE) -xc --prettyPrint $<
	touch $@


# ------------------------------ strlit --------------------------------
# String literal tests run with --printStringLiterals and compare the
# output to expected output.

TESTS += strlit/strlit1.cc

# Run a C++ string literal test.
out/strlit/%.cc.ok: strlit/%.cc strlit/%.cc.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/strlit/$*.cc.actual \
	  --expect strlit/$*.cc.expect \
	  $(CCPARSE) --printStringLiterals $<
	touch $@


TESTS += strlit/multiline.c

# Run a C string literal test.
out/strlit/%.c.ok: strlit/%.c strlit/%.c.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/strlit/$*.c.actual \
	  --expect strlit/$*.c.expect \
	  $(CCPARSE) -xc --printStringLiterals $<
	touch $@


# ------------------------------ tcheck --------------------------------
# Type-check tests simply run ccparse and expect it to do its own
# internal validation with internal testing hooks.

TESTS += tcheck/expr-type.c

# Run a C tcheck test
out/tcheck/%.c.ok: tcheck/%.c $(CCPARSE)
	@mkdir -p $(dir $@)
	$(PERL) ../multitest.pl $(CCPARSE) -xc tcheck/$*.c
	touch $@


# ---------------------------- check rule ------------------------------
check: $(patsubst %,out/%.ok,$(TESTS))
	@echo "All tests in test/ passed."


# ----------------------------- cleanup --------------------------------
clean:
	rm -rf out


# EOF
