/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat/config.h>

#include <inttypes.h>
#include <assert.h>
#include "funcsat/vec.h"
#include "funcsat.h"

/* NewVec(vecp, void *); */
/* NewVec(vecc, char); */
/* NewVec(veci, int64_t); */

vector *vectorAlloc(unsigned capacity)
{
  vector *ret = malloc(sizeof(*ret));
  vectorInit(ret, capacity);
  return ret;
}

void vectorInit(vector *v, unsigned capacity)
{
  unsigned c = capacity > 0 ? capacity : 4;
  CallocX(v->data, c, sizeof(*v->data));
  v->size = 0;
  v->capacity = c;
}

void vectorDestroy(vector *v)
{
  free(v->data);
  v->data = NULL;
  v->size = 0;
  v->capacity = 0;
}

void vectorFree(vector *v)
{
  vectorDestroy(v);
  free(v);
}

void vectorClear(vector *v)
{
  v->size = 0;
}

void vectorPush(vector *v, void *data)
{
  vectorGrowTo(v, v->size+1);
  v->data[v->size++] = data;
}

void vectorPushAt(vector *v, void *data, unsigned i)
{
  unsigned j;
  assert(i <= v->size);
  vectorGrowTo(v, v->size+1);
  v->size++;
  for (j = v->size-1; j > i; j--) {
    v->data[j] = v->data[j-1];
  }
  v->data[i] = data;
}


void vectorGrowTo(vector *v, unsigned newCapacity)
{
  if (v->capacity < newCapacity) {
    unsigned delta = v->capacity*2+1;
    v->capacity = delta > newCapacity ? delta : newCapacity;
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  assert(v->capacity >= newCapacity);
}


void *vectorPop(vector *v)
{
  assert(v->size != 0);
  return v->data[v->size-- - 1];
}

void *vectorPopAt(vector *v, unsigned i)
{
  unsigned j;
  assert(v->size != 0);
  void *res = v->data[i];
  for (j = i; j < v->size-1; j++) {
    v->data[j] = v->data[j+1];
  }
  v->size--;
  return res;
}

void *vectorPeek(vector *v)
{
  assert(v->size != 0);
  return v->data[v->size-1];
}

void vectorSet(vector *v, unsigned i, void *p)
{
  v->data[i] = p;
}

void vectorCopy(vector *dst, vector *src)
{
  uint64_t i;
  for (i = 0; i < src->size; i++) {
    vectorPush(dst, src->data[i]);
  }
}







