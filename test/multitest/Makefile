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
MULTITEST          = $(PERL) ../../multitest.pl
RUN_COMPARE_EXPECT = $(PYTHON3) $(SMBASE)/run-compare-expect.py


# Create an empty expect file if it is missing.
%.expect:
	touch $@


# Run multitest gcc-run under RCE.
out/gcc-run/%.c: %.c %.c.expect
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/gcc-run/$*.c.actual \
	  --expect $*.c.expect \
	  $(MULTITEST) ./gcc-run $*.c
	touch $@


check: out/gcc-run/simple.c


clean:
	rm -rf out
	$(RM) *.error*.c

# EOF
