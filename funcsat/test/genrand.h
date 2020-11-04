#ifndef genrand_h_included
#define genrand_h_included

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "funcsat.h"

long int random(void);
void srandom(unsigned int seed);

/**
 * A random boolean value.
 */
bool mumble();

/**
 * Generates a random number no more than ub and greater than 0.
 */
long int randomPosNum(uint32_t ub);
literal randomLit(unsigned maxVar);
/**
 * Generates a double between 0.0 and 1.0 (inclusive).
 */
double randomDouble();
void genClause(clause *c, uint32_t maxLen, unsigned maxVar);

#endif
