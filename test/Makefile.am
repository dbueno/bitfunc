include ../common.am

AM_CFLAGS = $(CODEMODEL) $(SHARED_CFLAGS) $(OPT_CFLAGS) $(DEBUG_CFLAGS) $(WARN_CFLAGS) $(PROF_CFLAGS)
AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src $(FS_INCLUDES)
AM_LDFLAGS = $(CODEMODEL) -no-install


# TANGLE = ctangle
# WEAVE = cweave
INDENT = gnuindent
INDENT_FLAGS = -kr -l80 -ci2 -i2
# PDFTEX = pdftex


noinst_PROGRAMS = bitvec-test machine_state_test mem_test synthesis-swap synthesis-crc incremental-test pcode-aig gcd-test popcnt bftest adder gcd2
TESTS = bitvec-test mem_test incremental-test popcnt bftest machine_state_test gcd2

if HAVE_CWEB
%.c: %.w
	$(CTANGLE) $^ - $@
%.tex: %.w
	$(CWEAVE) $^ - $@
endif

if HAVE_PDFTEX
%.pdf: %.tex
	$(PDFTEX) $^
endif

# so I can easily generate bitcode
%.bc: %.c
	clang -emit-llvm -std=gnu99 -c $(AM_CPPFLAGS) $(AM_CFLAGS) $(PERFPROBES_CFLAGS) -o $@ $^


bitvec_test_CFLAGS = $(AM_CFLAGS) $(PERFPROBES_CFLAGS)
bitvec_test_SOURCES = bitvec-test.c mitre.h mitre.c
bitvec_test_LDADD = ../src/libbitfunc.la $(FS_LIBS) $(PERFPROBES_LIBS)

machine_state_test_SOURCES = machine_state_test.c mitre.h mitre.c gen.c genrand.h
machine_state_test_LDADD = ../src/libbitfunc.la $(FS_LIBS)

adder_SOURCES = adder.c
adder_LDADD = ../src/libbitfunc.la $(FS_LIBS)

bftest_SOURCES = bftest.c mitre.h mitre.c popcnt_defs.c gen.c genrand.h
bftest_LDADD = ../src/libbfabc_bridge.la ../src/libbitfunc.la $(FS_LIBS)

incremental_test_SOURCES = incremental-test.c mitre.h mitre.c
incremental_test_LDADD = ../src/libbitfunc.la $(FS_LIBS)

mem_test_SOURCES = mem_test.c mitre.h mitre.c
mem_test_LDADD = ../src/libbitfunc.la $(FS_LIBS)

gcd_test_SOURCES = gcd.c
gcd_test_LDADD = ../src/libbitfunc.la $(FS_LIBS)

gcd2_SOURCES = gcd2.c
gcd2_LDADD = ../src/libbitfunc.la $(FS_LIBS)

synthesis_swap_SOURCES = synthesis_swap.c
synthesis_swap_LDADD = ../src/libbitfunc.la $(FS_LIBS)

# synthesis_swap2_SOURCES = synthesis_swap2.c
# synthesis_swap2_LDADD = ../src/libbitfunc.la $(FS_LIBS)

synthesis_crc_SOURCES = synthesis_crc.c
synthesis_crc_LDADD = ../src/libbitfunc.la $(FS_LIBS)

# synthesis_store_SOURCES = synthesis_store.c
# synthesis_store_LDADD = ../src/libbitfunc.la $(FS_LIBS)

# pcode_test_SOURCES = pcode_test.c
# pcode_test_LDADD = ../src/libbitfunc.la $(FS_LIBS)

popcnt_SOURCES = popcnt.c popcnt_defs.c
popcnt_LDADD = ../src/libbitfunc.la $(FS_LIBS)

# regexp_SOURCES = regexp.c
# regexp_LDADD = ../src/libbitfunc.la $(FS_LIBS)

pcode_aig_SOURCES = pcode-aig.c
pcode_aig_LDADD = ../src/libbitfunc.la $(FS_LIBS)

.PHONY: allaigs
allaigs: pINT_ADD.aig pINT_SUB.aig pINT_MULT.aig pINT_DIV.aig pINT_REM.aig pINT_SDIV.aig pINT_SREM.aig pINT_OR.aig pINT_XOR.aig pINT_AND.aig pINT_LEFT.aig pINT_RIGHT.aig pINT_SRIGHT.aig pINT_EQUAL.aig pINT_NOTEQUAL.aig pINT_LESS.aig pINT_LESSEQUAL.aig pINT_CARRY.aig pINT_SCARRY.aig pINT_SBORROW.aig pBOOL_OR.aig pBOOL_XOR.aig pBOOL_AND.aig pBOOL_OR.aig pBOOL_NEGATE.aig pPIECE.aig pSUBPIECE.aig pINT_ZEXT.aig pINT_SEXT.aig pINT_2COMP.aig pINT_NEGATE.aig


%.aig: pcode-aig
	./pcode-aig $@

clean-local::
	rm -f *.aig


