#include "funcsat.h"
#include "funcsat/system.h"
#include "funcsat/types.h"
#include "funcsat/internal.h"
#include "funcsat/restart.h"
#include "funcsat/debug.h"
#include "funcsat/vecgen.h"
#include "funcsat/hashtable.h"


/**
 * Parse the dimacs CNF file at path and add the clauses to the given
 * solver. Any parsing errors are sent to stderr, and exit(1) is called.
 */
void parseDimacsCnf(const char *path, funcsat *func);

