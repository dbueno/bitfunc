/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <bitfunc.h>
#include <bitfunc/bitvec.h>
#include <bitfunc/circuits.h>
#include <bitfunc/debug.h>
#include <math.h>
#include <string.h>

#include "bfprobes.h"

#define UNUSED(x) (void)(x)

extern inline unsigned hashBv(void *k)
{
  bitvec *bv = k;
  unsigned int hash = 0;
  unsigned int b = 378551, a = 63689;
  literal *p;
  forBv(p, bv) {
    hash = hash * a + (unsigned)*p;
    a = a * b;
  }
  return hash;
}

extern inline int bvEqual(void *k1, void *k2)
{
  bitvec *b1 = k1, *b2 = k2;
  if (b1 == b2) return true;
  if (b1->size == b2->size) {
    for (unsigned i = 0; i < b1->size; i++) {
      if (b1->data[i] == b2->data[i]) continue;
      else return false;
    }
    return true;
  }
  return false;
}

/* circuits.c */
extern literal bfAuxVar(bfman *m);

/* Emit definitions to object file */
extern bitvec *bfvZextend(bfman *m, uint32_t bfNew_width, bitvec *v);
extern bitvec *bfvSextend(bfman *man, uint32_t new_width, bitvec *v);
extern void CONSUME_BITVEC(bitvec *vec);
extern bitvec *bfvHold(bitvec *v);
extern bitvec *bfvRelease(bitvec *v);
extern void bfvReleaseConsume(bitvec *vec);
extern bitvec *bfvLRshift(bfman *m, bitvec *val, bitvec *dist);
extern bitvec *bfvARshift(bfman *m, bitvec *val, bitvec *dist);
extern bitvec *bfvQuot(bfman *m, bitvec *x, bitvec *y);
extern bitvec *bfvDiv(bfman *m, bitvec *x, bitvec *y);
extern bitvec *bfvRem(bfman *m, bitvec *x, bitvec *y);


bitvec *bfvInit(bfman *man, uint32_t size)
{
  bitvec *rbv = bfvAlloc(size);

  for (uint32_t i = 0; i < size; i++) {
    bfvPush(rbv, bfNewVar(man));
  }

  return rbv;
}

bitvec *bfvAlloc(uint32_t initialCap)
{
  bitvec *v;
  MallocX(v, 1, sizeof(*v));
  if(initialCap == 0) initialCap = 4;
  CallocX(v->data, initialCap, sizeof(*v->data));
  v->size = 0;
  v->capacity = initialCap;
  v->numHolds = 0;
  v->name = NULL;
  PROBE(probe_counter_inc1(&bitvec_count));
  return v;
}

bitvec *bfvSetName(bfman *b, bitvec *x, const char *str)
{
  char *n = strdup(str);
  if (!n) abort();
  free(x->name);
  x->name = n;
  bf_IfDebug (b, "varmap", 1) {
    bf_log(b, "varmap", 1, "%s = ", x->name);
    bfvPrint(b->dout, x);
    fprintf(b->dout, "\n");
  }
  char *buf = calloc(strlen(x->name) + log2(x->size)+1 + 3, sizeof(char));
  for (unsigned i = 0; i < x->size; i++) {
    sprintf(buf, "%s[%u]", x->name, i);
    bfSetName(b, x->data[i], buf);
  }
  free(buf);
  return x;
}

void bfvPush(bitvec *v, literal data)
{
  if (v->capacity <= v->size) {
    while (v->capacity <= v->size) {
      v->capacity = v->capacity*2+1;
    }
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  v->data[v->size++] = data;
}

literal bfvPop(bitvec *v)
{
  assert(v->size != 0);
  return v->data[v->size-- - 1];
}

void bfvGrowTo(bitvec *v, uint32_t newCapacity)
{
  if (v->capacity < newCapacity) {
    v->capacity = newCapacity;
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  assert(v->capacity >= newCapacity);
}

void bfvClear(bitvec *v)
{
  v->size = 0;
}

void bfvDestroy(bitvec *v)
{
  assert(v->numHolds==0);
  free(v->data);
  v->data = NULL;
  v->size = 0;
  v->capacity = 0;
  free(v->name);
  free(v);
  PROBE(probe_counter_dec1(&bitvec_count));
}

void bflPrint(FILE *stream, literal l)
{
  if (bf_litIsTrue(l)) fprintf(stream, "TRUE");
  else if (bf_litIsFalse(l)) fprintf(stream, "FALSE");
  else fprintf(stream, "%i", l);
}

void bfvPrintConcrete(bfman *m, FILE *stream, bitvec *b, bfvprint_mode mode)
{
  if (!stream) stream = stderr;
  switch (mode) {
  case PRINT_BINARY: {
    literal *l;
    uint32_t cnt = 0;
    forBvRev(l,b) {
      mbool val = bfGet(m,*l);
      fprintf(stream, "%s", val == true ? "1" : (val == false ? "0" : "?"));
      if ((++cnt % 4) == 0) fprintf(stream, " ");
    }
    break;
  }

  case PRINT_HEX: {
    // i counts # bytes
    bitvec *copy = bfvDup(m, bfvHold(b));
    bfvRelease(b);
    while ((copy->size % 8) != 0) {
      copy = bfvZextend(m, (uint32_t) copy->size+1, copy);
    }
    for (int i = (copy->size/8)-1; i >= 0; i--) {
      uint32_t byte = 0;
      for (int64_t j = 0; j < 8; j++) {
        literal l = copy->data[(i*8)+j];
        uint8_t val;
        mbool v = bfGet(m,l);
        assert(v != unknown);
        val = mbool2int(v);
        byte |= (uint32_t) (val << j);
      }
      fprintf(stream, "%02x", byte);
    }
    CONSUME_BITVEC(copy);
    break;
  }
  }
}

void bfvPrint(FILE *stream, bitvec *b)
{
  if (!stream) stream = stderr;
  literal *l;
  fprintf(stream, "[");
  bool first = true;
  forBv(l,b) {
    if (first) first = false;
    else fprintf(stream, " ");
    if (*l == LITERAL_TRUE) fprintf(stream, "TRUE");
    else if (*l == LITERAL_FALSE) fprintf(stream, "FALSE");
    else fprintf(stream, "%i", *l);
  }
  fprintf(stream, "]");
}

mbool bfvSame(bitvec *b1, bitvec *b2)
{
  mbool ret = true;
  if (b1 == b2) return true;
  if (b1->size == b2->size) {
    for (unsigned i = 0; i < b1->size; i++) {
      if (b1->data[i] == -b2->data[i]) {
        return false;
      } else if (b1->data[i] == b2->data[i]) {
        continue;
      } else {
        ret = unknown;
      }
    }
    return ret;
  }
  return false;
}

bitvec *bfvZero(const uint32_t sz)
{
  bitvec *ret = bfvAlloc(sz);
  ret->size = sz;
  for (uint32_t i = 0; i < sz; i++) {
    ret->data[i] = LITERAL_FALSE;
  }
  return ret;  
}

bitvec *bfvOne(const uint32_t sz)
{
  bitvec *ret = bfvAlloc(sz);
  ret->data[0] = LITERAL_TRUE;
  ret->size = sz;
  for (uint32_t i = 1; i < sz; i++) {
    ret->data[i] = LITERAL_FALSE;
  }
  return ret;  
}

bitvec *bfvOnes(const uint32_t sz)
{
  bitvec *ret = bfvAlloc(sz);
  ret->size = sz;
  for (uint32_t i = 0; i < sz; i++) {
    ret->data[i] = LITERAL_TRUE;
  }
  return ret;
}

bool bfvIsConstant(bitvec *x)
{
  literal *l;
  forBv(l, x) {
    if (!bf_litIsConst(*l)) return false;
  }
  return true;
}

bitvec *bfvIncr(bfman *b, bitvec *x)
{
  return bfvAdd0(b, x, bfvUconstant(b, x->size, 1));
}

bitvec *bfvAnd(bfman *bf, bitvec *x, bitvec *y)
{
  INPUTCHECK(bf, x->size == y->size, "x and y are not the same size");
  bitvec *ret = bfvAlloc(x->size); ret->size = x->size;
  for (uint32_t i = 0; i < x->size; i++) {
    ret->data[i] = bfNewAnd(bf, x->data[i], y->data[i]);
  }

  CONSUME_BITVEC(x);
  if (x != y) CONSUME_BITVEC(y);
  
  return ret;
}

bitvec *bfvOr(bfman *bf, bitvec *x, bitvec *y)
{
  INPUTCHECK(bf, x->size == y->size, "x and y are not the same size");
  bitvec *ret = bfvAlloc(x->size); ret->size = x->size;
  for (uint32_t i = 0; i < x->size; i++) {
    ret->data[i] = bfNewOr(bf, x->data[i], y->data[i]);
  }

  CONSUME_BITVEC(x);
  if (x != y) CONSUME_BITVEC(y);

  return ret;
}

bitvec *bfvXor(bfman *m, bitvec *x, bitvec *y)
{
  INPUTCHECK(m, x->size == y->size, "x and y are not the same size");
  bitvec *ret = bfvAlloc(x->size); ret->size = x->size;
  for (uint32_t i = 0; i < x->size; i++) {
    ret->data[i] = bfNewXor(m, x->data[i], y->data[i]);
  }

  CONSUME_BITVEC(x);
  if (x != y)
    CONSUME_BITVEC(y);

  return ret;
}

bitvec *bfvExtend(bfman *man, uint32_t new_width, bitvec *v, extendty ty)
{
  UNUSED(man);
  INPUTCHECK(man, new_width >= v->size, "new_width must be at least the current size");
  bitvec *ret = bfvAlloc(new_width);

  for (uint32_t i = 0; i < v->size; i++) {
    bfvPush(ret, v->data[i]);
  }
  switch (ty) {
  case EXTEND_ZERO:
    for (uint32_t i = v->size; i < new_width; i++) {
      bfvPush(ret, LITERAL_FALSE);
    }
  break;
  case EXTEND_SIGN:
  for (uint32_t i = v->size; i < new_width; i++) {
    bfvPush(ret, v->data[v->size-1]);
  }
  break;
  default:
    assert(0 && "unrecognised extension type");
  }
  assert(new_width == ret->size);

  CONSUME_BITVEC(v);

  return ret;
}

bitvec *bfvTruncate(bfman *man, uint32_t newWidth, bitvec *v)
{
  UNUSED(man);
  INPUTCHECK(man, newWidth <= v->size, "newWidth must be at most the current size");
  bitvec *ret = bfvAlloc(newWidth);
  for (uint32_t i = 0; i < newWidth; i++) {
    bfvPush(ret, v->data[i]);
  }

  CONSUME_BITVEC(v);

  return ret;
}



bitvec *bfvNegate(bfman *man, bitvec *x)
{
  bitvec *ret = bfvAlloc(x->size);
  ret->size = x->size;
  literal invert = LITERAL_FALSE;

  for (uint32_t i = 0; i < x->size; i++) {
    ret->data[i] = bfNewSelect(man, invert, -x->data[i], x->data[i]);
    invert = bfNewOr(man, invert, x->data[i]);
  }

  CONSUME_BITVEC(x);

  return ret;
}

bitvec *bfvAbsoluteValue(bfman *man, bitvec *x)
{
  bitvec *ret;

  bfvHold(x);
  literal sign = x->data[x->size-1];
  bitvec *x_inv = bfvNegate(man, x);
  
  ret = bfvSelect(man, sign, x_inv, x);
  
  CONSUME_BITVEC(bfvRelease(x));
  
  return ret;
}

bitvec *bfvInvert(bfman *man, bitvec *x)
{
  UNUSED(man);
  bitvec *ret = bfvAlloc(x->size);
  ret->size = x->size;
  for (uint32_t i = 0; i < x->size; i++) {
    ret->data[i] = -x->data[i];
  }

  CONSUME_BITVEC(x);

  return ret;
}


literal bfvUlt(bfman *man, bitvec *x, bitvec *y)
{
  INPUTCHECK(man, x->size == y->size, "x and y are not the same size");
  bitvec *biteqs = bfvHold(bfvAlloc((uint32_t) x->size*2)); 
  assert(biteqs->numHolds > 0);
  bitvec *ltp = bfvAlloc(x->size);
  for (uint32_t i = x->size-1; i != (uint32_t)~0; i--) {
    bfvPush(ltp, bfNewAnd(man, bfBigAnd(man, biteqs), bfNewAnd(man, y->data[i], -x->data[i])));
    bfvPush(biteqs, bfNewEqual(man, -x->data[i], -y->data[i]));
  }

  CONSUME_BITVEC(bfvRelease(biteqs));
  CONSUME_BITVEC(x);
  if (x != y) CONSUME_BITVEC(y);

  return bfBigOr(man, ltp);
}


literal bfvUlte(bfman *man, bitvec *x, bitvec *y)
{
  INPUTCHECK(man, x->size == y->size, "x and y are not the same size");
  bfvHold(x);
  bfvHold(y);
  literal lit = bfNewOr(man, bfvUlt(man, x, y), bfvEqual(man, x, y));
  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));
  return lit;
}


literal bfvSlt(bfman *man, bitvec *x, bitvec *y)
{
  INPUTCHECK(man, x->size == y->size, "x and y are not the same size");

  literal xsign = x->data[x->size-1], ysign = y->data[x->size-1];
  return bfNewOr(man, bfNewAnd(man, xsign, -ysign),
                      bfNewAnd(man, bfNewEqual(man, xsign, ysign), bfvUlt(man, x, y)));
}

literal bfvSlte(bfman *man, bitvec *x, bitvec *y)
{
  INPUTCHECK(man, x->size == y->size, "x and y are not the same size");
  bfvHold(x);
  bfvHold(y);
  literal lit = bfNewOr(man, bfvSlt(man, x, y), bfvEqual(man, x, y));
  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));
  return lit;
}

literal bfvSltZero(bfman *man, bitvec *x)
{
  UNUSED(man);
  // duh, but from hacker's delight
  literal lit = x->data[x->size-1];
  CONSUME_BITVEC(x);
  return lit;
}

literal bfvSgtZero(bfman *m, bitvec *x)
{
  bfvHold(x);
  // hacker's delight
  bitvec *sub = bfvSubtract(m, bfvRshiftConst(m, x, 1, x->data[x->size-1]), x);
  literal lit = sub->data[x->size-1];
  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(sub);
  return lit;
}

literal bfvSgteZero(bfman *man, bitvec *x)
{
  UNUSED(man);
  // hacker's delight
  literal lit = -x->data[x->size-1];
  CONSUME_BITVEC(x);
  return lit;
}


// returns the result vector and carry out
bitvec *bfvAdd(bfman *man, bitvec *x, bitvec *y, literal cin, literal *cout)
{
  INPUTCHECK(man, x->size == y->size, "x and y are not the same size");
  bitvec *result = bfvAlloc(x->size);
  result->size = x->size;
  literal carry = cin;
  for (uint32_t i = 0; i < x->size; i++) {
    result->data[i] = bfNewXor(man, carry, bfNewXor(man, x->data[i], y->data[i]));
    carry = bfNewSelect(man, carry, bfNewOr(man, x->data[i], y->data[i]), bfNewAnd(man, x->data[i], y->data[i]));
  }
  if (cout) *cout = carry;

  CONSUME_BITVEC(x);
  if (x != y) CONSUME_BITVEC(y);

  return result;
}


bitvec *bfvSubtract(bfman *man, bitvec *x, bitvec *y)
{
  INPUTCHECK(man, x->size == y->size, "x and y are not the same size");
  bitvec *negy = bfvInvert(man, y);
  bitvec *result = bfvAdd(man, x, negy, LITERAL_TRUE, NULL);
  return result;
}


// returns a literal that's true iff the input bit vectors are elementwise equal
literal bfvEqual(bfman *man, bitvec *x, bitvec *y)
{
  INPUTCHECK(man, x->size == y->size, "x and y are not the same size");
  bfvHold(x), bfvHold(y);
  if (x == y) { 
    CONSUME_BITVEC(bfvRelease(x));
    CONSUME_BITVEC(bfvRelease(y));
    return LITERAL_TRUE;
  }
  uint32_t l = 0;
  for (; l < x->size; ++l) {
    if (x->data[l] == -y->data[l]) {
      CONSUME_BITVEC(bfvRelease(x));
      CONSUME_BITVEC(bfvRelease(y));
      return LITERAL_FALSE;
    }
    if (x->data[l] != y->data[l]) {
      break;
    }
  }
  if (l==x->size) {
    CONSUME_BITVEC(bfvRelease(x));
    CONSUME_BITVEC(bfvRelease(y));
    return LITERAL_TRUE;
  }
  bitvec *equalities = bfvAlloc(x->size - l);

  for (; l < x->size; l++) {
    bfvPush(equalities, bfNewEqual(man, x->data[l], y->data[l]));
  }

  literal ret = bfBigAnd(man, equalities);

  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));

  return ret;
}

bitvec *bfvSconstant(bfman *man, uint32_t width, int value)
{
  unsigned absValue = value < 0 ? (unsigned)-value : (unsigned)value;
  bitvec *bv = bfvAlloc(width);
  // set number bits
  while (absValue != 0) {
    bfvPush(bv, absValue & 1 ? LITERAL_TRUE : LITERAL_FALSE);
    absValue >>= 1;

    // ... or we could truncate `absValue'
    INPUTCHECK(man, bv->size <= width, "given width too small");
  }

  while (bv->size != width) {
    bfvPush(bv, LITERAL_FALSE);
  }

  if (value < 0) {
    bv = bfvNegate(man, bv);
  }
  
  assert(bv->size == width);
  return bv;
}

bitvec *bfvUconstant(bfman *man, uint32_t width, unsigned value)
{
  UNUSED(man);
  bitvec *bv = bfvAlloc(width);
  // set number bits
  while (value != 0) {
    bfvPush(bv, value & 1 ? LITERAL_TRUE : LITERAL_FALSE);
    value >>= 1;

    // ... or we could truncate `absValue'
    INPUTCHECK(man, bv->size <= width, "given width too small");
  }

  while (bv->size != width) {
    bfvPush(bv, LITERAL_FALSE);
  }

  assert(bv->size == width);
  return bv;
}

static const int MultiplyDeBruijnBitPosition2[32] = 
{
  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

static inline int ilog2(uint32_t v) {
  return MultiplyDeBruijnBitPosition2[(uint32_t)(v * 0x077CB531U) >> 27];
}

bitvec *bfvLshiftConst(bfman *man, bitvec *bv, uint32_t dist)
{
  UNUSED(man);
  const uint32_t size = bv->size;
  bitvec *ret = bfvAlloc(size);
  /* XXX - what is this doing here? -gr */
  ret->size = size;

  // only as necessary to fill with zeros
  if (dist > bv->size) {
    dist = bv->size;
  }

  // fill bottom with zeros
  for (uint32_t i = 0; i < dist; i++) {
    ret->data[i] = LITERAL_FALSE;
  }

  // shift rest of the bits
  for (uint32_t i = dist; i < size; i++) {
    ret->data[i] = bv->data[i-dist];
  }

  CONSUME_BITVEC(bv);

  return ret;
}


static bitvec *leftBarrelShift(bfman *man, bitvec *val, bitvec *dist, int32_t s)
{
  bitvec *ret = bfvAlloc(val->size);
  ret->size = val->size;

  bfvHold(val);
  bfvHold(dist);

  if (s == -1) {
    for (uint32_t i = 0; i < val->size; i++) {
      ret->data[i] = val->data[i];
    }
  } else {
    bitvec *sub = leftBarrelShift(man, val, dist, s-1);
    for (uint32_t i = 0; i < val->size; i++) {
      uint32_t two_s = 1 << s;
      if (i >= two_s) {
        ret->data[i] = bfNewSelect(man,
                                   dist->data[s],
                               sub->data[i - two_s],
                               sub->data[i]);
      } else {
        ret->data[i] = bfNewSelect(man,
                                   dist->data[s],
                               LITERAL_FALSE,
                               sub->data[i]);
      }
    }
    CONSUME_BITVEC(sub);
  }

  CONSUME_BITVEC(bfvRelease(val));
  CONSUME_BITVEC(bfvRelease(dist));

  return ret;
}

bitvec *bfvLshift(bfman *man, bitvec *val, bitvec *dist)
{
  uint32_t l = val->size;
  INPUTCHECK(man, (l & (l-1)) == 0, "val size not a power of two");
#if 0
  assert((double) dist->size == log2((double) l)
         && "shift distance not log2(val-->size)");
#endif

  literal of = LITERAL_FALSE;
  uint32_t shiftsize = ilog2(val->size);
  if (shiftsize >= dist->size) {
    shiftsize = dist->size;
  } else {
    for (uint32_t i = shiftsize; i < dist->size; ++i) {
      of = bfNewOr(man, of, dist->data[i]);
    }
  }

  if (bf_litIsTrue(of)) {
    bitvec *ret = bfvUconstant(man, val->size, 0);
    return ret;
  } else {
    bitvec *shifted = leftBarrelShift(man, val, dist, ((int32_t)shiftsize)-1);
    if (!bf_litIsFalse(of)) {
      for (uint32_t i = 0; i < shifted->size; ++i) {
        shifted->data[i] = bfNewSelect(man, of, LITERAL_FALSE, shifted->data[i]);
      }
    }
    return shifted;
  }
}


bitvec *bfvRshiftConst(bfman *man, bitvec *bv, uint32_t dist, literal fill)
{
  UNUSED(man);
  const uint32_t size = bv->size;
  bitvec *ret = bfvAlloc(size);
  /* XXX - what is this doing here? -gr */
  ret->size = size;

  // only as necessary to fill with zeros
  if (dist > bv->size) {
    dist = bv->size;
  }

  // fill top with filler
  for (uint32_t i = size - dist; i < size; i++) {
    ret->data[i] = fill;
  }

  // shift rest of the bits
  for (uint32_t i = 0; i < size - dist; i++) {
    ret->data[i] = bv->data[i+dist];
  }

  CONSUME_BITVEC(bv);

  return ret;
}

static bitvec *rightBarrelShift(bfman *man, bitvec *val, bitvec *dist, int32_t s, literal fill)
{
  bitvec *ret = bfvAlloc(val->size);
  ret->size = val->size;

  bfvHold(val);
  bfvHold(dist);

  if (s == -1) {
    for (uint32_t i = 0; i < val->size; i++) {
      ret->data[i] = val->data[i];
    }
  } else {
    bitvec *sub = rightBarrelShift(man, val, dist, s-1, fill);
    for (uint32_t i = 0; i < val->size; i++) {
      uint32_t two_s = 1 << s;
      if (i < (val->size - two_s)) {
        ret->data[i] = bfNewSelect(man,dist->data[s],
                            sub->data[i + two_s],
                            sub->data[i]);
      } else {
        ret->data[i] = bfNewSelect(man,dist->data[s],
                            fill,
                            sub->data[i]);
      }
    }
    CONSUME_BITVEC(sub);
  }

  CONSUME_BITVEC(bfvRelease(val));
  CONSUME_BITVEC(bfvRelease(dist));

  return ret;
}

bitvec *bfvRshift(bfman *man, bitvec *val, bitvec *dist, literal fill)
{
  uint32_t l = val->size;
  INPUTCHECK(man, (l & (l-1)) == 0, "val size not a power of two");
#if 0
  assert((double) dist->size == log2((double) l)
         && "shift distance not log2(val->size)");
#endif

  literal of = LITERAL_FALSE;
  uint32_t shiftsize = ilog2(val->size);
  if (shiftsize >= dist->size) {
    shiftsize = dist->size;
  } else {
    for (uint32_t i = shiftsize; i < dist->size; ++i) {
      of = bfNewOr(man, of, dist->data[i]);
    }
  }

  if (bf_litIsTrue(of)) {
    bitvec *ret = bfvAlloc(val->size);
    ret->size = val->size;
    for (uint32_t i = 0; i < ret->size; ++i) {
      ret->data[i] = fill;
    }
    return ret;
  } else {
    bitvec *shifted = rightBarrelShift(man, val, dist, ((int32_t)shiftsize)-1, fill);
    if (!bf_litIsFalse(of)) {
      for (uint32_t i = 0; i < shifted->size; ++i) {
        shifted->data[i] = bfNewSelect(man, of, fill, shifted->data[i]);
      }
    }
    return shifted;
  }
}


bitvec *bfvSelect(bfman *man, literal t, bitvec *thn, bitvec *els)
{
  INPUTCHECK(man, thn->size == els->size, "thn and els not the same size");
  bitvec *ret = bfvAlloc(thn->size); ret->size = thn->size;
  for (uint32_t i = 0; i < thn->size; i++) {
    ret->data[i] = bfNewSelect(man, t, thn->data[i], els->data[i]);
  }

  CONSUME_BITVEC(thn);
  CONSUME_BITVEC(els);

  return ret;
}


bitvec *bfvMul(bfman *man, bitvec *x, bitvec *y)
{
  INPUTCHECK(man, x->size == y->size, "x and y are not the same size");
  bfvHold(x), bfvHold(y);
  
  bitvec *ret = bfvConstant(man, y->size, 0, false);
  bitvec *tmp = bfvAlloc(ret->size); tmp->size = ret->size;
  
  for (uint32_t j = 0; j < x->size; j++) {
    literal carry = LITERAL_FALSE; // initially false
    for (uint32_t i = 0; i < (x->size-j); i++) {
      tmp->data[i+j] = bfNewXor(man, carry, bfNewXor(man, x->data[i], ret->data[i+j]));
      // majority vote
      carry = bfNewSelect(man,
                          carry,
                      bfNewOr(man, x->data[i], ret->data[i+j]),
                      bfNewAnd(man, x->data[i], ret->data[i+j]));
    }

    for (uint32_t i = 0; i < (x->size-j); i++) {
      ret->data[i+j] = bfNewSelect(man, y->data[j], tmp->data[i+j], ret->data[i+j]);
    }
  }

  CONSUME_BITVEC(tmp);
  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));

  return ret;
}


void bfvCopy(bfman *man, bitvec *dst, bitvec *src)
{
  UNUSED(man);
  dst->size = 0;
  bfvGrowTo(dst, src->size);
  for (uint32_t i = 0; i < src->size; i++) {
    bfvPush(dst, src->data[i]);
  }
}

bitvec *bfvDup(bfman *b, bitvec *src)
{
  bitvec *dst = bfvAlloc(src->size);
  bfvCopy(b, dst, src);
  CONSUME_BITVEC(src);
  return dst;
}

bitvec *bfvSdiv(bfman *b, bitvec *x, bitvec *y)
{
  INPUTCHECK(b, x->size == y->size, "x and y not the same size");
  bitvec *output;
   
  bitvec *vec_zero = bfvUconstant(b, x->size, 0);
  literal zerotest, sign, sign0, sign1;
   
  bfvHold(vec_zero), bfvHold(y), bfvHold(x);
  zerotest = bfvEqual(b, y, vec_zero);
  if (bf_litIsTrue(zerotest)) {
    bfvRelease(vec_zero);
    CONSUME_BITVEC(bfvRelease(y));
    return vec_zero;
  }
   
  sign0 = x->data[x->size-1];
  sign1 = y->data[y->size-1];
  sign = bfNewXor(b, sign0, sign1);
   
  bitvec *x_abs = bfvAbsoluteValue(b, x);
  bitvec *y_abs = bfvAbsoluteValue(b, y);
  bitvec *quotient;
  bitvec *remainder;
  bfvQuotRem(b, x_abs, y_abs, &quotient, &remainder);
  CONSUME_BITVEC(remainder);
  bfvHold(quotient);
  bitvec *negated = bfvNegate(b, quotient);
  bfvRelease(quotient);
  bitvec *intermediate = bfvSelect(b, sign, negated, quotient);
  bfvRelease(vec_zero);
  output = bfvSelect(b, zerotest, vec_zero, intermediate);

  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));
   
  return output;   
}

bitvec *bfvSrem(bfman *b, bitvec *x, bitvec *y)
{
  INPUTCHECK(b, x->size == y->size, "x and y not the same size");
  bitvec *output;
   
  literal sign = x->data[x->size-1];
   
  bitvec *x_abs = bfvAbsoluteValue(b, x);
  bitvec *y_abs = bfvAbsoluteValue(b, y);
   
  bitvec *quotient;
  bitvec *remainder;
  bfvQuotRem(b, x_abs, y_abs, &quotient, &remainder);
  bfvHold(remainder);
  bitvec *negated = bfvNegate(b, remainder);
  bfvRelease(remainder);
   
  output = bfvSelect(b, sign, negated, remainder);
   
  return output;
}


void bfvQuotRem(bfman *man, bitvec *x, bitvec *y, bitvec **quot, bitvec **rem)
{
  INPUTCHECK(man, x->size == y->size, "x and y not the same size");
  bitvec *ret = *rem = bfvAlloc(x->size);
  bfvCopy(man, ret, x);
  *quot = bfvInit(man, x->size);
  bitvec *q = *quot;
  bitvec *tmp = bfvInit(man, x->size);
  q->size = ret->size = tmp->size = x->size;

  for (int32_t j = (int32_t)(x->size-1); j >= 0; j--) {
    literal known = LITERAL_FALSE;
    for (int32_t i = (int32_t)(x->size-1); i > (((int32_t) x->size-1) - j); i--) {
      known = bfNewOr(man, known, y->data[i]);
      if (known == LITERAL_TRUE) {
        break;
      }
    }
    q->data[j] = known;

    for (int32_t i = (int32_t)(x->size-1); i >= 0; i--) {
      if (known == LITERAL_TRUE) {
        break;
      }

      literal y_bit = (i >= j) ? y->data[i-j] : LITERAL_FALSE;
      q->data[j] = bfNewSelect(man, known, q->data[j], 
                                  bfNewAnd(man, y_bit, -ret->data[i]));
      known = bfNewOr(man, known, bfNewXor(man, y_bit, ret->data[i]));
    }

    q->data[j] = -q->data[j];

    if (q->data[j] == LITERAL_FALSE) {
      continue;
    }

    literal borrow = LITERAL_FALSE; // initially false
    for (int32_t i = 0; i < (int32_t) x->size; i++) {
      literal top_bit = bfNewSelect(man, borrow, -ret->data[i], ret->data[i]);
      literal y_bit = (i >= j) ? y->data[i-j] : LITERAL_FALSE;
      borrow = bfNewSelect(man, ret->data[i], bfNewAnd(man, borrow, y_bit), 
                           bfNewOr(man, borrow, y_bit));
      tmp->data[i] = bfNewXor(man, top_bit, y_bit);
    }

    if (q->data[j] == LITERAL_TRUE) {
      bfvCopy(man, ret, tmp);
    } else {
      for (int32_t i = 0; i < (int32_t) x->size; i++) {
        ret->data[i] = bfNewSelect(man, q->data[j], tmp->data[i], ret->data[i]);
      }
    }
  }


  CONSUME_BITVEC(tmp);
  CONSUME_BITVEC(x);
  if (x != y) CONSUME_BITVEC(y);

  /* return std::pair<bitvec, bitvec>(quot, ret); */

  // if (quotRem) {
  //     return quot;
  // }
  // return ret;
}

bitvec *bfvConcat(bfman *man, bitvec *x, bitvec *y)
{
  UNUSED(man);
  bfvHold(x), bfvHold(y);
  /* integer promotion rules are dumb */
  bitvec *res = bfvAlloc((uint32_t)x->size + y->size);

  for (unsigned i = 0; i < x->size; i++) {
    bfvPush(res, x->data[i]);
  }
  for (unsigned i = 0; i < y->size; i++) {
    bfvPush(res, y->data[i]);
  }

  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));

  return res;
}


bitvec *bfvConcat3(bfman *man, bitvec *x, bitvec *y, bitvec *z)
{
  UNUSED(man);
  bfvHold(x), bfvHold(y), bfvHold(z);
  /* integer promotion rules are dumb */
  bitvec *res = bfvAlloc((uint32_t)x->size + y->size + z->size);

  for (unsigned i = 0; i < x->size; i++) {
    bfvPush(res, x->data[i]);
  }
  for (unsigned i = 0; i < y->size; i++) {
    bfvPush(res, y->data[i]);
  }
  for (unsigned i = 0; i < z->size; i++) {
    bfvPush(res, z->data[i]);
  }

  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));
  CONSUME_BITVEC(bfvRelease(z));

  return res;
}

bitvec *bfvReverse(bfman *b, bitvec *x)
{
  UNUSED(b);
  bitvec *res = bfvAlloc(x->size);
  literal *p;
  forBvRev (p, x) bfvPush(res, *p);

  CONSUME_BITVEC(x);
  
  return res;
}


bitvec *bfvExtract(bfman *man, bitvec *x, uint32_t begin, uint32_t len)
{
  UNUSED(man);
  INPUTCHECK(man, !(x->size < begin), "begin larger than x->size");
#define min(x,y) ((x) <= (y) ? (x) : (y))
  bitvec *res = bfvAlloc(min(begin+len, x->size) - begin + 1);

  for (unsigned i = begin; i < min(begin+len, x->size); i++) {
    bfvPush(res, x->data[i]);
  }

  assert(res->size <= len);

  CONSUME_BITVEC(x);

  return res;
#undef min
}

 
/* unsigned int _rotr(const unsigned int value, int shift) { */
/*     if ((shift &= sizeof(value)*8 - 1) == 0) */
/*       return value; */
/*     return (value >> shift) | (value << (sizeof(value)*8 - shift)); */
/* } */

bitvec *bfvRrotate(bfman *bf, bitvec *x, bitvec *d)
{
  bitvec *result;
  bfvHold(x), bfvHold(d);
  d = bfvAnd(bf, d, bfvUconstant(bf, d->size, x->size-1));
  result = bfvSelect(bf, bfvEqual(bf, d, bfvZero(d->size)), x,
                     bfvOr(bf, bfvRshift(bf, x, d, LITERAL_FALSE),
                           bfvLshift(bf, x,
                                     bfvSubtract(bf, bfvUconstant(bf, d->size, x->size), d))));
  return result;
}

/*  unsigned int _rotl(const unsigned int value, int shift) { */
/*     if ((shift &= sizeof(value)*8 - 1) == 0) */
/*       return value; */
/*     return (value << shift) | (value >> (sizeof(value)*8 - shift)); */
/* } */

bitvec *bfvLrotate(bfman *bf, bitvec *x, bitvec *d)
{
  bitvec *result;
  bitvec *myd;
  bfvHold(x), bfvHold(d);
  myd = bfvHold(bfvAnd(bf, d, bfvUconstant(bf, d->size, x->size-1)));
  result = bfvSelect(bf, bfvEqual(bf, d, bfvZero(d->size)), x,
                     bfvOr(bf, bfvLshift(bf, x, d),
                           bfvRshift(bf, x,
                                     bfvSubtract(bf, bfvUconstant(bf, d->size, x->size), d),
                                     LITERAL_FALSE)));
  CONSUME_BITVEC(bfvRelease(myd));
  CONSUME_BITVEC(bfvRelease(d));
  CONSUME_BITVEC(bfvRelease(x));
  return result;
}
