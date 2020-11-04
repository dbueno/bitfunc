/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <bitfunc/circuits.h>
#include <bitfunc/array.h>
#include <funcsat/hashtable_itr.h>

DEFINE_HASHTABLE_INSERT(readMapInsert, bitvec, bitvec)
DEFINE_HASHTABLE_SEARCH(readMapSearch, bitvec, bitvec)
DEFINE_HASHTABLE_INSERT(valuesMapInsert, uint64_t, bvpair)
DEFINE_HASHTABLE_SEARCH(valuesMapSearch, uint64_t, bvpair)

extern inline unsigned hashuint64(void *k)
{
  uint64_t u = *(uint64_t *) k;
  unsigned int hash = 0;
  unsigned int b = 378551, a = 63689;
  while (u > 0) {
    uint8_t byte = u & (uint8_t) 0xff;
    hash = hash * a + byte;
    a = a * b;
    u >>= 8;
  }
  return hash;
}

extern inline int equaluint64(void *k1, void *k2)
{
  uint64_t x = *(uint64_t *)k1, y = *(uint64_t *)k2;
  return x == y;
}

struct barray *newBareArray(uint32_t is, uint32_t es)
{
  struct barray *r;
  CallocX(r, 1, sizeof(*r));
  r->idxSize = is;
  r->eltSize = es;
  r->readMap = create_hashtable(16, hashBv, bvEqual);
  r->valuesMap = create_hashtable(16, hashuint64, equaluint64);
  return r;
}

struct array *newArray(uint32_t is, uint32_t es)
{
  struct array *r;
  CallocX(r, 1, sizeof(*r));
  r->bare = newBareArray(is, es);
  /* r->next = NULL; */
  return r;
}

struct array *newArrayFrom(struct array *a, bitvec *idx, bitvec *elt)
{
  struct array *r;
  CallocX(r, 1, sizeof(*r));
  r->bare = a->bare;
  r->next = a;
  r->index = idx;
  r->value = elt;
  r->memo = create_hashtable(128, hashBv, bvEqual);
  assert(idx->size == r->bare->idxSize);
  assert(elt->size == r->bare->eltSize);
  return r;
}

struct array *writeArrayAtomic(struct array *a, bitvec *idx, bitvec *val)
{
  struct array *aa;
  if (!a->atomic_writes) {
    CallocX(aa, 1, sizeof(*aa));
    aa->bare = a->bare;
    aa->next = a;
    aa->memo = create_hashtable(128, hashBv, bvEqual);
    aa->atomic_writes = create_hashtable(8, hashBv, bvEqual);
  } else {
    aa = a;
  }
  if ((aa != a) || !hashtable_search(aa->atomic_writes, idx)) {
    hashtable_insert(aa->atomic_writes, idx, val);
  }
  return aa;
}

void destroyBareArray(struct barray *b)
{
  hashtable_destroy(b->readMap, false, false);
  hashtable_destroy(b->valuesMap, true, true);
  free(b);
}
void destroyArray(struct array *a, bool destroyBare, bool recurse)
{
  barray *bare = a->bare;
  if (recurse && a->next) destroyArray(a->next, false, recurse);
  if (a->memo) hashtable_destroy(a->memo, false, false);
  free(a);
  if (destroyBare) destroyBareArray(bare);
}



bool arrayIsBare(array *a)
{
  return !a->next;
}

bool arrayIsAtomic(array *a)
{
  return a->atomic_writes;
}

void bfaPrint(bfman *b, FILE *out, array *a)
{
  array *ret = NULL;
  if (arrayIsBare(a)) {
    ret = newArray(a->bare->idxSize, a->bare->eltSize);
    barray *rbare = ret->bare;
    if (hashtable_count(a->bare->readMap)) {
      struct hashtable_itr *it = hashtable_iterator(a->bare->readMap);
      do {
        bitvec *readIndex = hashtable_iterator_key(it);
        bitvec *readValue = hashtable_iterator_value(it);
        if (bfvIsConstant(readIndex)) {
          bfvPrintConcrete(b, stderr, readIndex, PRINT_HEX);
        } else {
          bfvPrint(stderr, readIndex);
        }
        fprintf(stderr, ": ");
        if (bfvIsConstant(readValue)) {
          bfvPrintConcrete(b, stderr, readValue, PRINT_HEX);
        } else {
          bfvPrint(stderr, readValue);
        }
        fprintf(stderr, "\n");
      } while (hashtable_iterator_advance(it));
      free(it);
    }
  } else {
    bitvec *readIndex = a->index;
    bitvec *readValue = a->value;
    if (bfvIsConstant(readIndex)) {
      bfvPrintConcrete(b, stderr, readIndex, PRINT_HEX);
    } else {
      bfvPrint(stderr, readIndex);
    }
    fprintf(stderr, ": ");
    if (bfvIsConstant(readValue)) {
      bfvPrintConcrete(b, stderr, readValue, PRINT_HEX);
    } else {
      bfvPrint(stderr, readValue);
    }
    fprintf(stderr, "\n");
    bfaPrint(b, out, a->next);
  }
}
