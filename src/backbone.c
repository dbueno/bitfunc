/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <strings.h>

#include <bitfunc.h>
#include <bitfunc/backbone.h>
#include <funcsat/hashtable.h>

/* BROKEN AT THE MOMENT */


/*
Computing backbones.

A literal $x$ is a \textit{backbone} literal of formula $\phi$ if it has the
same assignment in every model of $\phi$ \cite{Marques-Silva+:FAIA10}. This is
the same as saying that $x$ is a backbone if $\phi \wedge x$ is UNSAT.

*/

static unsigned hashvar(void *k)
{
  variable u = *(variable *) k;
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

static int litEqual(void *k1, void *k2)
{
  literal *l1 = (literal *) k1;
  literal *l2 = (literal *) k2;
  return *l1 == *l2;
}

DEFINE_HASHTABLE(bflpInsert, bflpSearch, bflpRemove, literal, unsigned)


typedef struct litpos_
{
  struct hashtable *table;
  bitvec *b;
} litpos;

static litpos *lpInit(bitvec *b)
{
  litpos *r;
  MallocX(r, sizeof(*r), 1);
  r->b = b;
  r->table = create_hashtable(17, hashvar, litEqual);
  return r;
}

static literal *lpPos(litpos *s, literal l)
{
  literal *p = &l;
  unsigned *r;
  if (NULL != (r = bflpSearch(s->table, (void *)p))) {
    return s->b->data + *r;
  }
  
  return NULL;
}

static bool lpRemove(litpos *s, literal l)
{
  literal *k = lpPos(s, l);
  if (k) {
    unsigned *v = bflpRemove(s->table, k);
    free(v);
    return true;
  }
  return false;
}

static void lpAdd(litpos *s, literal l, unsigned i)
{
  literal   *k;
  unsigned *v;
  MallocX(k, sizeof(*k), 1);
  MallocX(v, sizeof(*v), 1);
  *k = l;
  *v = i;
  assert(NULL == bflpSearch(s->table, (void *)k));
  bflpInsert(s->table, (void *)k, (void *)v);
}

static void lpDestroy(litpos *s)
{
  hashtable_destroy(s->table, true, true);
  free(s);
}


/*
how can we reduce the model.

clear model out. clear litpos. stash the model.

1. walk through the model, pushing literals. If any literal is not needed to
satisfy any clause, do not add it to the model.

2. (todo) set covering. find (greedy) minimal model that satisfies all
clauses. associate each variable with the clauses it satisfies.
*/
static inline void reduceModel(bfman *b, litpos *model)
{
}

bitvec *computeBackbone(bfman *b)
{
  bfresult issat;
  bitvec *vars = bfvAlloc(b, b->numVars+1); /* vars 1-numVars @ same index */
  bitvec *ref = bfvAlloc(b, b->numVars); /* reference assignment */
  litpos *refPos = lpInit(ref);
  bitvec *bb = bfvAlloc(b, vars->size);

  issat = bfSolve(b);
  if (issat == BF_UNSAT) {
    bfvDestroy(bb);
    bb = NULL;
    goto Done;
  }

  reduceModel(b, refPos);

  literal *nextVar;
  forBv (nextVar, vars) {
    if (*nextVar == 0) continue;
    if (!lpPos(refPos, *nextVar)) continue;
    /* negate literal assignment for variable *nextVar */
    literal l = -bflFromCounterExample(b, *nextVar);
    bfPushAssumption(b, l);
    issat = bfSolve(b);
    if (issat == BF_UNSAT) {
      bfvPush(bb, -l); /* backbone identified */
      bfPopAssumptions(b, 1);
      if (b->assumps->size == 0) bfSet(b, -l);
      /* if (f->assumptions.size==0) trailPush(f, -*nextVar, NULL); */
      continue;
    }
    bfPopAssumptions(b, 1);

    reduceModel(b, refPos);
    for (literal x = 1; x <= (int)b->numVars; x++) {
      if (!lpPos(refPos, x) && !lpPos(refPos, -x)) {
        /* variable filtering */
        /* literal *lInVars = lpPos(varsPos, *lIt); */
        /* *lInVars = 0; */
        /* lInVars = lpPos(varsPos, -*lIt); */
        /* *lInVars = 0; */
      }
    }
    literal *lIt;
    forBv (lIt, refPos->b) {
      literal *lInVars;
      /* if ((lInVars = lpPos(varsPos, *lIt))) { */
      /*   /\* variable filtering *\/ */
      /*   *lInVars = 0; */
      /* } */
    }
  }

Done:
  bfPopAssumptions(b, b->assumps->size);
  /* lpDestroy(varsPos); */
  lpDestroy(refPos);
  bfvDestroy(ref);
  
  return bb;

}
