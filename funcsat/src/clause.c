/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat/config.h>

#include "funcsat/types.h"
#include "funcsat/internal.h"
#include "funcsat/clause.h"

#include <string.h>

unsigned int fsLitHash(void *lIn)
{
  /* TODO get better hash function */
  literal l = *(literal *) lIn;
  return (unsigned int) l;
}
int fsLitEq(void *a, void *b) {
  literal p = *(literal *) a, q = *(literal *) b;
  return p == q;
}
unsigned int fsVarHash(void *lIn)
{
  /* TODO get better hash function */
  literal l = *(literal *) lIn;
  return (unsigned int) fs_lit2var(l);
}
int fsVarEq(void *a, void *b) {
  literal p = *(literal *) a, q = *(literal *) b;
  return fs_lit2var(p) == fs_lit2var(q);
}



int litCompare(const void *l1, const void *l2)
{
  literal x = *(literal *) l1, y = *(literal *) l2;
  if (fs_lit2var(x) != fs_lit2var(y)) {
    return fs_lit2var(x) < fs_lit2var(y) ? -1 : 1;
  } else {
    if (x == y) {
      return 0;
    } else {
      return x < y ? -1 : 1;
    }
  }
}


#if 0
int clauseCompare(const void *cp1, const void *cp2)
{
  const clause *c = *(clause **) cp1, *d = *(clause **) cp2;
  if (c->size != d->size) {
    return c->size < d->size ? -1 : 1;
  } else {
    /* lexicographically compare */
    uint32_t i;
    for (i = 0; i < c->size; i++) {
      int ret;
      if (0 != (ret = litCompare(&c->data[i], &d->data[i]))) {
        return ret;
      }
    }
    return 0;
  }
}
#endif


void sortClause(clause *clause)
{
  qsort(clause->data, clause->size, sizeof(*clause->data), litCompare);
}


literal findLiteral(literal p, clause *clause)
{
  literal min = 0, max = clause->size-1, mid = -1;
  int res = -1;                 /* comparison */
  while (!(res == 0 || min > max)) {
    mid = min + ((max-min) / 2);
    res = litCompare(&p, &clause->data[mid]);
    if (res > 0) {
      min = mid + 1;
    } else {
      max = mid - 1;
    }
  } 
  return res == 0 ? mid : -1;
}

literal findVariable(variable v, clause *clause)
{
  literal min = 0, max = clause->size-1, mid = -1;
  int res = -1;                 /* comparison */
  while (!(res == 0 || min > max)) {
    mid = min + ((max-min) / 2);
    if (v == fs_lit2var(clause->data[mid])) {
      res = 0;
    } else {
      literal p = fs_var2lit(v);
      res = litCompare(&p, &clause->data[mid]);
    }
    if (res > 0) {
      min = mid + 1;
    } else {
      max = mid - 1;
    }
  } 
  return res == 0 ? mid : -1;
}



/* Low level clause manipulations. */
clause *clauseAlloc(uint32_t capacity)
{
  clause *c;
  MallocX(c, 1, sizeof(*c));
  clauseInit(c, capacity);
  return c;
}

void clauseInit(clause *v, uint32_t capacity)
{
  uint32_t c = capacity > 0 ? capacity : 4;
  CallocX(v->data, c, sizeof(*v->data));
  v->size = 0;
  v->capacity = c;
  v->isLearnt = false;
  v->lbdScore = LBD_SCORE_MAX;
  v->next[0] = v;
  v->next[1] = v;
  v->prev[0] = v;
  v->prev[1] = v;
  v->inBinaryWatches = false;
  v->isReason = false;
  v->activity = 0.f;
  v->id = (unsigned)-1;
}

void clauseDestroyAll(clause *c, uint8_t i)
{
  if (c) {
    clause *end = c;
    c = c->next[i];
    while (c != end) {
      clause *next = c->next[i];
      clauseDestroy(c);
      c = next;
    }
    clauseDestroy(c);           /* end */
  }
}

void clauseFree(clause *v)
{
  clauseDestroy(v);
  free(v);
}

void clauseDestroy(clause *v)
{
  free(v->data);
  v->data = NULL;
  v->size = 0;
  v->capacity = 0;
  v->isLearnt = false;
  v->lbdScore = LBD_SCORE_MAX;
  v->next[0] = NULL;
  v->next[1] = NULL;
  v->prev[0] = NULL;
  v->prev[1] = NULL;
}

void clauseClear(clause *v)
{
  v->size = 0;
  v->lbdScore = LBD_SCORE_MAX;
  v->isLearnt = false;
}

void clausePush(clause *v, literal data)
{
  if (v->capacity <= v->size) {
    while (v->capacity <= v->size) {
      v->capacity = v->capacity*2+1;
    }
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  v->data[v->size++] = data;
}

void clausePushAt(clause *v, literal data, uint32_t i)
{
  uint32_t j;
  assert(i <= v->size);
  if (v->capacity <= v->size) {
    while (v->capacity <= v->size) {
      v->capacity = v->capacity*2+1;
    }
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  v->size++;
  for (j = v->size-(uint32_t)1; j > i; j--) {
    v->data[j] = v->data[j-1];
  }
  v->data[i] = data;
}


void clauseGrowTo(clause *v, uint32_t newCapacity)
{
  if (v->capacity < newCapacity) {
    v->capacity = newCapacity;
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  assert(v->capacity >= newCapacity);
}


literal clausePop(clause *v)
{
  assert(v->size != 0);
  return v->data[v->size-- - 1];
}

literal clausePopAt(clause *v, uint32_t i)
{
  uint32_t j;
  assert(v->size != 0);
  literal res = v->data[i];
  for (j = i; j < v->size-(uint32_t)1; j++) {
    v->data[j] = v->data[j+1];
  }
  v->size--;
  return res;
}

literal clausePeek(clause *v)
{
  assert(v->size != 0);
  return v->data[v->size-1];
}

void clauseSet(clause *v, uint32_t i, literal p)
{
  v->data[i] = p;
}

void clauseCopy(clause *dst, clause *src)
{
  literal i;
  for (i = 0; i < src->size; i++) {
    clausePush(dst, src->data[i]);
  }
  dst->isLearnt = src->isLearnt;
  dst->lbdScore = src->lbdScore;
  dst->next[0] = dst;
  dst->next[1] = dst;
  dst->prev[0] = dst;
  dst->prev[1] = dst;
#ifdef PROOFS
  dst->id = src->id;
#endif
}


/**************************************************************************/
/* clause list management */


void clauseSplice1(clause *new, clause **existing)
{
  if (new) {
    new->next[1] = new->prev[1] = NULL;
    if (!*existing) {
      *existing = new;
    } else {
      clause *next = new->next[0], *prev = (*existing)->prev[0];
      new->next[0] = (*existing);
      (*existing)->prev[0] = new;
      next->prev[0] = prev;
      prev->next[0] = next;
    }
  }
}



clause *clauseUnSplice(clause **n, uint8_t w)
{
  clause *ret = *n;
  if (clauseIsAlone(*n, w)) {
    *n = NULL;
  } else {
    clause *c = *n;
    /* p <-w- c -w-> m */
    /*  \-w->m p<-w-/ */
    clause *m = c->next[w], *p = c->prev[w];
    p->next[w] = m;
    m->prev[w] = p;
    *n = p;
    /* make c point only to itself. */
    c->next[w] = c->prev[w] = c;
  }
  return ret;
}

void clauseToFront(clause *n, uint8_t w, clause **front)
{
  assert(n);
  if (n != *front) {
    /*                  n---| */
    /*                      v */
    /* --> F -- X -- ... -- X [-- X -- ...] */
    /* F is the front, X is other nodes */
    clause *nAlias = n;
    clauseUnSpliceWatch(&nAlias, n, w);
    assert(clauseIsAlone(n,w));
    clauseSpliceWatchFront(n, w, front);
  }
}

void clauseSpliceWatch(clause *n, uint8_t w, clause **m)
{
  if (!*m) {
    n->next[w] = n->prev[w] = n;
    *m = n;
  } else {
    clause *c = *m;
    /* p <-w- n -w-> c */
    /*  \-y->c p<-x-/ */
    assert(c->data[0] == n->data[w] || c->data[1] == n->data[w]);
    const uint8_t x = c->data[0] == n->data[w] ? 0 : 1;
    assert(c->data[x] == n->data[w]);
    clause *p = c->prev[x];
    assert(p->data[0] == n->data[w] || p->data[1] == n->data[w]);
    const uint8_t y = p->data[0] == n->data[w] ? 0 : 1;
    assert(p->data[y] == n->data[w]);
    assert(p->next[y] == c);
    n->next[w] = c;
    n->prev[w] = p;
    c->prev[x] = n;
    p->next[y] = n;
  }
}

void clauseSpliceWatchFront(clause *n, uint8_t w, clause **m)
{
  clauseSpliceWatch(n,w,m);
  *m = n;
}

clause *clauseUnSpliceWatch(clause **n, clause *c, uint8_t w)
{
  clause *ret = c;
  if (clauseIsAlone(c, w)) {
    *n = NULL;
  } else {
    /* p <-w- c -w-> m */
    /*  \-y->m p<-x-/ */
    clause *m = c->next[w], *p = c->prev[w];
    assert(m->data[0] == c->data[w] || m->data[1] == c->data[w]);
    const uint8_t x = m->data[0] == c->data[w] ? 0 : 1;
    assert(p->data[0] == c->data[w] || p->data[1] == c->data[w]);
    const uint8_t y = p->data[0] == c->data[w] ? 0 : 1;
    p->next[y] = m;
    m->prev[x] = p;
    if (*n == c) *n = m;
    /* make c point only to itself. */
    c->next[w] = c->prev[w] = c;
  }
  return ret;
}

bool clauseIsAlone(clause *n, uint8_t w)
{
  return n->next[w] == n && n->prev[w] == n;
}

void clauseInitSentinel(clause *c)
{
  memset(c, 0, sizeof(*c));
  c->next[0] = c->prev[0] = c;
  c->next[1] = c->prev[1] = NULL;
}
