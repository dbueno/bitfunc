#ifndef bf_memory_h
#define bf_memory_h

#include <funcsat/hashtable.h>
#include <stdint.h>

#include "types.h"
#include "array.h"
#include "bitvec.h"

/** Address value pair. */
typedef struct memoryCell
{
  bitvec *index;
  bitvec *value;
} memoryCell;

/** Represents a contiguous region of symbolic memory.

    The lifecycle of a memory:

    - creation: ::bfmInit
    - loading and storing: ::bfmLoad, ::bfmStore, and variants
    - muxing: ::bfmSelect
    - destruction: ::bfmDestroy

    When a memory is created, it is placed on a ::bfman-wide list of all
memories. Its presence in this list means it is referenced; in order to remove
the reference to it, the client must call ::bfmDestroy on the memory. The same
reference occurs when a memory is created using ::bfmCopy.
*/
typedef struct memory
{
  unsigned memflag;
  struct memory *flagged_mem;
  bitvec *memvalue;

  /** writes after condition ::c */
  vector *arr; //a vector of memoryCells
  uint8_t idxsize;
  uint8_t eltsize;

  /** condition */
  literal   c;
  /** memory if condition is true */
  struct memory *memt;

  /** memory if condition is false */
  struct memory *memf;
} memory;

#endif

/** The size of address stored in the memory. */
unsigned bfmIdxSize(memory *m);
/** The size of elements stored in the memory. */
unsigned bfmEltSize(memory *m);

/**
 * Creates a RAM indexed by and containing bit vectors.
 *  
 * @param idxsize the number of bits per address
 * @param eltsize the number of bits per element
 */
memory *bfmInit(bfman *b, uint8_t idxsize, uint8_t eltsize);

/**
 * Store a native-size bit vector at an address. Side-effects the given memory.
 *
 * @pre addr must be the size of ::bfmIdxSize(m)
 * @pre val must be the size of ::bfmEltSize
 */
void bfmStore(bfman *b, memory *m, bitvec *addr, bitvec *val);
void bfmStore_le(bfman *b, memory *m, bitvec *address_start, bitvec *val);
void bfmStore_be(bfman *b, memory *m, bitvec *address_start, bitvec *val);
void bfmStoreVector_le(bfman *b, memory *m, bitvec *address_start, vector *values);
void bfmStoreVector_be(bfman *b, memory *m, bitvec *address_start, vector *values);

/**
 * Load a native-size bit vector from an address.
 *
 * @pre addr must be the size of ::bfmIdxSize(m)
 * @post the returned bit vector is the size of ::bfmEltSize(m)
 */
bitvec *bfmLoad(bfman *b, memory *m, bitvec *addr);
/**
 * Load and check for a read-before-write condition. Given the address of a
 * literal, upon return that literal will be true iff this load read a location
 * that was never written.
 *
 */
bitvec *bfmLoad_RBW(bfman *b, memory *m, bitvec *addr, literal *rbw);
/** Symbolically addressed load of a bitvec (little endian) */
bitvec *bfmLoad_le(bfman *b, memory *m, bitvec *address_start, unsigned numElts);
/** Symbolically addressed load of a bitvec (big endian) */
bitvec *bfmLoad_be(bfman *b, memory *m, bitvec *address_start, unsigned numElts);
bitvec *bfmLoad_le_RBW(bfman *b, memory *m, bitvec *address_start, unsigned numElts, literal *rbw);
bitvec *bfmLoad_be_RBW(bfman *b, memory *m, bitvec *address_start, unsigned numElts, literal *rbw);
vector *bfmLoadVector_le(bfman *b, memory *m, bitvec *address_start, unsigned numVecs, unsigned vecSize);
vector *bfmLoadVector_be(bfman *b, memory *m, bitvec *address_start, unsigned numVecs, unsigned vecSize);

/**
 * Performs a shallow copy of the given memory.
 *
 * If n = bfmCopy(b, m), then operations on n do not affect the contents of
 * m. Reads of n will function as normal, except that they will read from m
 * afterward.
 */
memory *bfmCopy(bfman *b, memory *m);

/**
 * Muxes two memories together based on a condition. Evaluates to t if the
 * condition holds, and f otherwise.
 */
memory *bfmSelect(bfman *b, literal c, memory *t, memory *f);

/**
 * Produces a concrete memory from the current solution
 */
memory *bfmFromCounterExample(bfman *b, memory *m);

/**
 * Print the contents of a memory.
 */
void bfmPrint(bfman *b, FILE *out, memory *m);

/**
 * Reclaims the space used by the given memory and any memories dependent on it,
 * so long as all the memories involved are not referenced by other active
 * memories.
 */
void bfmDestroy(bfman *b, memory *m);

/**
 * Normally, a client won't call this. The client should call ::bfmDestroy when
 * it is known that the given memory is not referenced by any other memory and
 * should be reclaimed.
 */
void bfmUnsafeFree(memory *m);
