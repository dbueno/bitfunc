/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include <bitfunc.h>
#include <bitfunc/circuits.h>
#include <bitfunc/machine_state.h>

#include "bitfunc/bitfunc_internal.h"

static machine_state *bfmsInitNoRam(bfman *b, unsigned numregs, uint8_t eltsize)
{
  machine_state *s;
  CallocX(s, 1, sizeof(*s));
  s->eltsize = eltsize;
  MallocX(s->reg, sizeof(*s->reg), numregs);
  s->numregs = numregs;
  for (unsigned i = 0; i < s->numregs; i++) {
    s->reg[i] = bfvHold(bfvUconstant(b, eltsize, 0));
  }
  s->ram = NULL;
  s->isValid = LITERAL_TRUE;
  //vectorPush(&b->machineStates, s);
#ifndef NDEBUG
  bfmsCheck(b, s);
#endif
  return s;
}


machine_state *bfmsInit(bfman *b, unsigned numregs, uint8_t idxsize, uint8_t eltsize)
{
  machine_state *s;
  CallocX(s, 1, sizeof(*s));
  s->eltsize = eltsize;
  MallocX(s->reg, sizeof(*s->reg), numregs);
  s->numregs = numregs;
  for (unsigned i = 0; i < s->numregs; i++) {
    s->reg[i] = bfvHold(bfvUconstant(b, eltsize, 0));
  }
  s->ram = bfmInit(b, idxsize, eltsize);
  s->isValid = LITERAL_TRUE;
  //vectorPush(&b->machineStates, s);
#ifndef NDEBUG
  bfmsCheck(b, s);
#endif
  return s;
}



void bfmsDestroy(bfman *b, machine_state *s)
{
  if(s == NULL) return;
  for(unsigned i = 0; i < s->numregs; i++) {
    if(s->reg[i]) {
      assert(s->reg[i]->numHolds == 1);
      CONSUME_BITVEC(bfvRelease(s->reg[i]));
    }
  }
  free(s->reg);

  bfmDestroy(b, s->ram);
  free(s);
}

void bfmsCheck(bfman *UNUSED(b), machine_state *MS)
{
  for (unsigned i = 0; i < MS->numregs; i++) {
    assert(MS->reg[i]->numHolds==1);
    assert(MS->reg[i]->size == MS->eltsize);
  }
}

void bfmsPrint(bfman *b, FILE *stream, machine_state *m)
{
  for (unsigned r = 0; r < m->numregs; r++) {
    if (bfvIsConstant(m->reg[r])) bfvPrintConcrete(b, stream, m->reg[r], PRINT_HEX);
    else bfvPrint(stream, m->reg[r]);
    fprintf(stream, " ");
  }
  fprintf(stream, "\n");
  bfmPrint(b, stream, m->ram);
  fprintf(stream, "\n");
}

machine_state *bfmsCopy(bfman *b, machine_state *m)
{
  machine_state *ret = bfmsInitNoRam(b, m->numregs, m->eltsize);

  for (unsigned i = 0; i < m->numregs; i++)
    bfRegStore_le(b, ret, i, bfRegLoad_le(b, m, i, 1));
  
  ret->ram = bfmCopy(b, m->ram);
  ret->isValid = m->isValid;
  return ret;
}

machine_state *bfmsCopyNoRam(bfman *b, machine_state *m)
{
  machine_state *ret = bfmsInitNoRam(b, m->numregs, m->eltsize);

  for (unsigned i = 0; i < m->numregs; i++)
    bfRegStore_le(b, ret, i, bfRegLoad_le(b, m, i, 1));
  
  ret->isValid = m->isValid;
  return ret;
}

machine_state *bfmsSelect(bfman *b, literal condition, machine_state *t, machine_state *e)
{
  assert(t->numregs == e->numregs);
  assert(bfmIdxSize(t->ram) == bfmIdxSize(e->ram));
  assert(bfmEltSize(t->ram) == bfmEltSize(e->ram));

  if (condition == LITERAL_TRUE) {
    bfmsDestroy(b, e);
    return t;
  }
  if (condition == LITERAL_FALSE) {
    bfmsDestroy(b, t);
    return e;
  }

  assert(t->eltsize == e->eltsize);
  machine_state *ret = bfmsInitNoRam(b, t->numregs, t->eltsize);

  ret->isValid = bfNewSelect(b, condition, t->isValid, e->isValid);

  for (unsigned r = 0; r < t->numregs; r++) {
    bitvec *from_t = bfRegLoad_le(b, t, r, 1);
    bitvec *from_e = bfRegLoad_le(b, e, r, 1);
    bfRegStore_le(b, ret, r, bfvSelect(b, condition, from_t, from_e));
  }

  ret->ram = bfmSelect(b, condition, t->ram, e->ram);

  bfmsDestroy(b, t);
  bfmsDestroy(b, e);
  return ret;
}



/** Concretely addressed store (little endian)
 *
 * @pre reg[0] must be initialized to a bit vector of appropriate size
 */
void bfRegStore_le(bfman *b, machine_state *s, unsigned address, bitvec *val) {
  unsigned numElts = val->size / s->eltsize;
  assert((val->size % s->eltsize) == 0);

  bfvHold(val);

  for(unsigned i = 0; i < numElts; i++) {
    bitvec *val_partial = bfvExtract(b, val, i*s->eltsize, s->eltsize);
    bfvCopy(b, s->reg[address], val_partial);
    assert(s->reg[address]->numHolds==1);
    CONSUME_BITVEC(val_partial);
    address++;
  }

  CONSUME_BITVEC(bfvRelease(val));
}

/** Concretely addressed store (little endian)
 *
 * @pre reg[0] must be initialized to a bit vector of appropriate size
 */
void bfRegStore_be(bfman *b, machine_state *s, unsigned address, bitvec *val) {
  unsigned numElts = val->size / s->eltsize;
  assert((val->size % s->eltsize) == 0);

  bfvHold(val);

  for(unsigned i = numElts-1; i != (unsigned)~0; i--) {
    bitvec *val_partial = bfvExtract(b, val, i*s->eltsize, s->eltsize);
    bfvCopy(b, s->reg[address], val_partial);
    assert(s->reg[address]->numHolds==1);
    CONSUME_BITVEC(val_partial);
    address++;
  }

  CONSUME_BITVEC(bfvRelease(val));
}




/** Concretely addressed load of a bitvec (little endian) */
bitvec *bfRegLoad_le(bfman *b, machine_state *s, unsigned address, unsigned numElts) {
  bitvec *ret = bfvAlloc(numElts*s->eltsize);
  
  for(unsigned i = 0; i < numElts; i++) {
    bitvec *partialValue = s->reg[address];
    assert(s->reg[address]->numHolds==1);
    ret = bfvConcat(b, ret, partialValue);
    address++;
  }

  return ret;
}

/** Concretely addressed load of a bitvec (little endian) */
bitvec *bfRegLoad_be(bfman *b, machine_state *s, unsigned address, unsigned numElts) {
  bitvec *ret = bfvAlloc(numElts*s->eltsize);
  
  for(unsigned i = 0; i < numElts; i++) {
    bitvec *partialValue = s->reg[address];
    ret = bfvConcat(b, partialValue, ret);
    address++;
  }

  return ret;
}

