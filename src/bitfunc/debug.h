#ifndef bf_debug_included
#define bf_debug_included

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <bitfunc.h>

/**
 *
 * \code
 *   bf_IfDebug(b, "bf", 2) {
 *   ... something that only appears if debugging for label "bf" is at least level 2 ...
 *   }
 * \endcode
 */
#define bf_IfDebug(b, label, level)                                 \
  if ((b)->logSyms &&                                               \
      bfLogMapSearch((b)->logSyms, (void *) (label)) &&             \
      (level) <= *bfLogMapSearch((b)->logSyms, (void *) (label)))

int bf_log(const bfman *, const char *label, int level, const char *format, ...);
int bf_dopen(const bfman *, const char *label);
int bf_dclose(const bfman *, const char *label);

DECLARE_HASHTABLE(bfLogMapInsert, bfLogMapSearch, bfLogMapRemove, char, int)

#endif
