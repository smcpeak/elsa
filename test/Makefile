# elsa/test/Makefile
# Makefile-based tests.

# I'm transitioning away from 'regrtest' in favor of 'make'-based tests
# since the latter are easier to run individually and parallelize.

# Default rule.
all: check


# -------------------------- configuration -----------------------------
# Other repos, expressed relative to the *parent* directory.
SMBASE  = ../smbase

# Locations relative to *this* directory.
SMBASE_FROM_HERE = ../$(SMBASE)

# External tools.
PYTHON3 = python3
PERL    = perl
GCC     = gcc

# How to invoke run-compare-expect.py.
RUN_COMPARE_EXPECT = $(PYTHON3) $(SMBASE_FROM_HERE)/run-compare-expect.py

# How to invoke multitest.pl.
RUN_MULTITEST = $(PERL) ../multitest.pl

# Name of parser binary.
CCPARSE = ../ccparse.exe

# Pull in automatic and manual configuration adjustments.
include ../config.mk
-include ../personal.mk


# ------------------------------ rules ---------------------------------
# Defines CREATE_OUTPUT_DIRECTORY, among other things.
include $(SMBASE_FROM_HERE)/sm-lib.mk

# Like $(CREATE_OUTPUT_DIRECTORY), but also create the 'idem'
# subdirectory underneath it so the 'idemcheck' script will work.
CREATE_OUTPUT_DIRECTORY_IDEM = @mkdir -p $(dir $@)idem


# List of tests.  This gets added to below.
TESTS :=

# If we are missing an expect file, just make an empty one.
%.expect:
	touch $@


# ------------------------------ pprint --------------------------------
# Pretty-print tests run with --pretty-print and compare the output to
# a "known good" expected output.

TESTS += pprint/class.cc
TESTS += pprint/class2.cc
TESTS += pprint/conversion-op.cc
TESTS += pprint/forward.cc
TESTS += pprint/gnu-attr-on-conversion-id.cc
TESTS += pprint/longlines.cc
TESTS += pprint/method-defn.cc
TESTS += pprint/pp-decl.cc
TESTS += pprint/pp-enum.cc
TESTS += pprint/pp-syntax.cc
TESTS += pprint/precedence.cc
TESTS += pprint/template1.cc
TESTS += pprint/throwspec.cc
TESTS += pprint/try-catch.cc
TESTS += pprint/vararg-promote.cc

# Run a C++ pretty-print test.
out/pprint/%.cc.ok: pprint/%.cc pprint/%.cc.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY_IDEM)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.cc.actual \
	  --expect pprint/$*.cc.expect \
	  $(CCPARSE) --pretty-print $<
#	The 'cpp' here is to get rid of the template instantiations,
#	which are bracketed by #if 0/#endif.
	../chop_out < out/pprint/$*.cc.actual | cpp > out/pprint/$*.cc.trimmed
	$(CCPARSE) out/pprint/$*.cc.trimmed
	g++ -c -oout/pprint/$*.cc.trimmed.o -xc++ out/pprint/$*.cc.trimmed
	../idemcheck --elsadir .. -d $(dir $@)idem -q $<
	touch $@


TESTS += pprint/ambig-cast-vs-funcall.c
TESTS += pprint/array-plus-int.c
TESTS += pprint/attr-may-alias.c
TESTS += pprint/big-for-loop.c
TESTS += pprint/bitfield.c
TESTS += pprint/builtin-offsetof.c
TESTS += pprint/cast-array-to-int.c
TESTS += pprint/character-literals.c
TESTS += pprint/compound-after-label.c
TESTS += pprint/const-enum.c
TESTS += pprint/const-struct.c
TESTS += pprint/deref-array.c
TESTS += pprint/for-loop-missing-elts.c
TESTS += pprint/for-loop-no-init.c
TESTS += pprint/func.c
TESTS += pprint/func-symbol.c
TESTS += pprint/funcptr.c
TESTS += pprint/gnu-attr-after-asm-label.c
TESTS += pprint/gnu-attr-after-enum.c
TESTS += pprint/gnu-attr-after-struct.c
TESTS += pprint/gnu-attr-in-declaration.c
TESTS += pprint/gnu-attr-in-declarator.c
TESTS += pprint/gnu-attr-in-type-specifier.c
TESTS += pprint/gnu-attr-mode.c
TESTS += pprint/gnu-attr-on-label.c
TESTS += pprint/gnu-attr-several.c
TESTS += pprint/gnu-declaration-attr.c
TESTS += pprint/if-without-else.c
TESTS += pprint/init-struct-with-string.c
TESTS += pprint/init-struct-with-string2.c
TESTS += pprint/logical-op-ptr.c
TESTS += pprint/long-decl.c
TESTS += pprint/long-parameters.c
TESTS += pprint/longlines2.c
TESTS += pprint/longlines3.c
TESTS += pprint/nested-block.c
TESTS += pprint/nested-field-access.c
TESTS += pprint/printf-cond-arg.c
TESTS += pprint/ptr-as-condition.c
TESTS += pprint/range-case.c
TESTS += pprint/return-zero-as-ptr.c
TESTS += pprint/sizeof.c
TESTS += pprint/stdconv.c
TESTS += pprint/stringlit.c
TESTS += pprint/typedef.c
TESTS += pprint/typedef-multi-declarator.c
TESTS += pprint/vararg-promote.c
TESTS += pprint/weak-alias.c

# Run a C pretty-print test.
#
# For these I also disable the elaborator because it does not do much
# in C and the things it does do (mainly about structure assignment)
# are things I do not want, at least as part of a pprint test.
out/pprint/%.c.ok: pprint/%.c pprint/%.c.expect pprint/%.c.isc.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY_IDEM)
#	Main pretty-print.
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.c.actual \
	  --expect pprint/$*.c.expect \
	  $(CCPARSE) --pretty-print --no-elaborate $<
#	Test that output can be parsed by Elsa and GCC.
	../chop_out < out/pprint/$*.c.actual > out/pprint/$*.c.trimmed
	$(CCPARSE) -xc out/pprint/$*.c.trimmed
	gcc -c -oout/pprint/$*.c.trimmed.o -xc out/pprint/$*.c.trimmed
	@#
	@# The '-xc' in the following command is required because,
	@# although the input is a .c file, 'idem' uses the ".cc"
	@# extension for its intermediate files.
	@#
	../idemcheck --elsadir .. -d $(dir $@)idem -xc --no-elaborate -q $<
#	Pretty-print with ISC enabled.
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.c.isc.actual \
	  --expect pprint/$*.c.isc.expect \
	  $(CCPARSE) --pretty-print --no-elaborate --print-isc $<
	touch $@


# Test without passing the result into GCC.
out/pprint/%.c.nogcc.ok: pprint/%.c pprint/%.c.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY_IDEM)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.c.actual \
	  --expect pprint/$*.c.expect \
	  $(CCPARSE) --pretty-print $<
	touch $@

check: out/pprint/asm.c.nogcc.ok
check: out/pprint/chained-labels.c.nogcc.ok
check: out/pprint/gnu-asm.c.nogcc.ok


# Test C++ without passing the result into GCC.
out/pprint/%.cc.nogcc.ok: pprint/%.cc pprint/%.cc.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY_IDEM)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pprint/$*.cc.actual \
	  --expect pprint/$*.cc.expect \
	  $(CCPARSE) --pretty-print $<
	touch $@

check: out/pprint/asm-cc.cc.nogcc.ok


# ------------------------------ strlit --------------------------------
# String literal tests run with --print-string-literals and compare the
# output to expected output.

TESTS += strlit/strlit1.cc

# Run a C++ string literal test.
out/strlit/%.cc.ok: strlit/%.cc strlit/%.cc.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/strlit/$*.cc.actual \
	  --expect strlit/$*.cc.expect \
	  --hex-replacer \
	  $(CCPARSE) --print-string-literals $<
	touch $@


TESTS += strlit/multiline.c

# Run a C string literal test.
out/strlit/%.c.ok: strlit/%.c strlit/%.c.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/strlit/$*.c.actual \
	  --expect strlit/$*.c.expect \
	  --hex-replacer \
	  $(CCPARSE) --print-string-literals $<
	touch $@


# ------------------------------ pqname --------------------------------
# pqname tests run with -tr test-astbuild-pqname.

TESTS += pqname/pqname1.c

out/pqname/%.cc.ok: pqname/%.cc pqname/%.cc.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pqname/$*.cc.actual \
	  --expect pqname/$*.cc.expect \
	  $(CCPARSE) -tr test-astbuild-pqname $<
	touch $@


TESTS += pqname/pqname2.cc

out/pqname/%.c.ok: pqname/%.c pqname/%.c.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/pqname/$*.c.actual \
	  --expect pqname/$*.c.expect \
	  $(CCPARSE) -tr test-astbuild-pqname $<
	touch $@


# ------------------------------ tcheck --------------------------------
# Type-check tests simply run ccparse and expect it to do its own
# internal validation with internal testing hooks.

TESTS += tcheck/builtin-offsetof.c
TESTS += tcheck/c99-array-declarator-qualifier.c
TESTS += tcheck/cast-btw-float-and-ptr.c
TESTS += tcheck/cast-of-union-literal.c
TESTS += tcheck/complex-operands.c
TESTS += tcheck/conv-void-ptr.c
TESTS += tcheck/designated-init-array-size.c
TESTS += tcheck/enum.c
TESTS += tcheck/expr-type.c
TESTS += tcheck/func-ret-func.c
TESTS += tcheck/func-void-param.c
TESTS += tcheck/gnu-alias.c
TESTS += tcheck/init-global-struct.c
TESTS += tcheck/hello-world.c
TESTS += tcheck/make-typedef.c
TESTS += tcheck/ptr-plus-equals.c
TESTS += tcheck/static-assert.c

# Run a C tcheck test
out/tcheck/%.c.ok: tcheck/%.c $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_MULTITEST) $(CCPARSE) tcheck/$*.c
	touch $@

#TESTS += tcheck/builtin-offsetof.cc  # TODO: Broken by switching to Designator.
TESTS += tcheck/new-int-star.cc
TESTS += tcheck/template-default-arg-templ-spec.cc

# Run a C++ tcheck test
out/tcheck/%.cc.ok: tcheck/%.cc $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_MULTITEST) $(CCPARSE) tcheck/$*.cc
	touch $@


# ------------------------------- run ----------------------------------
# Test that we can run the pprint output and it exits with code 0.
out/run/%: run/% $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	@#
	@# First check that it parses, and any errors detected.
	@#
	$(RUN_MULTITEST) $(CCPARSE) run/$*
	@#
	@# Now pretty-print, compile, and run.
	@#
	$(CCPARSE) --pretty-print run/$* > out/run/pprint-$*
	$(GCC) -o out/run/$*.exe out/run/pprint-$*
	./out/run/$*.exe
	touch $@

.PHONY: run-tests
check-run: out/run/anon-union-in-struct.c
check-run: out/run/hex-e-minus.c
check-run: out/run/offsetof-array-index.c

check: check-run


# ------------------------------ errmsg --------------------------------
# Check for specific error messages.
out/errmsg/%: errmsg/% errmsg/%.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/errmsg/$*.actual \
	  --expect errmsg/$*.expect \
	  $(CCPARSE) errmsg/$*
	touch $@

check-errmsg: out/errmsg/desig-too-large.c
check-errmsg: out/errmsg/empty-struct.c
check-errmsg: out/errmsg/field-desig-for-array.c
check-errmsg: out/errmsg/incomplete-struct-variable.c
#check-errmsg: out/errmsg/init-has-flex-array.c
check-errmsg: out/errmsg/invalid-field-desig.c
check-errmsg: out/errmsg/negative-size-array.c
check-errmsg: out/errmsg/struct-with-flex-array.c
check-errmsg: out/errmsg/too-many-inits.c
check-errmsg: out/errmsg/union-init-override.c
check-errmsg: out/errmsg/vla-with-init.c
check-errmsg: out/errmsg/zero-size-array.c


# Check while running -pedantic.
out/errmsg-pedantic/%: errmsg/% errmsg/%.pedantic.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/errmsg-pedantic/$*.actual \
	  --expect errmsg/$*.pedantic.expect \
	  $(CCPARSE) -pedantic errmsg/$*
	touch $@

check-errmsg: out/errmsg-pedantic/empty-struct.c


# Check while running -pedantic-errors.
out/errmsg-pedantic-errors/%: errmsg/% errmsg/%.pedantic-errors.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/errmsg-pedantic-errors/$*.actual \
	  --expect errmsg/$*.pedantic-errors.expect \
	  $(CCPARSE) -pedantic-errors errmsg/$*
	touch $@

check-errmsg: out/errmsg-pedantic-errors/empty-struct.c
check-errmsg: out/errmsg-pedantic-errors/empty-struct.cc
#check-errmsg: out/errmsg-pedantic-errors/init-has-flex-array.c
check-errmsg: out/errmsg-pedantic-errors/zero-size-array.c
check-errmsg: out/errmsg-pedantic-errors/zero-size-array.cc


.PHONY: check-errmsg
check: check-errmsg


# --------------------------- clang tests ------------------------------
# Run ccparse --clang and check the results with pprint.
#
# Warnings are suppressed with -w because my examples are filled with
# anomalies that provoke warnings that I do not want in the expected
# output.
out/clang-pprint-in/%: ../in/% pprint-in/%.clang.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/clang-pprint-in/$*.actual \
	  --expect pprint-in/$*.clang.expect \
	  $(CCPARSE) --pretty-print --clang -w $<
	touch $@

.PHONY: check-clang
ifeq ($(USE_CLANG),1)
check: check-clang
endif

check-clang: out/clang-pprint-in/t0001.cc
check-clang: out/clang-pprint-in/t0002.cc
check-clang: out/clang-pprint-in/t0003.cc
check-clang: out/clang-pprint-in/t0004.cc
check-clang: out/clang-pprint-in/t0005.cc
#check-clang: out/clang-pprint-in/t0006.cc
#check-clang: out/clang-pprint-in/t0007.cc
#check-clang: out/clang-pprint-in/t0008.cc
#check-clang: out/clang-pprint-in/t0009.cc
#check-clang: out/clang-pprint-in/t0010.cc
#check-clang: out/clang-pprint-in/t0011.cc
#check-clang: out/clang-pprint-in/t0012.cc

#check-clang: out/clang-pprint-in/c/t0001.c
#check-clang: out/clang-pprint-in/c/t0002.c
#check-clang: out/clang-pprint-in/c/t0003.c
#check-clang: out/clang-pprint-in/c/t0004.c
#check-clang: out/clang-pprint-in/c/t0005.c
#check-clang: out/clang-pprint-in/c/t0006.c
#check-clang: out/clang-pprint-in/c/t0007.c
#check-clang: out/clang-pprint-in/c/t0008.c
#check-clang: out/clang-pprint-in/c/t0009.c
#check-clang: out/clang-pprint-in/c/t0010.c
#check-clang: out/clang-pprint-in/c/t0011.c
#check-clang: out/clang-pprint-in/c/t0012.c
#check-clang: out/clang-pprint-in/c/t0013.c
#check-clang: out/clang-pprint-in/c/t0014.c
#check-clang: out/clang-pprint-in/c/t0015.c
#check-clang: out/clang-pprint-in/c/t0016.c
#check-clang: out/clang-pprint-in/c/t0017.c
#check-clang: out/clang-pprint-in/c/t0018.c
#check-clang: out/clang-pprint-in/c/t0019.c
#check-clang: out/clang-pprint-in/c/t0020.c
#check-clang: out/clang-pprint-in/c/t0021.c

# This test seems to be aimed at supporting some non-standard ways of
# redeclaring 'main', tolerated by Elsa in a fairly specific way.  Since
# this is primarily a code compliance test, I'll skip it for Clang.
#check-clang: out/clang-pprint-in/c/t0022.c

#check-clang: out/clang-pprint-in/c/t0023.c
#check-clang: out/clang-pprint-in/c/t0024.c
#check-clang: out/clang-pprint-in/c/t0025.c

# This is testing invalid code, and Clang properly rejects it.  Elsa
# accepts it for bug compatibility only.
#check-clang: out/clang-pprint-in/c/t0026.c

#check-clang: out/clang-pprint-in/c/t0027.c


# Run a C++ pretty-print test with --clang.
#
# Also test with --print-isc so I can verify the standard conversions I
# need are in place.
#
# This uses different expected output than when not using --clang
# because Clang loses some information, including:
#
#   * parameter names in prototypes (this is a defect in my importer;
#     Clang has the information in its AST)
#   * multiple tokens of concatenated string literals
#   * combined definition of a struct with declaration of an instance
#     of that struct
#
# There are also differences that are not a loss of information, but
# the same information presented differently:
#
#   * Elsa combines multiple standard conversions occurring at the same
#     place into one AST node, while Clang uses multiple AST nodes.
#
# In some cases, this information could be recovered by doing more work
# in the importer, but I have chosen not to.
#
out/clang-pprint/%: pprint/% pprint/%.clang.expect pprint/%.clang.isc.expect $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/clang-pprint/$*.actual \
	  --expect pprint/$*.clang.expect \
	  $(CCPARSE) --pretty-print --clang $<
	$(RUN_COMPARE_EXPECT) \
	  --actual out/clang-pprint/$*.actual \
	  --expect pprint/$*.clang.isc.expect \
	  $(CCPARSE) --pretty-print --print-isc --clang $<
	touch $@

# Clang C pprint tests.
#check-clang: out/clang-pprint/ambig-cast-vs-funcall.c
#check-clang: out/clang-pprint/array-plus-int.c
#check-clang: out/clang-pprint/attr-may-alias.c
#check-clang: out/clang-pprint/big-for-loop.c
#check-clang: out/clang-pprint/bitfield.c
#check-clang: out/clang-pprint/builtin-offsetof.c
#check-clang: out/clang-pprint/cast-array-to-int.c
#check-clang: out/clang-pprint/character-literals.c
#check-clang: out/clang-pprint/compound-after-label.c
#check-clang: out/clang-pprint/const-enum.c
#check-clang: out/clang-pprint/const-struct.c
#check-clang: out/clang-pprint/deref-array.c
#check-clang: out/clang-pprint/for-loop-missing-elts.c
#check-clang: out/clang-pprint/for-loop-no-init.c
#check-clang: out/clang-pprint/func.c
#check-clang: out/clang-pprint/func-symbol.c
#check-clang: out/clang-pprint/funcptr.c
#check-clang: out/clang-pprint/gnu-attr-after-asm-label.c
#check-clang: out/clang-pprint/gnu-attr-after-enum.c
#check-clang: out/clang-pprint/gnu-attr-after-struct.c
#check-clang: out/clang-pprint/gnu-attr-in-declaration.c
#check-clang: out/clang-pprint/gnu-attr-in-declarator.c
#check-clang: out/clang-pprint/gnu-attr-in-type-specifier.c
#check-clang: out/clang-pprint/gnu-attr-mode.c
#check-clang: out/clang-pprint/gnu-attr-on-label.c
#check-clang: out/clang-pprint/gnu-attr-several.c
#check-clang: out/clang-pprint/gnu-declaration-attr.c
#check-clang: out/clang-pprint/if-without-else.c
#check-clang: out/clang-pprint/init-struct-with-string.c
#check-clang: out/clang-pprint/init-struct-with-string2.c
#check-clang: out/clang-pprint/logical-op-ptr.c
#check-clang: out/clang-pprint/long-decl.c
#check-clang: out/clang-pprint/long-parameters.c
#check-clang: out/clang-pprint/longlines2.c
#check-clang: out/clang-pprint/longlines3.c
#check-clang: out/clang-pprint/nested-block.c
#check-clang: out/clang-pprint/nested-field-access.c
#check-clang: out/clang-pprint/printf-cond-arg.c
#check-clang: out/clang-pprint/ptr-as-condition.c
#check-clang: out/clang-pprint/range-case.c
#check-clang: out/clang-pprint/return-zero-as-ptr.c
#check-clang: out/clang-pprint/sizeof.c
#check-clang: out/clang-pprint/stdconv.c
#check-clang: out/clang-pprint/stringlit.c
#check-clang: out/clang-pprint/typedef.c
#check-clang: out/clang-pprint/typedef-multi-declarator.c
#check-clang: out/clang-pprint/vararg-promote.c
#check-clang: out/clang-pprint/weak-alias.c

# Clang C++ pprint tests.
#check-clang: out/clang-pprint/method-defn.cc
#check-clang: out/clang-pprint/try-catch.cc


# ---------------------------- unit tests ------------------------------
out/unit-tests.ok: $(CCPARSE)
	$(CREATE_OUTPUT_DIRECTORY)
	./$(CCPARSE) --unit-tests
	touch $@

check: out/unit-tests.ok


out/test-astbuild.ok: $(CCPARSE) test-astbuild.expect
	$(CREATE_OUTPUT_DIRECTORY)
	$(RUN_COMPARE_EXPECT) \
	  --actual out/test-astbuild.actual \
	  --expect test-astbuild.expect \
	  $(CCPARSE) --test-print-astbuild
	touch $@

check: out/test-astbuild.ok


# ----------------------------- subdirs --------------------------------
.PHONY: check-subdirs
check-subdirs:
	$(MAKE) -C multitest check
	$(MAKE) -C initializer check

check: check-subdirs


.PHONY: clean-subdirs
clean-subdirs:
	$(MAKE) -C multitest clean
	$(MAKE) -C initializer clean

clean: clean-subdirs


# ---------------------------- check rule ------------------------------
check: $(patsubst %,out/%.ok,$(TESTS))
	@echo "All tests in test/ passed."


# ----------------------------- cleanup --------------------------------
clean:
	rm -rf out


# EOF
