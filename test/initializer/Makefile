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


# Use $(GCC) and multitest to check %.c.
out/gcc-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) -sut gcc $(GCC) -c -o /dev/null -std=c11 -pedantic-errors $*.c
	$(GCC) -o out/gcc-check/$*.exe $*.c
	./out/gcc-check/$*.exe
	touch $@


# Use $(CLANG) and multitest to check %.c.
out/clang-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) -sut clang $(CLANG) -c -o /dev/null -std=c11 -pedantic-errors $*.c
	$(CLANG) -o out/clang-check/$*.exe $*.c
	./out/clang-check/$*.exe
	touch $@


# Use $(CCPARSE) and multitest to check %.c.
out/elsa-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) -sut elsa $(CCPARSE) -xc $*.c
	touch $@


# Test all of them.
out/all-check/%.c: out/gcc-check/%.c out/clang-check/%.c out/elsa-check/%.c
	$(CREATE_OUTPUT_DIRECTORY)
	touch $@


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
# p16 is covered by p13-struct-single-expression.c.
# p17: TODO: Designator tests.
# p18: Would be covered by p17 tests.
check: out/all-check/p19-override-eval.c
# p20: Also p17.
check: out/all-check/p21-short-init.c
check: out/all-check/p22-array-unspec-size.c
check: out/all-check/p23-eval-sequence.c
# p24 and later: TODO: Make these examples into tests.


clean:
	rm -rf out
	$(RM) *.error*.c


# EOF
