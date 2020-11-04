/*******************************************************************\

Module: Array Internal Support

Author: Timothy Loffredo <tjloffr@sandia.gov> & Denis Bueno <denis.bueno>

@@@todo add concrete store optimisation (concrete addresses)
@@@todo get rid of barray and store hashtable (if applicable) in array

\*******************************************************************/

#ifndef bf_array_h
#define bf_array_h

#include <funcsat/hashtable.h>
#include <stdint.h>

#include "types.h"
#include "bitvec.h"

/* define this constant in order to turn off CEGAR for array congruence clauses. */
#define IMMEDIATE_ARRAY_CONGRUENCE

#ifndef IMMEDIATE_ARRAY_CONGRUENCE
#define CEGAR_ARRAY_CONGRUENCE 
#endif

typedef struct bvpair_t
{
  bitvec     *index;
  uint64_t  value;
} bvpair;

DECLARE_HASHTABLE(readMapInsert, readMapSearch, readMapRemove, bitvec, bitvec)
DECLARE_HASHTABLE(valuesMapInsert, valuesMapSearch, valuesMapRemove, uint64_t, struct bvpair_t)

typedef struct barray
{
  uint32_t idxSize;
  uint32_t eltSize;
  
  /** Map of bitvec (index) to bitvec (fresh bare read bits). Used for each read. */
  struct hashtable *readMap;

  /** Map of uint64_t (concrete index) to bvpair_t (associated bitvec index and
   * value). Used during CEGAR congruence procedure. */
  struct hashtable *valuesMap;
} barray;

/**
 * An array represents a state of memory that has been written to, possible many
 * times. Each ::array is actually just one written index/value pair, with a
 * pointer to the next-most-recent write.
 */
typedef struct array
{
  struct array *next;
  barray *bare;

  /** index of the write */
  bitvec *index;
  /** value of the write */
  bitvec *value;

  struct hashtable *atomic_writes;

  struct hashtable *memo;
} array;


struct barray *newBareArray(uint32_t is, uint32_t es);
array *newArray(uint32_t is, uint32_t es);
void destroyBareArray(struct barray *b);
void destroyArray(array *a, bool destroyBare, bool recurse);
array *newArrayFrom(array *a, bitvec *idx, bitvec *val);
array *writeArrayAtomic(array *a, bitvec *idx, bitvec *val);
bool arrayIsBare(array *a);
bool arrayIsAtomic(array *a);
void bfaPrint(bfman *b, FILE *out, array *a);

#define forArray(prev, curr, arr) \
  for (prev = curr = arr; curr; prev = curr, curr = curr->next)

#endif
