include ./common.am

AM_CFLAGS = $(CODEMODEL) $(SHARED_CFLAGS) $(OPT_CFLAGS) $(DEBUG_CFLAGS) $(WARN_CFLAGS) $(PROF_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) $(FS_INCLUDES)
AM_LDFLAGS = $(CODEMODEL)

ACLOCAL_AMFLAGS = --install -I m4

nobase_include_HEADERS = bitfunc.h

SUBDIRS = funcsat src . test
dist_doc_data = README


if HAVE_CYTHON

bitfunc_cy.c: bitfunc_cy.pyx _bitfunc_cy.pxd
	$(CYTHON) -o $@ -I$(builddir) $<

EXTRA_DIST = bitfunc_cy.pyx _bitfunc_cy.pxd
CLEANFILES = biftunc_cy.c

endif

pyexec_LTLIBRARIES = bitfunc_cy.la

bitfunc_cy_la_SOURCES = bitfunc_cy.c
bitfunc_cy_la_LDFLAGS = -module -avoid-version
bitfunc_cy_la_CPPFLAGS = $(AM_CPPFLAGS) $(PYTHON_CPPFLAGS)
bitfunc_cy_la_LIBADD = $(AM_LDADD) $(PYTHON_LDFLAGS) src/libbitfunc.la src/libbapparse.la
bitfunc_cy_la_CFLAGS = $(AM_CFLAGS) -Wno-error


BitfuncInterface.hs: src/libbitfunc.la
	c2hs -o $@ -C"$(AM_CPPFLAGS)"

Bitfunc.hs: Bitfunc.hsc
	hsc2hs $(AM_CPPFLAGS) $<

gen-c2hs: Bitfunc_hs.chs
	c2hs --cppopts='-U__BLOCKS__' --cppopts='-I.' --cppopts='-Isrc' --cppopts='-Ifuncsat' --cppopts='-Ifuncsat/src' $<
	touch gen-c2hs

BitfuncTest: Bitfunc_hs.hs BitfuncTest.hs gen-c2hs
	ghc --make -o BitfuncTest Bitfunc_hs.hs BitfuncTest.hs $(AM_LDADD) src/.libs/libbitfunc.a src/.libs/libbapparse.a -lz

bin_PROGRAMS = bap_parse_il makeitcrash
bap_parse_il_SOURCES = bap_parse_il.c
bap_parse_il_LDADD = src/libbapparse.la src/libbitfunc.la $(FS_LIBS)

makeitcrash_SOURCES = makeitcrash.c
makeitcrash_LDADD = src/libbapparse.la src/libbitfunc.la $(FS_LIBS)
