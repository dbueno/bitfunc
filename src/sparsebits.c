/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include "bitfunc/sparsebits.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

struct sbs_entry {
  unsigned base;
  uint32_t bits;
};

struct sparse_bitset {
  size_t avail;
  size_t used;
  struct sbs_entry *data;
};

sparse_bitset *sbs_create() {
  sparse_bitset *res = malloc(sizeof(sparse_bitset));
  res->avail = 8;
  res->used = 0;
  res->data = calloc(8,sizeof(struct sbs_entry));
  return res;
}

extern inline unsigned base_for_bit(unsigned i) { return i / 32; }
extern inline uint32_t mask_for_bit(unsigned i) { return 1<<(i % 32); }

void dump_sbs(sparse_bitset *sbs) {
  printf("%p avail: %zd used: %zd\n", sbs, sbs->avail, sbs->used);
  for (size_t n = 0; n < sbs->used; ++n) {
    printf("0x%x = ", sbs->data[n].base*32);
    for (char b = 32; b > 0; --b) {
      if (sbs->data[n].bits & (1<<(b-1))) {
        putchar('1');
      } else {
        putchar('0');
      }
    }
    putchar('\n');
  }
}

bool sbs_test(sparse_bitset *sbs, unsigned i) {
  unsigned b = base_for_bit(i);
  for (size_t n = 0; n < sbs->used; ++n) {
    if (sbs->data[n].base == b) {
      return (sbs->data[n].bits & mask_for_bit(i)) != 0;
    }
  }
  return false;
}

static inline struct sbs_entry *sbs_find_entry(sparse_bitset *sbs, unsigned i) {
  unsigned b = base_for_bit(i);
  for (size_t n = 0; n < sbs->used; ++n) {
    if (sbs->data[n].base == b) {
      return &sbs->data[n];
    }
  }
  if (sbs->avail == 0) {
    sbs->data = realloc(sbs->data, sbs->used*2);
    memset(&sbs->data[sbs->used+1], 0, sbs->used*sizeof(struct sbs_entry));
    assert(sbs->data != NULL);
    sbs->avail = sbs->used;
  }
  sbs->used++;
  sbs->avail--;
  sbs->data[sbs->used-1].base = b;
  return &sbs->data[sbs->used-1];
}

void sbs_set(sparse_bitset *sbs, unsigned i) {
  struct sbs_entry *entry = sbs_find_entry(sbs, i);
  entry->bits |= mask_for_bit(i);
}

void sbs_reset(sparse_bitset *sbs, unsigned i) {
  struct sbs_entry *entry = sbs_find_entry(sbs, i);
  entry->bits &= ~(mask_for_bit(i));
}

void sbs_destroy(sparse_bitset *sbs) {
  free(sbs->data);
  free(sbs);
}
