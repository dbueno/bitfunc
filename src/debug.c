/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <bitfunc/debug.h>
#include <funcsat/hashtable.h>
#include <funcsat/posvec.h>

DEFINE_HASHTABLE(bfLogMapInsert, bfLogMapSearch, bfLogMapRemove, char, int)

int bf_log(const bfman *b, const char *label, int msgLevel, const char *format, ...)
{
  int pr = 0;
  int *logLevel;
  va_list ap;

  if (b->logSyms && (logLevel = bfLogMapSearch(b->logSyms, (void *) label))) {
    if (msgLevel <= *logLevel) {
      unsigned indent = posvecPeek((posvec *)&b->logStack), i;
      for (i = 0; i < indent; i++) fprintf(b->dout, " ");
      if (true) pr += fprintf(b->dout, "%s[%d]: ", label, msgLevel);
      va_start(ap, format);
      pr += vfprintf(b->dout, format, ap);
      va_end(ap);
    }
  }
  return pr;
}




int bf_dopen(const bfman *b, const char *label)
{
  if (b->logSyms && bfLogMapSearch(b->logSyms, (void *) label)) {
    unsigned indent = posvecPeek((posvec *)&b->logStack)+2;
    posvecPush((posvec *)&b->logStack, indent);
  }
  return 0;
}
int bf_dclose(const bfman *b, const char *label)
{
  if (b->logSyms && bfLogMapSearch(b->logSyms, (void *) label)) {
    posvecPop((posvec *)&b->logStack);
  }
  return 0;
}

bool bfEnableDebug(bfman *b, char *label, int maxlevel)
{
  int *currMaxLevel;
  bool ret = true;
  if (NULL != (currMaxLevel = bfLogMapSearch(b->logSyms, (void *) label))) {
    bfDisableDebug(b, label);
    ret = false;
  }
  int *l = malloc(sizeof(*l));
  *l = maxlevel;
  bfLogMapInsert(b->logSyms, strdup(label), l);
  return ret;
}

bool bfDisableDebug(bfman *b, char *label)
{
  int *lev = bfLogMapRemove(b->logSyms, (void *) label);
  if (lev) {
    free(lev);
    return true;
  }
  return false;
}


#if 0
char *
make_message(const char *fmt, ...)
{
  /* Guess we need no more than 100 bytes. */
  int n, size = 100;
  char *p, *np;
  va_list ap;

  if ((p = malloc(size)) == NULL)
    return NULL;

  while (1) {
    /* Try to print in the allocated space. */
    va_start(ap, fmt);
    n = vsnprintf(p, size, fmt, ap);
    va_end(ap);
    /* If that worked, return the string. */
    if (n > -1 && n < size)
      return p;
    /* Else try again with more space. */
    if (n > -1)    /* glibc 2.1 */
      size = n+1; /* precisely what is needed */
    else           /* glibc 2.0 */
      size *= 2;  /* twice the old size */
    if ((np = realloc (p, size)) == NULL) {
      free(p);
      return NULL;
    } else {
      p = np;
    }
  }
}
#endif

