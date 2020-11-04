
#ifndef binvec_h_included
#define binvec_h_included

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "funcsat/memory.h"
#include "funcsat/clause.h"


typedef struct binwatch
{
  /**
   * The literal implied if the watched literal becomes set.
   */
  literal implied;
  clause *clause;             /* can be used for reasons and conflicts */
} binwatch_t;

typedef struct
{
  /**
   * Indexed by literal (the negation of which is) in each clause.
   */
  binwatch_t *data;

  /**
   * Number of valid elements in binvec_t::data.
   */
  size_t size;

  /**
   * Number of allocated elements in binvec_t::data.
   */
  size_t capacity;
} binvec_t;

void binvecInit(binvec_t *v, size_t capacity);
void binvecDestroy(binvec_t *v);
void binvecClear(binvec_t *v);
void binvecPush(binvec_t *v, binwatch_t data);
void binvecPushAt(binvec_t *v, binwatch_t data, size_t i);
binwatch_t binvecPop(binvec_t *v);
binwatch_t binvecPopAt(binvec_t *v, size_t i);
binwatch_t binvecPeek(binvec_t *v);
#define binvecGet(v,i) ((v)->data[i])
void binvecSet(binvec_t *v, size_t i, binwatch_t p);
void binvecCopy(binvec_t *dst, binvec_t *src);




#endif
