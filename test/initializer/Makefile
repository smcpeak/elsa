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
GCC       = gcc
MULTITEST = $(PERL) ../../multitest.pl
CCPARSE   = ../../ccparse.exe


# Use $(CC) and multitest to check %.c.
out/gcc-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) -sut gcc $(GCC) -c -o /dev/null -std=c11 -pedantic-errors $*.c
	$(GCC) -o out/gcc-check/$*.exe $*.c
	./out/gcc-check/$*.exe
	touch $@


# Use $(CCPARSE) and multitest to check %.c.
out/elsa-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) -sut elsa $(CCPARSE) -xc $*.c
	touch $@


# Test both of them.
out/both-check/%.c: out/cc-check/%.c out/elsa-check/%.c
	$(CREATE_OUTPUT_DIRECTORY)
	touch $@


# Elsa wrongly accepts some of the errors, so I'm using cc-check.
check: out/gcc-check/array-designator-expression.c
check: out/gcc-check/block-scope-linkage.c
check: out/gcc-check/complete-object.c
check: out/gcc-check/constant-exprs.c
check: out/gcc-check/field-designator.c
check: out/gcc-check/init-scalar.c


clean:
	rm -rf out
	$(RM) *.error*.c


# EOF
