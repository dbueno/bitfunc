#include <assert.h>
#include <bitfunc.h>
#include <bitfunc/array.h>
#include <bitfunc/aiger.h>
#include <bitfunc/circuits.h>
#include <bitfunc/circuits_internal.h>
#include <funcsat.h>
#include <funcsat/hashtable.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct mitre
{
  /** This is required to be false. Clients should not negate it. */
  literal output;
  /** Name of the test. */
  const char *name;
  unsigned width;
  bfman *man;

  vector inputNames;
  vector inputs;
} mitre;


/**
 * Create a new ::mitre object for doing equivalence-based testing.
 */
mitre *mitre_init(const char *name, unsigned width);

/**
 * Adds an input ::bitvec to the mitre. The given bitvec is ::bfvHold'd by the
 * mitre.
 */
bitvec *mitre_addInput(mitre *m, const char *name, bitvec *b);

/**
 * Add an input literal to the mitre.
 */
literal mitre_addInputLit(mitre *m, const char *name, literal l);


/**
 * Ensure the mitre'd circuit is unsatisfiable.
 *
 * In case it is satisfiable (failure), each input bit is printed to the screen.
 */
int mitre_run(mitre *m);

/**
 * Release and consume all the inputs and destroy the mitre.
 */
void mitre_destroy(mitre *m);

