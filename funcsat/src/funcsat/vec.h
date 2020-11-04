
#ifndef vec_h_included
#define vec_h_included

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>


typedef struct
{
  void **data;

  /**
   * Number of pointers stored.
   */
  unsigned size;

  /**
   * Number of pointers allocated.
   */
  unsigned capacity;
} vector;

/* iterators and stuff */
#define vectorBegin(ty, vec) (ty)(vec)->data
#define vectorEnd(ty, vec) (ty)((vec)->data + (vec)->size)
#define vectorBeginEnd(b,e, ty, vec) b = (ty)(vec)->data, e = (ty)((vec)->data + (vec)->size)

/**
 * Iterate over a vector.
 *
 * In this example the vector points at bv_t*.
 *
 *   bv_t **elt;
 *   forVector(bv_t **, elt, vec) {
 *     ... do something with *elt ...
 *   }
 */
#define forVector(ty, elt, vec)                                         \
  for (elt = (ty) (vec)->data; elt != (ty) ((vec)->data + (vec)->size); elt++)

#define forVectorRev(ty, elt, vec)                                         \
  for (elt = (ty) ((vec)->data + (vec)->size - 1); elt != (ty) ((vec)->data - 1); elt--)

/**
 * Allocate a vector and its data.
 *
 * Note that this takes two malloc calls. If the calling context can support it,
 * consider putting the vector on the stack and using ::vectorInit and
 * ::vectorDestroy.
 */
vector *vectorAlloc(unsigned capacity);
void vectorInit(vector *v, unsigned capacity);
/**
 * The tear-down function for ::vectorInit.
 */
void vectorDestroy(vector *v);
/**
 * The tear-down function for ::vectorAlloc.
 */
void vectorFree(vector *v);
void vectorClear(vector *v);
void vectorPush(vector *v, void *data);
void vectorPushAt(vector *v, void *data, unsigned i);
void vectorGrowTo(vector *v, unsigned newCapacity);
void *vectorPop(vector *v);
void *vectorPopAt(vector *v, unsigned i);
void *vectorPeek(vector *v);
#define vectorGet(v,i) ((v)->data[i])
void vectorSet(vector *v, unsigned i, void *p);
void vectorCopy(vector *dst, vector *src);




#endif
