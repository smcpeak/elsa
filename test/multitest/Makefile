# multitest/Makefile
# Tests for multitest.pl.

all: check
.PHONY: all check

SMBASE := ../../../smbase
include $(SMBASE)/sm-lib.mk

# Tools.
PYTHON3            = python3
PERL               = perl
CC                 = gcc
MULTITEST_SCRIPT   = ../../multitest.pl
MULTITEST          = $(PERL) $(MULTITEST_SCRIPT)
RUN_COMPARE_EXPECT = $(PYTHON3) $(SMBASE)/run-compare-expect.py


# Create an empty expect file if it is missing.
%.expect:
	touch $@


# Run multitest gcc-run under RCE.
out/gcc-run/%.c: %.c %.c.expect $(MULTITEST_SCRIPT)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/gcc-run/$*.c.actual \
	  --expect $*.c.expect \
	  $(MULTITEST) ./gcc-run $*.c
	touch $@

check: out/gcc-run/simple.c
check: out/gcc-run/notworking.c


# Run multitest with a certain argument.
out/args/%.args: %.args %.args.expect $(MULTITEST_SCRIPT)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/args/$*.args.actual \
	  --expect $*.args.expect \
	  $(MULTITEST) $(shell cat $*.args) ./gcc-run simple.c
	touch $@

check: out/args/err-1.args
check: out/args/err-named-error.args
check: out/args/err-unknown.args


# Run with -sut sut1
out/sut1/%.c: %.c %.sut1.c.expect $(MULTITEST_SCRIPT)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/sut1/$*.c.actual \
	  --expect $*.sut1.c.expect \
	  $(MULTITEST) -sut sut1 ./gcc-run $*.c
	touch $@

check: out/sut1/notworking.c


clean:
	rm -rf out
	$(RM) *.error*.c

# EOF
