include ../common.am

AM_CFLAGS = $(CODEMODEL) $(SHARED_CFLAGS) $(OPT_CFLAGS) $(DEBUG_CFLAGS) $(WARN_CFLAGS) $(PROF_CFLAGS)
AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/bitfunc -I$(srcdir)/.. $(FS_INCLUDES)
AM_LDFLAGS = $(CODEMODEL)


normal_headers = bitfunc/config.h \
 bitfunc/types.h bitfunc/bitvec.h bitfunc/aiger.h bitfunc/array.h bitfunc/circuits.h bitfunc/circuits_internal.h bitfunc/bitfunc_internal.h bitfunc/mem.h bitfunc/program.h bitfunc/program_internal.h bitfunc/machine_state.h bitfunc/debug.h bitfunc/sparsebits.h \
 bitfunc/pcode_definitions.h bitfunc/llvm_definitions.h bitfunc/bap.h \
 bitfunc/klee_mem.h

normal_sources =  array.c circuits.c bitfunc.c bitvec.c mem.c cbranch.c program.c pcode_definitions.c llvm_definitions.c debug.c solve.c machine_state.c sparsebits.c bap.c \
  klee_mem.c

normal_built_headers = 
normal_built_sources = 

noinst_PROGRAMS = lemon
lemon_CFLAGS = 

EXTRA_DIST = lempar.c bap_parse.y bap_lex.l samepath.py

CLEANFILES = backbone-nw.c bitfunc/backbone-nw.h bap_parse.c bap_parse.h bap_parse.out bap_lex.c bap_lex.h parser.stamp lexer.stamp

# EXTRA_DIST = backbone.nw

if LLVM_PY_ENABLED
  llvm_py_sources = python/bitfunc/llvm_symex.py
else
  llvm_py_sources =
endif

bitfunc_PYTHON = python/bitfunc/expdag.py python/bitfunc/__init__.py \
  python/bitfunc/sexp_parser.py python/bitfunc/parse_smt2.py \
  python/bitfunc/qc.py \
  $(llvm_py_sources)
bitfuncdir = $(pythondir)/bitfunc

lib_LTLIBRARIES = libbitfunc.la
noinst_LTLIBRARIES = libaiger.la libbapparse.la libbfabc_bridge.la

nobase_include_HEADERS = \
 $(normal_headers)

nobase_nodist_include_HEADERS = \
 $(normal_built_headers)

libaiger_la_SOURCES = aiger.c
libaiger_la_CFLAGS = $(AM_CFLAGS) -Wno-error

libbfabc_bridge_la_SOURCES = abc_bridge.c
libbfabc_bridge_la_CFLAGS = $(AM_CFLAGS) -Wno-error

libbitfunc_la_SOURCES = \
 $(normal_sources)

if HAVE_PERFPROBES
libbitfunc_la_SOURCES += bfprobes.c bfprobes.h
endif

libbitfunc_la_CFLAGS = $(AM_CFLAGS) $(PERFPROBES_CFLAGS)
libbitfunc_la_LIBADD = libaiger.la libbapparse.la $(PERFPROBES_LIBS) $(FS_LIBS)

nodist_libbitfunc_la_SOURCES = \
 $(normal_built_sources)

INDENT = gnuindent
INDENT_FLAGS = -kr -l80 -ci2 -i2

# its own lib because we need special CFLAGS; stupid lemon
libbapparse_la_SOURCES = bap_lex.c bap_lex.h bap_parse.h bap_parse.c bap_parse_types.h
libbapparse_la_CFLAGS = $(AM_CFLAGS) -Wno-error

BUILT_SOURCES = bap_parse.h bap_parse.c bap_lex.h bap_lex.c

# bap_parse.c: bap_parse.y
# 	$(LEMON) $^

# from the automake manual
parser.stamp: lemon
parser.stamp: bap_parse.y
	@rm -f parser.tmp
	@touch parser.tmp
	if $(PYTHON) $(srcdir)/samepath.py "$<" "bap_parse.y"; then cp -f "$<" bap_parse.y ; fi
	./lemon T=$(srcdir)/lempar.c bap_parse.y
	if $(PYTHON) $(srcdir)/samepath.py "$<" "bap_parse.y"; then rm -f bap_parse.y ; fi
	@cp -f parser.tmp $@
	@rm -f parser.tmp
## Recover from the removal of $@
bap_parse.c bap_parse.h bap_parse.out: parser.stamp
	@if test -f $@; then :; else \
		rm -f parser.stamp; \
		$(MAKE) $(AM_MAKEFLAGS) parser.stamp; \
	fi


lexer.stamp: bap_lex.l
	@rm -f lexer.tmp
	@touch lexer.tmp
	$(LEX) $<
	@cp -f lexer.tmp $@
	@rm -f lexer.tmp
bap_lex.c bap_lex.h: lexer.stamp
	@if test -f $@; then :; else \
	  rm -f parser.stamp; \
	  $(MAKE) $(AM_MAKEFLAGS) lexer.stamp; \
	fi
