AM_CFLAGS = $(CODEMODEL)
AM_LDFLAGS = $(CODEMODEL)
AM_CPPFLAGS = -I$(srcdir)/../src -I$(srcdir)

SHTESTS = test-countsol.sh test-countsol2.sh \
  test-assume.sh test-assume2.sh test-assume3.sh

noinst_PROGRAMS = test-fibheap
TESTS = $(noinst_PROGRAMS) $(SHTESTS)

EXTRA_DIST = $(SHTESTS) trans.cnf.gz

test_fibheap_SOURCES = test-fibheap.c gen.c genrand.h
test_fibheap_LDADD = ../src/libfuncsat.la

# test_incremental_SOURCES = test-incremental.c gen.c
# test_incremental_LDADD = ../src/libfuncsat.la
