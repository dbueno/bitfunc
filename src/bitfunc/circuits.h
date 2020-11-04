
#ifndef bf_circuits_h
#define bf_circuits_h

// decision procedure wrapper for boolean propositional logics

#include "types.h"
#include "bitvec.h"

/**
 * Creates a fresh boolean variable. See also cnft::bfvInit and
 * cnft::bfmInit.
 *
 * Adds this as an input to the underlying AIG.
 */
literal bfNewVar(bfman *m);

/**
 * Associate a name with the given literal. The given string is duplicated
 * and stored, so the caller should manage the argument.
 */
literal bfSetName(bfman *b, literal l, const char *name);
char *bfGetName(bfman *b, literal l);

/**
 * @return a literal constrained to be the logical AND of a and b
 */
literal bfNewAnd(bfman *m, literal a, literal b);
literal bfNewOr(bfman *m, literal a, literal b);
literal bfNewXor(bfman *m, literal a, literal b);
literal bfNewNand(bfman *m, literal a, literal b);
literal bfNewNor(bfman *m, literal a, literal b);
literal bfNewEqual(bfman *m, literal a, literal b);
literal bfNewImplies(bfman *m, literal a, literal b);
literal bfNewIff(bfman *m, literal a, literal b);

/**
 * Constrains o to be the logical AND of a and b.
 */
void bfAnd(bfman *m, literal a, literal b, literal o);
void bfOr(bfman *m, literal a, literal b, literal o);
void bfXor(bfman *m, literal a, literal b, literal o);
void bfNand(bfman *m, literal a, literal b, literal o);
void bfNor(bfman *m, literal a, literal b, literal o);
void bfImplies(bfman *m, literal a, literal b, literal o);
void bfEqual(bfman *m, literal a, literal b, literal o);

/**
 * @return a literal which equals the conjunction of all the bits in bv
 */
literal bfBigAnd(bfman *m, bitvec *bv);
literal bfBigOr(bfman *m, bitvec *bv);
literal bfBigXor(bfman *m, bitvec *bv);
literal bfNewSelect(bfman *m, literal a, literal b, literal c); // a?b:c

///
/// Requires the literals to equal each other.
void bfSetEqual(bfman *m, literal a, literal b);

void printAiger(bfman *, char *, bitvec *);
void printAiger_allinputs(bfman *m);
void printFalseAig(bfman *m);

#endif
