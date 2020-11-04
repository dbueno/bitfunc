#ifndef bf_circuits_internal_h
#define bf_circuits_internal_h

#include <bitfunc/array.h>
#include <bitfunc/circuits.h>
#include <funcsat.h>
#include <bitfunc/aiger.h>

#include <bitfunc/bitfunc_internal.h>

/**
 * Adds the clauses for the AIG's cone of influence of l.
 *
 * If out is NULL, Uses ::bfman::addClauseToSolver. Otherwise, writes the
 * clauses in DIMACS CNF format to the given file.
 */
unsigned addLitConeOfInfluence(bfman *m, aiger_literal l, FILE *out);

array *bfaWrite(bfman *m, array *at, bitvec *index, bitvec *value);
array *bfaWriteAtomic(bfman *m, array *at, bitvec *index, bitvec *value);

bitvec *bfaRead(bfman *m, array *at, bitvec *index);
bitvec *bfaReadGen(bfman *m, array *at, bitvec *index, bool *perfectmatch);

array *bfaInit(bfman *m, uint32_t ind_size, uint32_t ele_size);



/// 
/// Shift val by pow(2,s) bits according to dist or leave it unaltered.
static bitvec *leftBarrelShift(bfman *man, bitvec *val, bitvec *dist, int32_t s);
static bitvec *rightBarrelShift(bfman *man, bitvec *val, bitvec *dist, int32_t s, literal fill);


extern int litCompare(const void *l1, const void *l2);

static void sortBitvec(bitvec *v)
{
  qsort(v->data, v->size, sizeof(*v->data), litCompare);
}

extern bool clauseInsert(clause *c, literal p);
extern void funcsatResize(funcsat *f, variable numVars);

static void eliminateDuplicates(bfman *man, bitvec *bv, bitvec *dest)
{
  bfvGrowTo(dest, bv->size);

  bfvCopy(man, dest, (bitvec *)bv);

  if (dest->size > 1) {
    unsigned size = dest->size;
    sortBitvec(dest);
    literal *i, *j, *end;
    /* i is current, j is target */
    for (i = j = (literal *) dest->data, end = i + dest->size; i != end; i++) {
      literal p = *i, q = *j;
      if (i != j) {
        if (p == q) {           /* duplicate literal */
          size--;
          continue;
        } else *(++j) = p;
      }
    }
    dest->size = size;
  }

}

/**
 * Actually calls funcsat.
 */
bfresult bfSolveHelper(bfman *m, int64_t lim);
/* maps */
typedef struct hashtable readhashmap;
typedef struct hashtable valuehashmap;
typedef struct hashtable varhashmap;
struct array3_t
{
  barray *bare;
  bitvec *index;
  bitvec *pindex;
};
/* from funcsat: */
extern unsigned int fsVarHash(void *);
extern int fsVarEq(void *, void *);

void addAllCongruenceClauses(bfman *m);


static literal bf_aiger2lit(aiger_literal l) {
  literal r;
  if (l == aiger_false) r = LITERAL_FALSE;
  else if (l == aiger_true) r = LITERAL_TRUE;
  else {
    if (aiger_sign(l)) {
      r = -(literal)aiger_lit2var(l);
    } else {
      r = (literal)aiger_lit2var(l);
    }
  }
  //printf("converting aig lit %d to funcsat list %" PRIdMAX "\n", l, r);
  return r;
}


static aiger_literal bf_lit2aiger(literal l)
{
  if (l == LITERAL_FALSE) return aiger_false;
  if (l == LITERAL_TRUE) return aiger_true;
  if (l < 0) {
    return aiger_not(aiger_var2lit(-l));
  } else {
    return aiger_var2lit(l);
  }
}

static void clauseInsertX(clause *c, literal l)
{
  if (!bf_litIsFalse(l) && !bf_litIsTrue(l)) {
    clausePush(c, l);
    //clauseInsert(c, l);
  }
}

static void add_clause_to_AIG(bfman *m, bitvec *c)
{
  literal *l;
  literal out = LITERAL_FALSE;
  forBv (l, c) {
    if (*l == LITERAL_TRUE) {
      return;
    } else if (*l != LITERAL_FALSE) {
      if (out == LITERAL_FALSE) {
        out = -*l;
      } else {
        literal new_out = (literal) ++m->numVars; /* create fresh var not an input */
        m->addAnd(m, new_out, -out, -*l);
        out = -new_out;
      }
    }
  }
  CONSUME_BITVEC(c);
  m->addAIGOutput(m, out);
}

static void addClauseFs(bfman *m, clause *c, void *UNUSED(ignore))
{
  clause *dup = clauseAlloc(c->size);
  literal *l;
  forClause(l, c) {
    if (*l == LITERAL_TRUE) {
      clauseFree(dup);
      return;
    } else if (*l != LITERAL_FALSE) {
      clausePush(dup, *l);
      //clauseInsert(dup, *l); 
    }
  }
#ifndef NDEBUG
  forClause(l, dup) {
    assert(*l != LITERAL_TRUE);
    assert(*l != LITERAL_FALSE);
  }
#endif
  funcsatAddClause(m->funcsat, dup);
}

extern funcsat_result funcsatAddClause(funcsat *, clause *);


#endif
