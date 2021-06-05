# elsa/test/Makefile
# New set of tests driven by 'make'.

# Default rule.
all: check

# Eliminate all implicit rules.
.SUFFIXES:

# Delete a target when its recipe fails.
.DELETE_ON_ERROR:

# Do not remove "intermediate" targets.
.SECONDARY:


# Name of ccparse binary.
CCPARSE := ../ccparse

# --------------------- interpreter tests ------------------------
INTERP_TESTS :=
INTERP_TESTS += interp/main0.cc

check: $(patsubst interp/%,out/interp/%.ok,$(INTERP_TESTS))


# How to run the interpreter.
INTERP := $(CCPARSE) --interp


# If we are missing an expect file, just make an empty one.
interp/%.expect:
	touch $@


# Run one interpreter test.
out/interp/%.ok: interp/% interp/%.expect $(CCPARSE)
	@mkdir -p $(dir $@)
	$(INTERP) $< >out/interp/$*.actual 2>out/interp/$*.stderr; \
	  echo "Exit code: $$?" >out/interp/$*.exit
	cat out/interp/$*.stderr >>out/interp/$*.actual
	cat out/interp/$*.exit >>out/interp/$*.actual
	@echo diff -b interp/$*.expect out/interp/$*.actual
ifeq ($(UPDATE_EXPECT),1)
	@if diff -b -u interp/$*.expect out/interp/$*.actual; then true; else \
	  echo ./update-expect.sh interp/$*; \
	  ./update-expect.sh interp/$*; \
	fi
endif
	@if diff -b -u interp/$*.expect out/interp/$*.actual; then true; else \
	  echo "Test output differs.  If it is right, run:"; \
	  echo "  ./update-expect.sh interp/$*"; \
	  echo "or re-run with UPDATE_EXPECT=1 to update them all."; \
	  exit 2; \
	fi
	touch $@


# ------------------------- cleanup ----------------------------
check:
	echo "All tests in test/ passed."

clean:
	rm -rf out


# EOF
