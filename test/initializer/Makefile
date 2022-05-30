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
CC        = gcc
MULTITEST = $(PERL) ../../multitest.pl
CCPARSE   = ../../ccparse.exe


# Use $(CC) and multitest to check %.c.
out/cc-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) $(CC) -c -o /dev/null -std=c11 -pedantic-errors $*.c
	$(CC) -o out/cc-check/$*.exe $*.c
	./out/cc-check/$*.exe
	touch $@


# Use $(CCPARSE) and multitest to check %.c.
out/elsa-check/%.c: %.c
	$(CREATE_OUTPUT_DIRECTORY)
	$(MULTITEST) $(CCPARSE) -xc $*.c
	touch $@


# Test both of them.
out/both-check/%.c: out/cc-check/%.c out/elsa-check/%.c
	$(CREATE_OUTPUT_DIRECTORY)
	touch $@


# Elsa wrongly accepts some of the errors, so I'm using cc-check.
check: out/cc-check/array-designator-expression.c
check: out/cc-check/block-scope-linkage.c
check: out/cc-check/complete-object.c
check: out/cc-check/constant-exprs.c


clean:
	rm -rf out
	$(RM) *.error*.c


# EOF
