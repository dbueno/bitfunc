EXTRA_DIST = README
SUBDIRS = src test .
dist_doc_DATA = README

AM_CFLAGS = @CODEMODEL@
AM_LDFLAGS = @CODEMODEL@

ACLOCAL_AMFLAGS = --install -I m4

bin_PROGRAMS = funcsat check_solution

funcsat_SOURCES = main.c
funcsat_LDADD = src/libfuncsat.la
funcsat_CPPFLAGS = -I$(srcdir)/src

check_solution_SOURCES = check_solution.c
check_solution_LDADD = src/libfuncsat.la
check_solution_CPPFLAGS = -I$(srcdir)/src


doxygen: Doxyfile
	mkdir -p doc/doxygen
	doxygen $^
clean-local:
	rm -rf doc/doxygen
	rm -f *.bc funcsat-llvm.opt

llvm:
	make -C src
	make funcsat-main.o
	llvm-ld -b=funcsat-llvm.unopt.bc -internalize -internalize-public-api-list=main funcsat-main.o src/libfuncsat.a
	opt -O3 -o funcsat-llvm.bc funcsat-llvm.unopt.bc
	llvm-ld -o=funcsat-llvm.opt -internalize -internalize-public-api-list=main $(addprefix -Xlinker=,@LIBS@) -native funcsat-llvm.bc
