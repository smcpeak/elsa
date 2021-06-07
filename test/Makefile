# elsa/test/Makefile
# Makefile-based tests.

# I'm transitioning away from 'regrtest' in favor of 'make'-based tests
# since the latter are easier to run individually and parallize.

# Default rule.
all: check

# Eliminate all implicit rules.
.SUFFIXES:

# Delete a target when its recipe fails.
.DELETE_ON_ERROR:

# Do not remove "intermediate" targets.
.SECONDARY:


# Name of parser binary.
#
# TODO: Have Elsa always create ccparse with the ".exe" extension.
CCPARSE := $(wildcard ../ccparse.exe ../ccparse)

# --------------------- interpreter tests ------------------------
TESTS :=
TESTS += strlit/multiline.c
TESTS += strlit/strlit1.cc

check: $(patsubst %,out/%.ok,$(TESTS))


# If we are missing an expect file, just make an empty one.
%.expect:
	touch $@


# Run a C++ string literal test.
out/strlit/%.cc.ok: strlit/%.cc strlit/%.cc.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(PYTHON3) ./run-compare-expect.py \
	  --actual out/strlit/$*.cc.actual \
	  --expect strlit/$*.cc.expect \
	  $(CCPARSE) --quiet -tr printStringLiterals $<
	touch $@

# Run a C string literal test.
out/strlit/%.c.ok: strlit/%.c strlit/%.c.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(PYTHON3) ./run-compare-expect.py \
	  --actual out/strlit/$*.c.actual \
	  --expect strlit/$*.c.expect \
	  $(CCPARSE) --quiet -xc -tr printStringLiterals $<
	touch $@


# ------------------------- cleanup ----------------------------
check:
	@echo "All tests in test/ passed."

clean:
	rm -rf out


# EOF
