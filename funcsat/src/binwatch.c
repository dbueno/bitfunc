/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat/config.h>

#include "funcsat/binwatch.h"

/* NewVec(vecp, binwatch_t); */
/* NewVec(vecc, char); */
/* NewVec(veci, int64_t); */


void binvecInit(binvec_t *v, size_t capacity)
{
  size_t c = capacity > 0 ? capacity : 4;
  CallocX(v->data, c, sizeof(*v->data));
  v->size = 0;
  v->capacity = c;
}

void binvecDestroy(binvec_t *v)
{
  free(v->data);
  v->data = NULL;
  v->size = 0;
  v->capacity = 0;
}

void binvecClear(binvec_t *v)
{
  v->size = 0;
}

void binvecPush(binvec_t *v, binwatch_t data)
{
  if (v->capacity <= v->size) {
    while (v->capacity <= v->size) {
      v->capacity = v->capacity*2+1;
    }
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  v->data[v->size++] = data;
}

void binvecPushAt(binvec_t *v, binwatch_t data, size_t i)
{
  size_t j;
  assert(i <= v->size);
  if (v->capacity <= v->size) {
    while (v->capacity <= v->size) {
      v->capacity = v->capacity*2+1;
    }
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  v->size++;
  for (j = v->size-1; j > i; j--) {
    v->data[j] = v->data[j-1];
  }
  v->data[i] = data;
}


binwatch_t binvecPop(binvec_t *v)
{
  assert(v->size != 0);
  return v->data[v->size-- - 1];
}

binwatch_t binvecPopAt(binvec_t *v, size_t i)
{
  size_t j;
  assert(v->size != 0);
  binwatch_t res = v->data[i];
  for (j = i; j < v->size-1; j++) {
    v->data[j] = v->data[j+1];
  }
  v->size--;
  return res;
}

binwatch_t binvecPeek(binvec_t *v)
{
  assert(v->size != 0);
  return v->data[v->size-1];
}

void binvecSet(binvec_t *v, size_t i, binwatch_t p)
{
  v->data[i] = p;
}

void binvecCopy(binvec_t *dst, binvec_t *src)
{
  uint64_t i;
  for (i = 0; i < src->size; i++) {
    binvecPush(dst, src->data[i]);
  }
}







