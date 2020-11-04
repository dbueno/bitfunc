/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <assert.h>
#include <math.h>
#include <string.h>
#include <bitfunc.h>
#include <bitfunc/debug.h>
#include <bitfunc/bitvec.h>
#include <bitfunc/array.h>
#include <bitfunc/circuits.h>
#include <bitfunc/circuits_internal.h>
#include <funcsat.h>
#include <funcsat/hashtable_itr.h>
#include <unistd.h>

#ifdef INCREMENTAL_PICOSAT_ENABLED
#include <picosat.h>
#endif

#include "bfprobes.h"

DEFINE_HASHTABLE(andhashInsert, andhashSearch, andhashRemove, struct and_key, literal)
DEFINE_HASHTABLE(litname_insert, litname_search, litname_remove, literal, char)

extern variable fs_lit2var(literal l);

extern void dimacsPrintClause(FILE *f, clause *clause);

extern void bfComputeCnf(bfman *m);

literal bfNewVar(bfman *b)
{
  PROBE(probe_meter_mark(&var_alloc_meter, 1));
  literal l = (literal) ++b->numVars;
  bf_log(b, "varmap", 2, "%jd [%jd]\n", l, bf_aiger2lit(bf_lit2aiger(l)));
  aiger_add_input(b->aig, bf_lit2aiger(l), NULL);
  bf_prep_assignment(b);
  return l;
}

literal bfSetName(bfman *b, literal l, const char *name)
{
  char *val;
  if (NULL != (val = litname_search(b->lit_names, &l))) {
    free(litname_remove(b->lit_names, &l));
  }
  literal *key;
  MallocX(key, 1, sizeof(*key));
  *key = l;
  val = strdup(name);
  if (!val) perror("strdup"), abort();
  litname_insert(b->lit_names, key, val);
  bf_log(b, "varmap", 1, "%s = %jd\n", val, *key);
  return l;
}

char *bfGetName(bfman *b, literal l)
{
  return litname_search(b->lit_names, &l);
}


literal bfAuxVar(bfman *man)
{
  literal l = (literal) ++man->numVars;
  return l;
}

bool bfGet(bfman *b, literal a)
{
  if (a == LITERAL_TRUE) return true;
  else if (a == LITERAL_FALSE) return false;

  mbool assign = bf_get_assignment_lazily(b, fs_lit2var(a));
  assert(assign != unknown);
  if (aiger_sign(bf_lit2aiger(a))) assign = !assign;
  return assign;
}

DEFINE_HASHTABLE(varMapInsert, varMapSearch, varMapRemove, literal, struct array3_t)
DEFINE_HASHTABLE(litSetInsert, litSetSearch, litSetRemove, literal, bool)

#ifdef CEGAR_ARRAY_CONGRUENCE
static literal *dupLit(literal l)
{
  literal *v;
  CallocX(v, 1, sizeof(*v));
  *v = l;
  return v;
}

/* used as the value in a hashtable that represents a set */
static bool YES = true;
#endif

void bfAnd(bfman *m, literal a, literal b, literal o)
{
  bfSetEqual(m, o, bfNewAnd(m, a, b));
}

void bfOr(bfman *m, literal a, literal b, literal o)
{
  bfSetEqual(m, o, bfNewOr(m, a,b));
}

void bfXor(bfman *m, literal a, literal b, literal o)
{
  bfSetEqual(m, o, bfNewXor(m, a, b));
}

void bfNand(bfman *m, literal a, literal b, literal o)
{
  bfSetEqual(m, o, bfNewNand(m, a, b));
}
void bfNor(bfman *m, literal a, literal b, literal o)
{
  bfSetEqual(m, o, bfNewNor(m, a, b));
}

void bfEqual(bfman *man, literal a, literal b, literal o)
{
  bfXor(man, a, b, -o);
}


void bfImplies(bfman *man, literal a, literal b, literal o)
{
  bfOr(man, -a, b, o);
}

  
literal bfNewAnd(bfman *man, literal a, literal b)
{
  if(a==LITERAL_TRUE) return b;
  if(b==LITERAL_TRUE) return a;
  if(a==LITERAL_FALSE) return LITERAL_FALSE;
  if(b==LITERAL_FALSE) return LITERAL_FALSE;
  if(a==b) return a;
  if(a==-b) return LITERAL_FALSE;

  literal o;
  struct and_key template = { .a = a, .b = b };
  literal *found;
  if (NULL == (found = andhashSearch(man->andhash, &template))) {
    PROBE(probe_meter_mark(&hash_miss_meter, 1));
    literal *lhs = calloc(1, sizeof(*lhs));
    *lhs = bfAuxVar(man);
    struct and_key *v = calloc(1, sizeof(struct and_key));
    memcpy(v, &template, sizeof(struct and_key));
    andhashInsert(man->andhash, v, lhs);
    man->addAnd(man, *lhs, a, b);
    o = *lhs;
  } else {
    PROBE(probe_meter_mark(&hash_hit_meter, 1));
    ++man->hash_hits;
    o = *found;
  }
  return o;
}

literal bfNewOr(bfman *man, literal a, literal b)
{
  if(a==LITERAL_FALSE) return b;
  if(b==LITERAL_FALSE) return a;
  if(a==LITERAL_TRUE) return LITERAL_TRUE;
  if(b==LITERAL_TRUE) return LITERAL_TRUE;
  if(a==b) return a;
  if(a==-b) return LITERAL_TRUE;

  literal o;
  o = -bfNewAnd(man, -a, -b);
  return o;
}


literal bfNewXor(bfman *man, literal a, literal b)
{
  if(a==LITERAL_FALSE) return b;
  if(b==LITERAL_FALSE) return a;
  if(a==LITERAL_TRUE) return -b;
  if(b==LITERAL_TRUE) return -a;
  if(a==b) return LITERAL_FALSE;
  if(a==-b) return LITERAL_TRUE;

  literal o;
  o = bfNewAnd(man, -bfNewAnd(man, a, b), -bfNewAnd(man, -a, -b));
  return o;
}

literal bfNewNand(bfman *man, literal a, literal b)
{
  return -bfNewAnd(man, a, b);
}

literal bfNewNor(bfman *man, literal a, literal b)
{
  return -bfNewOr(man, a, b);
}

literal bfNewEqual(bfman *man, literal a, literal b)
{
  return -bfNewXor(man, a, b);
}

literal bfNewIff(bfman *m, literal a, literal b)
{
  return bfNewEqual(m, a, b);
}

literal bfNewImplies(bfman *man, literal a, literal b)
{
  return bfNewOr(man, -a, b);
}

literal bfNewSelect(bfman *m, literal a, literal b, literal c)
{
  // a?b:c = (a AND b) OR (/a AND c)
  if(a==LITERAL_TRUE) return b;
  if(a==LITERAL_FALSE) return c;
  if(b==c) return b;

  literal x;
  x = -bfNewAnd(m, -bfNewAnd(m, a, b), -bfNewAnd(m, -a, c));

  return x;
}

void bfSetEqual(bfman *man, literal a, literal b)
{
  bfSet(man, bfNewOr(man, a, -b));
  bfSet(man, bfNewOr(man, -a, b));
}


mbool bfMayBeTrue(bfman *man, literal x)
{
  if (bf_litIsTrue(x)) return true;
  else if (bf_litIsFalse(x)) return false;

  bfresult r =  bfPushAssumption(man, x);
  if(r != BF_UNSAT) {
    r = bfSolve(man);
    bfPopAssumptions(man, 1);
  }
  if (r == BF_UNKNOWN) return unknown;
  else if (r == BF_SAT) return true;
  else return false;
}

mbool bfMayBeFalse(bfman *man, literal x)
{
  return bfMayBeTrue(man, -x);
}


mbool bfMustBeTrue(bfman *man, literal x)
{
  if (bf_litIsTrue(x)) return true;
  if (bf_litIsFalse(x)) return false;

  bfresult r = bfPushAssumption(man, -x);
  if(r != BF_UNSAT) {
    r = bfSolve(man);
    bfPopAssumptions(man, 1);
  }
  if (r == BF_UNKNOWN) return unknown;
  else if (r == BF_UNSAT) return true;
  else return false;
}

mbool bfMustBeFalse(bfman *man, literal x)
{
  return bfMustBeTrue(man, -x);
}

bfstatus bfCheckStatus(bfman *man, literal a)
{
  if (bf_litIsTrue(a)) {
    return STATUS_MUST_BE_TRUE;    //a must be true
  } else if (bf_litIsFalse(a)) {
    return STATUS_MUST_BE_FALSE;   //a must be false
  }

  unsigned curr_num_assumps = man->assumps->size;


  bfresult resultF = bfPushAssumption(man, -a);
  if(resultF != BF_UNSAT) {
    resultF = bfSolve(man);
    bfPopAssumptions(man, 1);
  }

  bfresult resultT = bfPushAssumption(man, a);
  if(resultT != BF_UNSAT) {
    resultT = bfSolve(man);
    bfPopAssumptions(man, 1);
  }

  assert(man->assumps->size == curr_num_assumps);

  
  bfstatus ret;
  /* assert(resultT != P_ERROR && resultF != P_ERROR); */
  if (resultT == BF_SAT && resultF == BF_UNSAT){
    ret = STATUS_MUST_BE_TRUE; //a must be true
  } else if (resultT == BF_UNSAT && resultF == BF_SAT) {
    ret = STATUS_MUST_BE_FALSE; //a must be false
  } else if (resultT == BF_SAT && resultF == BF_SAT) {
    ret = STATUS_TRUE_OR_FALSE; //a can be either true or false
  } else if (resultT == BF_UNSAT && resultF == BF_UNSAT) {
    ret = STATUS_NOT_TRUE_NOR_FALSE; //unsatisfiable :(
  } else{
    ret = STATUS_UNKNOWN;
  }
  return ret;
}

literal bfBigAnd(bfman *m, bitvec *bv)
{
  /* stands for the result of the conjunction */
  literal lit;

  literal *l;
  forBv(l, bv) {
    if (*l == LITERAL_FALSE) { CONSUME_BITVEC(bv); return LITERAL_FALSE; }
  }

  bitvec *newBv = bfvAlloc(bv->size);

  eliminateDuplicates(m, bv, newBv);

  if(newBv->size==0) {
    lit = LITERAL_TRUE;
  } else if(newBv->size==1) {
    lit = newBv->data[0];
  } else if(newBv->size==2) {
    lit = bfNewAnd(m, newBv->data[0], newBv->data[1]);
  } else {

    while (newBv->size > 1) {
      for (uint32_t i = 1; i < newBv->size; i+=2) {
	newBv->data[i/2] = bfNewAnd(m, newBv->data[i-1], newBv->data[i]);
      }
      if (newBv->size % 2 == 1) {
	newBv->data[newBv->size/2] = newBv->data[newBv->size-1];
	newBv->size = newBv->size/2+1;
      } else {
	newBv->size = newBv->size/2;
      }
    }
    lit = newBv->data[0];
  }

  CONSUME_BITVEC(bv);
  CONSUME_BITVEC(newBv);

  return lit;  
}

literal bfBigOr(bfman *m, bitvec *bv)
{
  /* stands for the result of the disjunction */
  literal lit;

  literal *l;
  forBv(l, bv) {
    if (*l == LITERAL_TRUE) { CONSUME_BITVEC(bv); return LITERAL_TRUE; }
  }

  bitvec *newBv = bfvAlloc(bv->size);

  eliminateDuplicates(m, bv, newBv);

  if(newBv->size==0) {
    lit = LITERAL_FALSE;
  } else if(newBv->size==1) {
    lit = newBv->data[0];
  } else if(newBv->size==2) {
    lit = bfNewOr(m, newBv->data[0], newBv->data[1]);
  } else {
    while (newBv->size > 1) {
      for (uint32_t i = 1; i < newBv->size; i+=2) {
	newBv->data[i/2] = bfNewOr(m, newBv->data[i-1], newBv->data[i]);
      }
      if (newBv->size % 2 == 1) {
	newBv->data[newBv->size/2] = newBv->data[newBv->size-1];
	newBv->size = newBv->size/2+1;
      } else {
	newBv->size = newBv->size/2;
      }
    }
    lit = newBv->data[0];
  }

  CONSUME_BITVEC(bv);
  CONSUME_BITVEC(newBv);

  return lit;  
}

literal bfBigXor(bfman *m, bitvec *bv)
{
  /* stands for the result of the equation */
  literal lit;

  if (bv->size == 0) {
    lit = LITERAL_FALSE;
  } else if (bv->size == 1) {
    lit = bv->data[0];
  } else if (bv->size == 2) {
    lit = bfNewXor(m, bv->data[0], bv->data[1]);
  } else {
    bitvec *temps = bfvAlloc((uint32_t) bv->size/2+1);
    for (uint32_t i = 1; i < bv->size; i+= 2) {
      temps->data[i/2] = bfNewXor(m, bv->data[i-1], bv->data[i]);
    }
    if (bv->size % 2 == 1) {
      temps->data[bv->size/2] = bv->data[bv->size-1];
      temps->size = bv->size/2+1;
    } else {
      temps->size = bv->size/2;
    }
    while (temps->size > 1) {
      for (uint32_t i = 1; i < temps->size; i+= 2) {
	temps->data[i/2] = bfNewXor(m, temps->data[i-1], temps->data[i]);
      }
      if (temps->size % 2 == 1) {
	temps->data[temps->size/2] = temps->data[temps->size-1];
	temps->size = temps->size/2+1;
      } else {
	temps->size = temps->size/2;
      }
    }
    lit = temps->data[0];
    CONSUME_BITVEC(temps);
  }

  CONSUME_BITVEC(bv);

  return lit;
}

array *bfaInit(bfman *UNUSED(man), uint32_t ind_size, uint32_t ele_size)
{
  return newArray(ind_size, ele_size);
}

struct array *bfaWrite(bfman *UNUSED(man), struct array *at, bitvec *index, bitvec *value)
{
  return newArrayFrom(at, index, value);
}

struct array *bfaWriteAtomic(bfman *UNUSED(man), struct array *at, bitvec *index, bitvec *value)
{
  return writeArrayAtomic(at, index, value);
}

bitvec *bfaRead(bfman *man, struct array *at, bitvec *index)
{
  bool ignored;

  return bfaReadGen(man, at, index, &ignored);
}

bitvec *bfaReadGen(bfman *man, struct array *at, bitvec *index, bool *perfect)
{
  if (arrayIsBare(at)) {
    struct hashtable *map = at->bare->readMap;
    bitvec *value = readMapSearch(map, index);
    /* readhashmap::iterator it = map->find(index), ie; */
    bitvec *bare_bitvec;
    assert(at->bare->idxSize == index->size);
        
    if (!value) {     // bare bits are new
      bare_bitvec = bfvInit(man, at->bare->eltSize);
      readMapInsert(map, index, bare_bitvec);
    }
    else { // bare bits are not new
      bare_bitvec = value;
    }
    return bare_bitvec;
  } else {
    literal if_c;
    bitvec *then_c, *else_c, *memoed;

    if (NULL != (memoed = hashtable_search(at->memo, index))) {
      return memoed;
    }

    if (arrayIsAtomic(at)) {
      if (NULL != (then_c = hashtable_search(at->atomic_writes, index))) {
        return then_c;
      } else {
        else_c = bfaReadGen(man, at->next, index, perfect);
        struct hashtable_itr *awit = hashtable_iterator(at->atomic_writes);
        if (hashtable_count(at->atomic_writes)) {
          do {
            if_c = bfvEqual(man, index, hashtable_iterator_key(awit));
            if (bf_litIsFalse(if_c)) continue;
            then_c = hashtable_iterator_value(awit);
            if (bf_litIsTrue(if_c)) return then_c;
            else_c = bfvSelect(man, if_c, then_c, else_c);
          } while (hashtable_iterator_advance(awit));
        }
        hashtable_insert(at->memo, index, else_c);
        return else_c;
      }
    } else {
      if_c = bfvEqual(man, index, at->index);
      if (bf_litIsTrue(if_c)) {
        *perfect = true;
        then_c = at->value;
        return then_c;
      } else if (bf_litIsFalse(if_c)) {
        else_c = bfaReadGen(man, at->next, index, perfect);
        return else_c;
      } else {
        then_c = at->value;
        else_c = bfaReadGen(man, at->next, index, perfect);
        memoed = bfvSelect(man, if_c, then_c, else_c);
        hashtable_insert(at->memo, index, memoed);
        return memoed;
      }
    }
  }
}


uint64_t bfvGet(bfman *bf, bitvec *bv)
{
  INPUTCHECK(bf, bv->size <= 64, "bfvGet only works with >= 64-bit bit vectors");
  uint64_t res = 0;
  for (uint32_t offset = 0; offset < bv->size; offset++) {
    uint64_t val;
    if (bf_litIsTrue(bv->data[offset])) {
      val = 1;
    } else if (bf_litIsFalse(bv->data[offset])) {
      val = 0;
    } else {
      mbool v = bfGet(bf, bv->data[offset]);
      assert(v != unknown);
      val = v == true ? 1 : 0;
    }
    res |= (val << offset);
  }
  return res;
}


static array *_bfaFromCounterExample(bfman *b, array *a)
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
        readMapInsert(rbare->readMap,
                      bfvFromCounterExample(b, readIndex),
                      bfvFromCounterExample(b, readValue));
      } while (hashtable_iterator_advance(it));
      free(it);
    }
  } else {
    /* ret = _bfaFromCounterExample(b, a->next); */
    array *rnext = _bfaFromCounterExample(b, a->next);
    ret = newArrayFrom(rnext,
                       bfvFromCounterExample(b, a->index),
                       bfvFromCounterExample(b, a->value));
  }

  return ret;
}

array *bfaFromCounterExample(bfman *b, array *a)
{
  return _bfaFromCounterExample(b,a);
}


