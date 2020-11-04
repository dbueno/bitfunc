#ifndef restart_h_included
#define restart_h_included

#include "types.h"

/* each one, given the current state, returns whether it's time to restart. */

bool funcsatLubyRestart(funcsat *f, void *p);
bool funcsatNoRestart(funcsat *, void *);
bool funcsatInnerOuter(funcsat *f, void *p);
bool funcsatMinisatRestart(funcsat *f, void *p);

#endif
