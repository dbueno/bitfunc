/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <bitfunc/circuits.h>
#include <bitfunc/array.h>
#include <bitfunc/circuits_internal.h>
#include <bitfunc/mem.h>
#include <bitfunc/bitvec.h>
#include <funcsat/hashtable_itr.h>
#include <funcsat.h>
#include <bitfunc.h>

#define HAVE_WE_BEEN_HERE(b, m) {            \
    if (!m) return;                          \
    if (m->memflag == b->memflag) return;    \
    m->memflag = b->memflag;                 \
  }

memory *bfmInit(bfman *b, uint8_t idxsize, uint8_t eltsize)
{
  memory *m;
  CallocX(m, 1, sizeof(*m));
  m->arr = vectorAlloc(10);
  m->idxsize = idxsize;
  m->eltsize = eltsize;
  m->c = LITERAL_TRUE;
  vectorPush(&b->memories, m);
  return m;
}

memory *bfmInitNoArray(bfman *b)
{
  memory *m;
  CallocX(m, 1, sizeof(*m));
  m->c = LITERAL_TRUE;
  vectorPush(&b->memories, m);
  return m;
}

memoryCell *bfmInitMemoryCell(bfman *b, memory *m, bitvec *index, bitvec *value) {
  memoryCell *mc = malloc(sizeof(*mc));

  assert(index->size == bfmIdxSize(m));
  assert(value->size == bfmEltSize(m));
  
  mc->index = bfvHold(bfvDup(b, index));
  mc->value = bfvHold(bfvDup(b, value));

  return mc;
}

static void bfmFlag(bfman *b, memory *m)
{
  HAVE_WE_BEEN_HERE(b,m);
  bfmFlag(b, m->memt), bfmFlag(b, m->memf);
}

static void collectNotFlagged(bfman *b, memory *m, vector *stack)
{
  HAVE_WE_BEEN_HERE(b,m);
  collectNotFlagged(b, m->memt, stack), collectNotFlagged(b, m->memf, stack);
  vectorPush(stack, m);
}

static void bfmDestroyMemoryCell(memoryCell *mc)
{
  assert(mc->index->numHolds == 1);
  CONSUME_BITVEC(bfvRelease(mc->index));
  assert(mc->value->numHolds == 1);
  CONSUME_BITVEC(bfvRelease(mc->value));
  free(mc);
}

static void bfmDestroyMemoryCells(memory *m)
{
  memoryCell **mcIt;
  forVector (memoryCell **, mcIt, m->arr) {
    bfmDestroyMemoryCell(*mcIt);
  }
  vectorDestroy(m->arr); free(m->arr);
}

void bfmUnsafeFree(memory *m)
{
  bfmDestroyMemoryCells(m);
  if (m->memvalue != NULL) {
    assert(m->memvalue->numHolds == 1);
    CONSUME_BITVEC(bfvRelease(m->memvalue));
    m->memvalue = NULL;
  }
  free(m);
}

void bfmDestroy(bfman *b, memory *m)
{
  b->memflag++;
  memory **mIt;

  for (unsigned head = b->memories.size-1; head != ~(unsigned)0; head--) {
    memory *sMem_tmp = (memory *)b->memories.data[head];
    if (sMem_tmp != m) bfmFlag(b, sMem_tmp);
    else {
      //Remove sMem from stack
      b->memories.data[head] = b->memories.data[b->memories.size-1];
      b->memories.size--;
    }
  }

  vector *stack = vectorAlloc(12);
  collectNotFlagged(b, m, stack);
  forVector (memory **, mIt, stack) {
    bfmDestroyMemoryCells(*mIt);
    if(m->memvalue != NULL) {
      assert(m->memvalue->numHolds == 1);
      CONSUME_BITVEC(bfvRelease(m->memvalue));
      m->memvalue = NULL;
    }
    free(*mIt);
  }
  vectorDestroy(stack), free(stack);
}

void bfmPrintMemoryCell(bfman *b, FILE *out, memoryCell *mc) {
  if(bfvIsConstant(mc->index)) {
    bfvPrintConcrete(b, out, mc->index, PRINT_HEX);
  } else {
    bfvPrint(out, mc->index);
  }
  fprintf(out, ":");
  
  if(bfvIsConstant(mc->value)) {
    bfvPrintConcrete(b, out, mc->value, PRINT_HEX);
  } else {
    bfvPrint(out, mc->value);
  }
  fprintf(out, "\n");
  
}

void bfmPrintMemoryCells(bfman *b, FILE *out, vector *arr) {
  memoryCell **mcIt;
  forVectorRev (memoryCell **, mcIt, arr) {
    bfmPrintMemoryCell(b, out, *mcIt);
  }
}

void _bfmPrint(bfman *b, FILE *out, memory *m)
{
  HAVE_WE_BEEN_HERE(b,m);
  fprintf(out, "%p (true = %p, false = %p)\n", m, m->memt, m->memf);
  fprintf(out, "c = "), bflPrint(out, m->c), fprintf(out, "\n");
  fprintf(out, "flag (%u)\n", m->memflag);
  if (m->memvalue) fprintf(out, "memvalue = "), bfvPrint(out, m->memvalue), fprintf(out, "\n");
  if (m->flagged_mem) fprintf(out, "flagged: %p\n", m->flagged_mem);

  bfmPrintMemoryCells(b, out, m->arr);

  _bfmPrint(b, out, m->memt), _bfmPrint(b, out, m->memf);
}

void bfmPrint(bfman *b, FILE *out, memory *m)
{
  b->memflag++;
  _bfmPrint(b, out, m);
}

void _bfmCheck(bfman *b, memory *m)
{
  HAVE_WE_BEEN_HERE(b,m);
  _bfmCheck(b, m->memt), _bfmCheck(b, m->memf);
  assert(m->arr);
  assert(!m->memvalue || (m->memvalue->numHolds==1));
  memoryCell **mcIt;
  forVector (memoryCell **, mcIt, m->arr) {
    assert((*mcIt)->index->numHolds==1);
    assert((*mcIt)->value->numHolds==1);
  }
}

void bfmCheck(bfman *b, memory *m)
{
  b->memflag++;
  _bfmCheck(b, m);
}

/** @return whether there was a match to remove */
bool removeMapping(bfman *UNUSED(b), memory *m, bitvec *index)
{
  assert(index->size == bfmIdxSize(m));
  for(unsigned i = 0; i < m->arr->size; i++) {
    memoryCell *mc = (memoryCell *)m->arr->data[i];
    if(bfvSame(mc->index, index)==true) {
      memoryCell *mcPop = vectorPopAt(m->arr, i);
      bfmDestroyMemoryCell(mcPop);
      return true;
    }
  }
  return false;
}

unsigned bfmIdxSize(memory *m)
{
  return m->idxsize;
}

unsigned bfmEltSize(memory *m)
{
  return m->eltsize;
}

void bfmStore(bfman *b, memory *m, bitvec *index, bitvec *value)
{
  assert(index->size == bfmIdxSize(m));
  assert(value->size == bfmEltSize(m));
  bfvHold(index);
  if (b->autocompress) removeMapping(b, m, index);

  memoryCell *mc = bfmInitMemoryCell(b, m, index, value);
  vectorPush(m->arr, mc);

  bfmCheck(b, m);
  CONSUME_BITVEC(bfvRelease(index)); //redundant
}

/** Symbolically addressed store (little endian) */
void bfmStore_le(bfman *b, memory *m, bitvec *address_start, bitvec *val) {
  assert((val->size % bfmEltSize(m)) == 0);
  unsigned numElts = val->size / bfmEltSize(m);

  bfvHold(address_start);
  bfvHold(val);

  for(unsigned i = 0; i < numElts; i++) {
    bitvec *address = bfvAdd0(b, address_start, bfvUconstant(b, bfmIdxSize(m), i));
    bitvec *val_partial = bfvExtract(b, val, i*bfmEltSize(m), bfmEltSize(m));
    bfmStore(b, m, address, val_partial);
  }

  CONSUME_BITVEC(bfvRelease(val));
  CONSUME_BITVEC(bfvRelease(address_start));
}

/** Symbolically addressed store (big endian) */
void bfmStore_be(bfman *b, memory *m, bitvec *address, bitvec *val) {
  assert((val->size % bfmEltSize(m)) == 0);
  unsigned numElts = val->size / bfmEltSize(m);

  bfvHold(val);
  for(int i = (int)numElts-1; i >= 0; i--) {
    bitvec *val_partial = bfvExtract(b, val, ((unsigned)i)*bfmEltSize(m), bfmEltSize(m));
    bfvHold(address);
    bfmStore(b, m, address, val_partial);
    address = bfvAdd0(b, bfvRelease(address), bfvUconstant(b, bfmIdxSize(m), 1));
  }

  CONSUME_BITVEC(address);
  CONSUME_BITVEC(bfvRelease(val));
}

/** Symbolically addressed store of a Vector of bitvecs (little endian) */
void bfmStoreVector_le(bfman *b, memory *m, bitvec *address_start, vector *values) {
  unsigned address_add = 0;

  bfvHold(address_start);

  for(unsigned i = 0; i < values->size; i++) {
    bitvec *val = (bitvec *)values->data[i];
    bitvec *address = bfvAdd0(b, address_start, bfvUconstant(b, bfmIdxSize(m), address_add));
    address_add = val->size / bfmEltSize(m);
    bfmStore_le(b, m, address, val);
  }

  CONSUME_BITVEC(bfvRelease(address_start));
}

/** Symbolically addressed store of a Vector of bitvecs (big endian) */
void bfmStoreVector_be(bfman *b, memory *m, bitvec *address_start, vector *values) {
  unsigned address_add = 0;
  
  bfvHold(address_start);

  for(unsigned i = 0; i < values->size; i++) {
    bitvec *val = (bitvec *)values->data[i];
    bitvec *address = bfvAdd0(b, address_start, bfvUconstant(b, bfmIdxSize(m), address_add));
    address_add = val->size / bfmEltSize(m);
    bfmStore_be(b, m, address, val);
  }
  
  CONSUME_BITVEC(bfvRelease(address_start));
}

static bool loadLocal(bfman *b, memory *m, bitvec *addr, vector *values, clause *conditions)
{
  memoryCell **mcIt;
  forVectorRev (memoryCell **, mcIt, m->arr) {
    memoryCell *mc = (*mcIt);
    literal c = bfvEqual(b, addr, mc->index);
    if(b->memLoadWithSat) {
      mbool must_be_false = bfMustBeTrue_picosatIncremental(b, -c);
      if(must_be_false == true && !bf_litIsFalse(c)) {
	c = LITERAL_FALSE;
      } else {
	mbool must_be_true = bfMustBeTrue_picosatIncremental(b, c);
	if(must_be_true == true && !bf_litIsTrue(c)) {
	  c = LITERAL_TRUE;
	}
      }
    }
    if (bf_litIsTrue(c)) {
      vectorPush(values, bfvDup(b, mc->value));
      return true;            /* perfect match */
    } else if (bf_litIsFalse(c)) {
      continue;
    } else {
      if(b->memLoadWithSat) {
	mbool must_be_false = bfMustBeTrue_picosatIncremental(b, -c);
	if(must_be_false == true) {
	  continue;
	} else {
	  mbool must_be_true = bfMustBeTrue_picosatIncremental(b, c);
	  if(must_be_true == true) {
	    vectorPush(values, bfvDup(b, mc->value));
	    return true;            /* perfect match */
	  }
	}
      }
      clausePush(conditions, c);
      vectorPush(values, bfvDup(b, mc->value));
    }
  }
  
  return false;
}

static bitvec *stackMerge(bfman *b, vector *values, clause *conditions, literal *rbw)
{
  assert(values->size == conditions->size+(unsigned)1);
  bitvec *vF = vectorPop(values);

  if (rbw) *rbw = LITERAL_TRUE;

  while (values->size > 0) {
    bitvec *vT = vectorPop(values);
    literal c = clausePop(conditions);
    if (rbw) {
      /* *rbw is set to true iff all the conditions are false, meaning we read
         a value we never wrote. */
      *rbw = bfNewAnd(b, *rbw, -c);
    }
    assert(vT->numHolds == 0);
    assert(vF->numHolds == 0);
    vF = bfvSelect(b, c, vT, vF);
  }

  assert(values->size == 0 && conditions->size == 0);
  return vF;
}

static bitvec *_bfmLoad(bfman *b, memory *m, bitvec *addr, literal *rbw)
{
  if (rbw) *rbw = LITERAL_FALSE;
  if (m->memflag == b->memflag) {
    assert(m->memvalue);
    return bfvDup(b, m->memvalue);
  }
  m->memflag = b->memflag;

  vector *values = vectorAlloc(10);
  clause *conditions = clauseAlloc(10);

  bool perfectmatch = loadLocal(b, m, addr, values, conditions);
  bitvec *ret;
  if (perfectmatch) {
    ret = stackMerge(b, values, conditions, NULL);
  } else if (!m->memf) {
    if (!m->memt) {
      vectorPush(values, bfvUconstant(b, bfmEltSize(m), 0));
      ret = stackMerge(b, values, conditions, rbw);
    } else {
      bitvec *rett = _bfmLoad(b, m->memt, addr, rbw);
      vectorPush(values, rett);
      ret = stackMerge(b, values, conditions, NULL);
    }
  } else {
    assert(m->memt);
    bitvec *rett = _bfmLoad(b, m->memt, addr, rbw);
    bitvec *retf = _bfmLoad(b, m->memf, addr, rbw);
    clausePush(conditions, m->c), vectorPush(values, rett);
    vectorPush(values, retf);
    ret = stackMerge(b, values, conditions, NULL);
  }

  vectorDestroy(values), free(values);
  clauseFree(conditions);
  
  if(m->memvalue != NULL) {
    assert(m->memvalue->numHolds == 1);
    CONSUME_BITVEC(bfvRelease(m->memvalue));
  }
  m->memvalue = bfvHold(bfvDup(b, bfvHold(ret)));
  bfvRelease(ret);
  return ret;
}

bitvec *bfmLoad_RBW(bfman *b, memory *m, bitvec *addr, literal *rbw)
{
  b->memflag++;

  bitvec *ret = _bfmLoad(b, m, bfvHold(addr), rbw);
  assert(ret->numHolds == 0);

  bfmCheck(b, m);
  CONSUME_BITVEC(bfvRelease(addr));

  return ret;
}

bitvec *bfmLoad(bfman *b, memory *m, bitvec *addr)
{
  return bfmLoad_RBW(b, m, addr, NULL);
}

bitvec *bfmLoad_le(bfman *b, memory *m, bitvec *address_start, unsigned numElts)
{
  return bfmLoad_le_RBW(b, m, address_start, numElts, NULL);
}

bitvec *bfmLoad_le_RBW(bfman *b, memory *m, bitvec *address_start, unsigned numElts, literal *rbw) {
  bitvec *ret = bfvAlloc(bfmEltSize(m));

  bfvHold(address_start);

  if (rbw) *rbw = LITERAL_FALSE;
  
  for (unsigned i = 0; i < numElts; i++) {
    literal tmp_rbw;
    bitvec *address = bfvAdd0(b, address_start, bfvUconstant(b, bfmIdxSize(m), i));
    bitvec *partialValue;
    if (rbw) {
      partialValue = bfmLoad_RBW(b, m, address, &tmp_rbw);
      *rbw = bfNewOr(b, *rbw, tmp_rbw);
    } else
      partialValue = bfmLoad(b, m, address);
    ret = bfvConcat(b, ret, partialValue);
  }

  CONSUME_BITVEC(bfvRelease(address_start));

  return ret;
}

bitvec *bfmLoad_be(bfman *b, memory *m, bitvec *address_start, unsigned numElts)
{
  return bfmLoad_be_RBW(b, m, address_start, numElts, NULL);
}

bitvec *bfmLoad_be_RBW(bfman *b, memory *m, bitvec *address_start, unsigned numElts, literal *rbw) {
  bitvec *ret = bfvAlloc(bfmEltSize(m));

  bfvHold(address_start);

  if (rbw) *rbw = LITERAL_FALSE;

  for (int i = (int)numElts-1; i >= 0; i--) {
    literal tmp_rbw;
    bitvec *address = bfvAdd0(b, address_start, bfvUconstant(b, bfmIdxSize(m), (unsigned)i));
    bitvec *partialValue;
    if (rbw) {
      partialValue = bfmLoad_RBW(b, m, address, &tmp_rbw);
      *rbw = bfNewOr(b, *rbw, tmp_rbw);
    } else
      partialValue = bfmLoad(b, m, address);
    ret = bfvConcat(b, ret, partialValue);
  }

  CONSUME_BITVEC(bfvRelease(address_start));

  return ret;
}

/** Symbolically addressed load of a Vector of bitvecs (big endian) */
vector *bfmLoadVector_le(bfman *b, memory *m, bitvec *address_start, unsigned numVecs, unsigned vecSize) {
  vector *ret = vectorAlloc(numVecs);
  
  bfvHold(address_start);
    
  for(unsigned i = 0; i < numVecs; i++) {
    bitvec *address = bfvAdd0(b, address_start, bfvUconstant(b, bfmIdxSize(m), i*vecSize));
    bitvec *val = bfmLoad_le(b, m, address, vecSize);
    vectorPush(ret, val);
  }

  CONSUME_BITVEC(bfvRelease(address_start));

  return ret;
}

/** Symbolically addressed load of a Vector of bitvecs (big endian) */
vector *bfmLoadVector_be(bfman *b, memory *m, bitvec *address_start, unsigned numVecs, unsigned vecSize) {
  vector *ret = vectorAlloc(numVecs);
    
  bfvHold(address_start);

  for(unsigned i = 0; i < numVecs; i++) {
    bitvec *address = bfvAdd0(b, address_start, bfvUconstant(b, bfmIdxSize(m), i*vecSize));
    bitvec *val = bfmLoad_be(b, m, address, vecSize);
    vectorPush(ret, val);
  }

  CONSUME_BITVEC(bfvRelease(address_start));
  
  return ret;
}

/** Concretely addressed load of a bitvec (little endian) */
bitvec *bfvLoad_le(bfman *b, bitvec **reg, unsigned address, unsigned numElts) {
  unsigned eltSize = reg[0]->size;
  bitvec *ret = bfvAlloc(numElts*eltSize);
  
  for(unsigned i = 0; i < numElts; i++) {
    bitvec *partialValue = reg[address];
    assert(reg[address]->numHolds==1);
    ret = bfvConcat(b, ret, partialValue);
    address++;
  }

  return ret;
}

/** Concretely addressed load of a bitvec (little endian) */
bitvec *bfvLoad_be(bfman *b, bitvec **reg, unsigned address, unsigned numElts) {
  unsigned eltSize = reg[0]->size;
  bitvec *ret = bfvAlloc(numElts*eltSize);
  
  for(unsigned i = 0; i < numElts; i++) {
    bitvec *partialValue = reg[address];
    ret = bfvConcat(b, partialValue, ret);
    address++;
  }

  return ret;
}

memory *bfmCopy(bfman *b, memory *m)
{
  memory *ret = bfmInit(b, bfmIdxSize(m), bfmEltSize(m));
  ret->memt = m;
  return ret;
}

memory *bfmSelect(bfman *b, literal c, memory *t, memory *f)
{
  assert(bfmIdxSize(t) == bfmIdxSize(f));
  assert(bfmEltSize(t) == bfmEltSize(f));
  if (bf_litIsTrue(c)) {
    return t;
  }
  if (bf_litIsFalse(c)) {
    return f;
  }
  memory *r = bfmInit(b, bfmIdxSize(t), bfmEltSize(t));
  r->c = c;
  r->memt = t;
  r->memf = f;

  return r;
}

vector *bfmMemoryCellsFromCounterExample(bfman *b, memory *m, vector *arr)
{
  vector *retarr = vectorAlloc(m->arr->size);

  memoryCell **mcIt;
  forVector (memoryCell **, mcIt, arr) {
    memoryCell *mc = (*mcIt);
    memoryCell *retmc = bfmInitMemoryCell(b, m, bfvFromCounterExample(b, mc->index), bfvFromCounterExample(b, mc->value));
    vectorPush(retarr, retmc);
  }

  return retarr;
}

memory *_bfmFromCounterExample(bfman *b, memory *m)
{
  if (!m) return NULL;
  if (m->memflag == b->memflag) {
    assert(m->flagged_mem);
    return m->flagged_mem;
  }
  m->memflag = b->memflag;

  memory *ret = bfmInitNoArray(b);

  ret->idxsize = m->idxsize;
  ret->eltsize = m->eltsize;

  ret->c = bflFromCounterExample(b, m->c);
  ret->arr = bfmMemoryCellsFromCounterExample(b, m, m->arr);

  ret->memt = _bfmFromCounterExample(b, m->memt);
  ret->memf = _bfmFromCounterExample(b, m->memf);

  m->flagged_mem = ret;

  bfmDestroy(b, ret->memf);
  bfmDestroy(b, ret->memt);

  return ret;
}

memory *bfmFromCounterExample(bfman *b, memory *m)
{
  b->memflag++;
  return _bfmFromCounterExample(b, m);
}
