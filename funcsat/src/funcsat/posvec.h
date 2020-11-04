#ifndef posvec_h_included
#define posvec_h_included

#include <limits.h>

#include "system.h"


/** A posvec is a vector of unsigned's. */
typedef struct posvec
{
  unsigned *data;
  uintptr_t  size;
  uintptr_t  capacity;
} posvec;

/**
 * Iterate over a ::posvec.
 */
#define forPosVec(iter, vec) \
  for (iter = (vec)->data; iter != (vec)->data + (vec)->size; iter++)

/**
 * Allocate an empty posvec with the given initial capacity.
 */
posvec *posvecAlloc(uintptr_t capacity);

/**
 * Initializes a new posvec.
 */
void posvecInit(posvec *v, uintptr_t capacity);
/**
 * Frees the underlying literal buffer and resets all the posvec metadata.
 */
void posvecDestroy(posvec *);
void posvecClear(posvec *v);
void posvecPush(posvec *v, unsigned data);
void posvecPushAt(posvec *v, unsigned data, uintptr_t i);
void posvecGrowTo(posvec *v, uintptr_t newCapacity);
unsigned posvecPop(posvec *v);
unsigned posvecPopAt(posvec *v, uintptr_t i);
unsigned posvecPeek(posvec *v);

#endif
