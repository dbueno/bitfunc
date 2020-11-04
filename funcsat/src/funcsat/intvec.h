#ifndef intvec_h_included
#define intvec_h_included

#include <limits.h>

#include "system.h"


/** A intvec is a vector of int's. */
typedef struct intvec
{
  int *data;
  uintptr_t  size;
  uintptr_t  capacity;
} intvec;

/**
 * Iterate over a ::intvec.
 */
#define forIntVec(iter, vec) \
  for (iter = (vec)->data; iter != (vec)->data + (vec)->size; iter++)

/**
 * Allocate an empty intvec with the given initial capacity.
 */
intvec *intvecAlloc(uintptr_t capacity);

/**
 * Initializes a new intvec.
 */
void intvecInit(intvec *v, uintptr_t capacity);
/**
 * Frees the underlying literal buffer and resets all the intvec metadata.
 */
void intvecDestroy(intvec *);
void intvecClear(intvec *v);
void intvecPush(intvec *v, int data);
void intvecPushAt(intvec *v, int data, uintptr_t i);
void intvecGrowTo(intvec *v, uintptr_t newCapacity);
int intvecPop(intvec *v);
int intvecPopAt(intvec *v, uintptr_t i);
int intvecPeek(intvec *v);

#endif
