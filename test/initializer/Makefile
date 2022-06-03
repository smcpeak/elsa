# initializer/Makefile
# These tests are meant, initially, to confirm my understanding of the
# semantics of C initializers.  Then, I intend to transition them to
# tests of Elsa's conformance to the C rules.  Eventually C++ will be
# tested as well.

all: check
.PHONY: all check

SMBASE := ../../../smbase
include $(SMBASE)/sm-lib.mk

# Tools.
PERL      = perl
MULTITEST = $(PERL) ../../multitest.pl
CCPARSE   = ../../ccparse.exe

# Currently I'm using GCC-9.3.0.
GCC       = gcc

# Currently I'm using Clang-14.0.0.
CLANG     = clang

# By default, don't actually run the GCC and Clang tests, since that is
# primarily meant to validate the tests themselves.  This can be
# overridden in personal.mk.
RUN_GCC_INITIALIZER_TESTS = 0
RUN_CLANG_INITIALIZER_TESTS = 0


# Pull in customizations.
-include ../../personal.mk


# Use $(GCC) and multitest to check %.c.
out/gcc-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
ifeq ($(RUN_GCC_INITIALIZER_TESTS),1)
	$(MULTITEST) -sut gcc $(GCC) -c -o /dev/null -std=c11 -pedantic-errors $*.c
	$(GCC) -o out/gcc-check/$*.exe $*.c
	./out/gcc-check/$*.exe
endif
	touch $@

# GCC without -pedantic-errors.
#
# Although most of this directory is about testing conformance to the
# C11 standard, I also want to test some GNU extensions.
out/gcc-relaxed-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
ifeq ($(RUN_GCC_INITIALIZER_TESTS),1)
	$(MULTITEST) -sut gcc-relaxed $(GCC) -c -o /dev/null -std=c11 $*.c
	$(GCC) -o out/gcc-relaxed-check/$*.exe $*.c
	./out/gcc-relaxed-check/$*.exe
endif
	touch $@


# Use $(CLANG) and multitest to check %.c.
out/clang-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
ifeq ($(RUN_CLANG_INITIALIZER_TESTS),1)
	$(MULTITEST) -sut clang $(CLANG) -c -o /dev/null -std=c11 -pedantic-errors $*.c
	$(CLANG) -o out/clang-check/$*.exe $*.c
	./out/clang-check/$*.exe
endif
	touch $@

# Clang without -pedantic-errors.
out/clang-relaxed-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
ifeq ($(RUN_CLANG_INITIALIZER_TESTS),1)
	$(MULTITEST) -sut clang-relaxed $(CLANG) -c -o /dev/null -std=c11 $*.c
	$(CLANG) -o out/clang-relaxed-check/$*.exe $*.c
	./out/clang-relaxed-check/$*.exe
endif
	touch $@


# Use $(CCPARSE) -pedantic-errors and multitest to check %.c.
out/elsa-check/%.c: %.c $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) -sut elsa $(CCPARSE) -pedantic-errors $*.c
	touch $@

# $(CCPARSE) without pedantic errors.
out/elsa-relaxed-check/%.c: %.c $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) -sut elsa-relaxed $(CCPARSE) $*.c
	touch $@


# Test all of them.
out/all-check/%.c: out/gcc-check/%.c out/clang-check/%.c out/elsa-check/%.c
	$(CREATE_OUTPUT_DIRECTORY)
	touch $@

# All in relaxed mode.
out/all-relaxed-check/%.c: out/gcc-relaxed-check/%.c out/clang-relaxed-check/%.c out/elsa-relaxed-check/%.c
	$(CREATE_OUTPUT_DIRECTORY)
	touch $@


# Test just gcc and clang.  This is for elsa bugs where it cannot even
# parse the main file without any syntax errors in it.
out/non-elsa-check/%.c: out/gcc-check/%.c out/clang-check/%.c
	$(CREATE_OUTPUT_DIRECTORY)
	touch $@


# The tests whose name begins with "pNN-" are based on C11 6.7.9,
# paragraph NN.

# p01 is the grammar syntax.
check: out/all-check/p02-init-non-contained.c
check: out/all-check/p03-complete-object.c
check: out/all-check/p04-constant-exprs.c
check: out/all-check/p05-block-scope-linkage.c
check: out/all-check/p06-array-designator-expression.c
check: out/all-check/p07-field-designator.c
# p08 ("specifies the initial value") is tested by all.
# p09 ("unnamed indeterminate") cannot be tested.
check: out/all-check/p10-zero-init.c
check: out/all-check/p11-init-scalar.c
# p12 ("rest of this subclause deals with ...") is informative.
check: out/all-check/p13-struct-single-expression.c
check: out/all-check/p14-array-of-char-in-func.c
check: out/all-check/p14-array-of-char.c
# p15 ("wchar_t") is a TODO.
# p16: Covered by p13-struct-single-expression.c.
check: out/non-elsa-check/p17-designators.c
# p18: Covered by p17-designators.c.
check: out/all-check/p19-override-eval.c
# p20: Covered by p17-designators.c.
check: out/all-check/p21-short-init.c
check: out/all-check/p22-array-unspec-size.c
check: out/all-check/p23-eval-sequence.c
check: out/all-check/p24-ex-single-expr-init.c
check: out/all-check/p25-ex-array-unspec-size.c
check: out/all-check/p26-ex-2d-array.c
check: out/all-check/p27-ex-2d-array-one-column.c
check: out/non-elsa-check/p28-ex-inconsistent-brackets.c
check: out/all-check/p29-ex-partial-brackets.c
# p30 ("less likely to cause confusion") is informative.
check: out/non-elsa-check/p31-ex-typedef-incomplete-array.c
check: out/all-check/p32-ex-string-lit-init.c
check: out/all-check/p33-ex-enum-array-desig.c
check: out/all-check/p34-ex-field-desig.c
check: out/all-check/p35-ex-designator-to-clarify.c
check: out/non-elsa-check/p36-ex-alloc-both-ends.c
# p37 is additional commentary on the example in p36.
check: out/all-check/p38-ex-any-union-member.c

# Other sections.
check: out/all-check/c11-6.7.2.1p1-no-empty-struct.c
check: out/all-check/c11-6.7.2.1p3-flexible-array.c
check: out/all-check/c11-6.7.6.2p1-no-empty-array.c

# Other tests, not specifically tied to sections in C11 standard.
check: out/all-check/deep-nesting.c

# Tests that need relaxed checking mode.
check: out/all-relaxed-check/empty-aggregate.c
check: out/all-relaxed-check/range-designators.c


clean:
	rm -rf out
	$(RM) *.error*.c


# EOF
