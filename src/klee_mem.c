/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
/* This file implements a Klee-like memory model.

   Each time an allocation is performed, we allocate a new heap block.
*/

#include "klee_mem.h"
#include "mem.h"
#include "funcsat/vec.h"
#include "bitfunc.h"
#include "circuits.h"

#define UNUSED(x) (void)(x)


klee_mem *klee_mem_init(bfman *BF)
{
  (void)(BF);
  klee_mem *ret;
  CallocX(ret, 1, sizeof(*ret));
  vectorInit(&ret->blocks, 2);

  return ret;
}

/* if it returns true, the address may point into the block. */
static mbool addr_may_use_mem_blk(bfman *BF, bitvec *addr, struct mem_blk *blk, literal *test_out)
{
  memory *mem = blk->mem;
  bitvec *base_bv = bfvUconstant(BF, mem->idxsize, blk->base);
  bitvec *top_bv  = bfvUconstant(BF, mem->idxsize, blk->base+blk->len);
  bool ret;
  literal may = bfNewAnd(BF, bfvUlte(BF, base_bv, addr), bfvUlt(BF, addr, top_bv));
  if (test_out) *test_out = may;
  if (BF_UNSAT == bfPushAssumption(BF, may)) {
    return false;
  }

  switch (bfSolve(BF)) {
  case BF_SAT:
    ret = true;
    break;
  case BF_UNSAT:
    ret = false;
    break;
  case BF_UNKNOWN:
    ret = unknown;
    break;
  }
  bfPopAssumptions(BF, 1);
  return ret;
}

/* if it returns true, the address must point into the block. */
static mbool addr_must_use_mem_blk(bfman *BF, bitvec *addr, struct mem_blk *blk)
{
  literal may;
  bool ret = false;
  if (addr_may_use_mem_blk(BF, addr, blk, &may) == true) {
    if (BF_UNSAT == bfPushAssumption(BF, -may)) {
      return true;
    }

    switch (bfSolve(BF)) {
    case BF_SAT:
      ret = false;
    break;
    case BF_UNSAT:
      ret = true;
      break;
    case BF_UNKNOWN:
      ret = unknown;
      break;
    }
  }
  return ret;
}

/*
  Puts in `blocks_in' the list of blocks that the `addr' can _possibly_ refer to. In
  other words, if there is a satisfying assignment where `addr' points inside a
  given block, that block will be included in `blocks_in'.  */
static int find_may_blocks(bfman *BF, klee_mem *m, bitvec *addr, vector *blocks_in)
{
  struct mem_blk **m_elt;
  bfvHold(addr);
  forVector (struct mem_blk **, m_elt, &m->blocks) {
    if (addr_may_use_mem_blk(BF, addr, *m_elt, NULL) == true)
      vectorPush(blocks_in, *m_elt);
  }

  CONSUME_BITVEC(bfvRelease(addr));

  return 0;
}

static int find_must_blocks(bfman *BF, klee_mem *m, bitvec *addr, vector *blocks_in)
{
  struct mem_blk **m_elt;
  bfvHold(addr);
  forVector (struct mem_blk **, m_elt, &m->blocks) {
    if (addr_must_use_mem_blk(BF, addr, *m_elt) == true)
      vectorPush(blocks_in, *m_elt);
  }

  CONSUME_BITVEC(bfvRelease(addr));

  return 0;
}
