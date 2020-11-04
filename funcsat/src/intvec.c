/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat/config.h>

#include <assert.h>
#include "funcsat/intvec.h"
#include "funcsat.h"


intvec *intvecAlloc(uintptr_t capacity)
{
  intvec *r;
  CallocX(r, 1, sizeof(*r));
  intvecInit(r, capacity);
  return r;
}

/* Low level intvec manipulations. */

void intvecInit(intvec *v, uintptr_t capacity)
{
  uintptr_t c = capacity > 0 ? capacity : 4;
  CallocX(v->data, c, sizeof(*v->data));
  v->size = 0;
  v->capacity = c;
}

void intvecDestroy(intvec *v)
{
  free(v->data);
  v->data = NULL;
  v->size = 0;
  v->capacity = 0;
}

void intvecClear(intvec *v)
{
  v->size = 0;
}

void intvecPush(intvec *v, int data)
{
  if (v->capacity <= v->size) {
    while (v->capacity <= v->size) {
      v->capacity = v->capacity*2+1;
    }
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  v->data[v->size++] = data;
}

void intvecPushAt(intvec *v, int data, uintptr_t i)
{
  uintptr_t j;
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


void intvecGrowTo(intvec *v, uintptr_t newCapacity)
{
  if (v->capacity < newCapacity) {
    size_t delta = v->capacity*2+1;
    v->capacity = delta > newCapacity ? delta : newCapacity;
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  assert(v->capacity >= newCapacity);
}


int intvecPop(intvec *v)
{
  assert(v->size != 0);
  return v->data[v->size-- - 1];
}

int intvecPopAt(intvec *v, uintptr_t i)
{
  uintptr_t j;
  assert(v->size != 0);
  int res = v->data[i];
  for (j = i; j < v->size-1; j++) {
    v->data[j] = v->data[j+1];
  }
  v->size--;
  return res;
}

int intvecPeek(intvec *v)
{
  assert(v->size != 0);
  return v->data[v->size-1];
}

