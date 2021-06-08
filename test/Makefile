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
CCPARSE := ../ccparse.exe

# --------------------- interpreter tests ------------------------
TESTS :=
TESTS += pprint/pp-syntax.cc
TESTS += pprint/vararg-promote.c
TESTS += pprint/vararg-promote.cc
TESTS += strlit/multiline.c
TESTS += strlit/strlit1.cc

check: $(patsubst %,out/%.ok,$(TESTS))


# If we are missing an expect file, just make an empty one.
%.expect:
	touch $@


# Run a C++ pretty-print test.
out/pprint/%.cc.ok: pprint/%.cc pprint/%.cc.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(PYTHON3) ./run-compare-expect.py \
	  --actual out/pprint/$*.cc.actual \
	  --expect pprint/$*.cc.expect \
	  $(CCPARSE) --quiet -tr prettyPrint $<
	touch $@


# Run a C pretty-print test.
out/pprint/%.c.ok: pprint/%.c pprint/%.c.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(PYTHON3) ./run-compare-expect.py \
	  --actual out/pprint/$*.c.actual \
	  --expect pprint/$*.c.expect \
	  $(CCPARSE) --quiet -xc -tr prettyPrint $<
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
