#ifndef debug_h_included
#define debug_h_included

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>

#include "funcsat/system.h"
#include "funcsat/types.h"

/* uint64_t funcsatDebugSet; in main.c*/

/* Descriptions for these options can be found in debugDescriptions[Option] */

#ifndef NDEBUG
/* used to change an assertion error to a particular exit code so you can easily
 * use cnfdd. */
#define assertExit(code, cond) \
  if (!(cond)) exit(code);
#else
#define assertExit(code, cond)
#endif

#ifdef FUNCSAT_LOG

#define dbgout(f) ((f)->conf->debugStream)

/** This is the old version. Use IfDebug now! */
#define Debug(f, label, level, newline, body)                           \
  do {                                                                  \
    int *_Debug_msgLevel;                                               \
    if (f->conf->logSyms &&                                             \
        (_Debug_msgLevel = fsLogMapSearch(f->conf->logSyms, (void *) label))) { \
      if (level <= *_Debug_msgLevel) {                                  \
        body;                                                           \
        if (newline) fprintf(f->conf->debugStream, "\n");               \
      }                                                                 \
    }                                                                   \
  } while (0);

#define fs_IfDebug(f, label, level)                                        \
  if ((f)->conf->logSyms &&                                             \
      fsLogMapSearch(f->conf->logSyms, (void *) (label)) &&             \
      (level) <= *fsLogMapSearch((f)->conf->logSyms, (void *) (label)))

#else

#define Debug(f, label, level, newline, body) do {} while(0);
#define fs_IfDebug(f, label, level) if (false)

#endif

/**
 * Log a debugging message.
 *
 * There are levels.  What do they mean?
 */
int dmsg(const funcsat *, const char *label, int level, bool newline, const char *format, ...);
int dmsg2(const funcsat *, const char *label, int level, const char *format, ...);

int dopen(const funcsat *, const char *label);
int dclose(const funcsat *, const char *label);

#endif
