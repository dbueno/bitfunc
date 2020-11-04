AM_CFLAGS = $(CODEMODEL)
AM_CPPFLAGS =
AM_LDFLAGS = $(CODEMODEL)

nobase_include_HEADERS = funcsat/config.h \
  funcsat/hashtable_private.h funcsat/hashtable.h funcsat/hashtable_utility.h funcsat/hashtable_itr.h funcsat/vec.h funcsat/vecgen.h funcsat/vecgen.h.in \
  funcsat/posvec.h funcsat/intvec.h \
  funcsat/fibheap.h funcsat/fibheap_internal.h \
  funcsat/system.h

include_HEADERS = funcsat.h funcsat_internal.h


# funcsat/types.h funcsat/system.h funcsat/clause.h 
# funcsat/memory.h 
# funcsat/vec.h funcsat/vecgen.h funcsat/clausevec.h 
# funcsat/list.h 
# funcsat/fifoq.h 
# funcsat/fibheap.h funcsat/fibheap_internal.h 
# funcsat/debug.h 
# funcsat/posvec.h funcsat/intvec.h 
# funcsat/binwatch.h 
# funcsat/internal.h 
# funcsat/proof.h 
# learning.h 
# funcsat/restart.h 
# funcsat/frontend.h 
# funcsat/backbone.h


noinst_LTLIBRARIES = libfuncsat.la
libfuncsat_la_SOURCES = \
  vec.c hashtable.c hashtable_utility.c hashtable_itr.c fifoq.c fibheap.c posvec.c intvec.c \
  funcsat.c funcsat/fifoq.h \
  funcsat/vec_reason_info.h funcsat/vec_uintptr.h

vec.c: funcsat.h
fifoq.c: funcsat.h
fibheap.c: funcsat.h
posvec.c: funcsat.h
intvec.c: funcsat.h
funcsat/vec_reason_info.h: funcsat/vecgen.h.in
	sed 's/@SHORTNAME@/reason_info/g' funcsat/vecgen.h.in | sed 's/@TYPE@/struct reason_info/g' >$@
funcsat/vec_uintptr.h: funcsat/vecgen.h.in
	sed 's/@SHORTNAME@/uintptr/g' funcsat/vecgen.h.in | sed 's/@TYPE@/uintptr_t/g' >$@

if HAVE_PDFTEX
dist_doc_DATA = funcsat.pdf
endif

# learning.c funcsat/learning.h: learning.w
# 	$(CTANGLE) $^ - learning.c
# 	mv learning.h funcsat/
# learning.h: learning.c


EXTRA_DIST = funcsat.w

funcsat.w: funcsat/vec_reason_info.h funcsat/vec_uintptr.h
funcsat.c funcsat.h funcsat_internal.h: funcsat.w
	$(CTANGLE) $< - $@
funcsat.h: funcsat.c
funcsat_internal.h: funcsat.h

# %.c: %.w
# 	$(CTANGLE) $< - $@
%.tex: %.w
	$(CWEAVE) $< - $@
%.pdf: %.tex
	$(PDFTEX) -file-line-error $<
	$(PDFTEX) -file-line-error $<
	$(PDFTEX) -file-line-error $<


