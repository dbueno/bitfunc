#ifndef bitfunc_internal_h
#define bitfunc_internal_h

#include <bitfunc.h>

/* ensures the assignment is allocated to the appropriate size for the
 * underlying AIG. */
void bf_prep_assignment(bfman *b);
void bf_clear_assignment(bfman *b);
bool bf_get_assignment_lazily(bfman *b, variable p);


/**
 * Resets the entire state of the Funcsat SAT solver.
 */
void bfResetFuncsat(bfman *m);

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#endif
