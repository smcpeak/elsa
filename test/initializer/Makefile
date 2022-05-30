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


# Elsa wrongly accepts some of the errors, so I'm using gcc-check.
check: out/all-check/array-designator-expression.c
check: out/all-check/array-of-char.c
check: out/all-check/array-of-char-in-func.c
check: out/all-check/block-scope-linkage.c
check: out/all-check/complete-object.c
check: out/all-check/constant-exprs.c
check: out/all-check/field-designator.c
check: out/all-check/init-scalar.c
check: out/all-check/struct-single-expression.c


clean:
	rm -rf out
	$(RM) *.error*.c


# EOF
