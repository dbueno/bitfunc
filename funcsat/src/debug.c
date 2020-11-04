/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <funcsat/debug.h>
#include <funcsat/hashtable.h>

DEFINE_HASHTABLE(fsLogMapInsert, fsLogMapSearch, fsLogMapRemove, char, int)

#ifdef FUNCSAT_LOG



int DebugSolverLoop = 0;        /* index into DebugCat */
int DebugCatLength = 12;
char *DebugCat[12] =
{
  "findUips",
  "bcp",
  "minimiseUip",
  "subsumption",
  "propfact",
  "jail",
  "decide",
  "decideActivity",
  "solve",
  "countsol",
  "sweep",
  "iddqd",
};


/* If FUNCSAT_LOG is not defined, the debugging will be enabled */

/**
 * Each message has a LABEL and LEVEL. The solver has a set of (LABEL,LEVEL)
 * pairs.  For each message, there is a runtime check to see if the label is in
 * the set of pairs; if so and the level in the pair is greater-than-or-equal-to
 * the level of the message, then the message is displayed.
 *
 * @param f, label, msgLevel, format
 * @param newline prints a newline after the msg iff this is true
 *
 * @return like printf, the number of characters printed
 */
int dmsg(const funcsat *f, const char *label, int msgLevel, bool newline, const char *format, ...)
{
  int pr = 0;
  int *logLevel;
  va_list ap;

  if (f->conf->logSyms && (logLevel = hashtable_search(f->conf->logSyms, (void *) label))) {
    if (msgLevel <= *logLevel) {
      unsigned indent = posvecPeek(&f->conf->logStack), i;
      for (i = 0; i < indent; i++) fprintf(f->conf->debugStream, " ");
      if (f->conf->printLogLabel) pr += fprintf(f->conf->debugStream, "%s[%d]: ", label, msgLevel);
      va_start(ap, format);
      pr += vfprintf(f->conf->debugStream, format, ap);
      va_end(ap);
      if (newline) pr += fprintf(f->conf->debugStream, "\n");
    }
  }
  return pr;
}
int dmsg2(const funcsat *f, const char *label, int msgLevel, const char *format, ...)
{
  int pr = 0;
  int *logLevel;
  va_list ap;

  if (f->conf->logSyms && (logLevel = hashtable_search(f->conf->logSyms, (void *) label))) {
    if (msgLevel <= *logLevel) {
      unsigned indent = posvecPeek(&f->conf->logStack), i;
      for (i = 0; i < indent; i++) fprintf(f->conf->debugStream, " ");
      if (f->conf->printLogLabel) pr += fprintf(f->conf->debugStream, "%s[%d]: ", label, msgLevel);
      va_start(ap, format);
      pr += vfprintf(f->conf->debugStream, format, ap);
      va_end(ap);
    }
  }
  return pr;
}



/* special messages push on an indentation stack -- they are messages that
 * "begin" a parenthesis around a part of the system.  other special messages
 * pop from that stack when they are displayed.  they "end" the parenthesis.
 * every message is displayed according to the current stack width, so that
 * messages are indented according to how much nesting is going on. */


int dopen(const funcsat *f, const char *label)
{
  if (f->conf->logSyms && hashtable_search(f->conf->logSyms, (void *) label)) {
    unsigned indent = posvecPeek(&f->conf->logStack)+2;
    posvecPush(&f->conf->logStack, indent);
  }
  return 0;
}
int dclose(const funcsat *f, const char *label)
{
  if (f->conf->logSyms && hashtable_search(f->conf->logSyms, (void *) label)) {
    posvecPop(&f->conf->logStack);
  }
  return 0;
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

#else  /* ifndef FUNCSAT_LOG */

int dmsg(const funcsat *f, const char *label, int level, bool newline, const char *format, ...)
{
  return 0;
}
int dopen(const funcsat *f, const char *label)
{
  return 0;
}
int dclose(const funcsat *f, const char *label)
{
  return 0;
}

#endif
