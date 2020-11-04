#include <bitfunc/config.h>

#include <assert.h>
#include <bitfunc.h>
#include <bitfunc/array.h>
#include <bitfunc/aiger.h>
#include <bitfunc/circuits.h>
#include <bitfunc/circuits_internal.h>
#include <funcsat.h>
#include <funcsat/hashtable.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "mitre.h"

/* #include "config.h" */

#if HAVE_PERFPROBES
#include <perfprobes/probes.h>
#include <perfprobes/stdiosampler.h>
#include <perfprobes/dirfilesampler.h>
#endif

/* ====================================================================== */
/* tests */


mitre *test_iff()
{
  mitre *m = mitre_init("test_iff", 1);

  literal a = bfNewVar(m->man);
  mitre_addInputLit(m, "a", a);
  literal b = bfNewVar(m->man);
  mitre_addInputLit(m, "b", b);
  // literal xor1  = p->lnot(p->lxor(a,b));
  literal output = bfNewEqual(
    m->man,
    -bfNewXor(m->man,a,b),
    bfNewAnd(m->man, bfNewImplies(m->man, b,a), bfNewImplies(m->man, a,b)));
  // p->l_set_to_true(miter);

  m->output = output;

  return m;
}

int testConstProp(const uint32_t width)
{
  printf("Running testConstProp %" PRIu32 "u ...\n", width);
  bfman *m = bfInit(AIG_MODE);

  bitvec *one = bfvConstant(m, width, 1, false);
  bitvec *one1 = bfvConstant(m, width, 1, false);
  bitvec *zero = bfvConstant(m, width, 0, false);
  bfvHold(one);
  literal trueLit = bfvEqual(m, one, one1);
  literal falseLit = bfvEqual(m, one, zero);
  CONSUME_BITVEC(bfvRelease(one));

  bfDestroy(m);
  return !(bf_litIsTrue(trueLit) && bf_litIsFalse(falseLit));
}

mitre *test_constants()
{
  uint32_t width = sizeof(long)*8;
  mitre *m = mitre_init("test_constants", width);

  long k = random(), v = random();
  bitvec *u = bfvHold(bfvConstant(m->man, width, (unsigned long)k, false));
  bitvec *u1 = bfvHold(bfvUconstant(m->man, width, (unsigned long)k));
  mitre_addInput(m, "u", u), mitre_addInput(m, "u1", u1);
  bitvec *s = bfvHold(bfvConstant(m->man, width, v, true));
  bitvec *s1 = bfvHold(bfvSconstant(m->man, width, v));
  mitre_addInput(m, "s", s), mitre_addInput(m, "s", s1);

  m->output = bf_bool2lit(true);
  m->output = bfNewAnd(m->man, m->output, bf_bool2lit(bfvGet(m->man, u) == (uint64_t) k));
  m->output = bfNewAnd(m->man, m->output, bf_bool2lit(bfvGet(m->man, s) == (uint64_t) v));
  assert(bf_litIsConst(m->output));

  bitvec *all = bfvAlloc(3);
  bfvPush(all, m->output);
  bfvPush(all, bfvEqual(m->man, u, u1));
  bfvPush(all, bfvEqual(m->man, s, s1));
  m->output = bfBigAnd(m->man, all);

  CONSUME_BITVEC(bfvRelease(u));
  CONSUME_BITVEC(bfvRelease(u1));
  CONSUME_BITVEC(bfvRelease(s));
  CONSUME_BITVEC(bfvRelease(s1));
  return m;
}

mitre *testExtension(uint32_t width)
{
  mitre *m = mitre_init("testExtension", width);
  bfman *b = m->man;
  
  bitvec *x = bfvHold(bfvInit(b, width)), *y = bfvHold(bfvInit(b, width));
  bitvec *zero = bfvHold(bfvUconstant(b, width, 0));
  
  /* x < 0 => sext(x, width*2) < 0 */
  m->output = bfNewSelect(b, bfvSlt(b, x, zero),
                          bfvSlt(b, bfvSextend(b, width*2, x), 
                                 bfvZextend(b, width*2, zero)),
                          LITERAL_TRUE);

  /* x > 0 => sext(x, width*2) > 0 */
  m->output = bfNewAnd(
    b,
    m->output,
    bfNewSelect(b, bfvSlt(b, zero, x),
                bfvSlt(b, bfvZextend(b, width*2, zero),
                       bfvSextend(b, width*2, x)),
                LITERAL_TRUE));

  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));
  CONSUME_BITVEC(bfvRelease(zero));
  return m;
}


/// x + 0 == x
mitre *test_add_0_is_identity(const uint32_t size)
{
  mitre *m = mitre_init("test_add_0_is_identity", size);
  bfman *b = m->man;

  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x);
  mitre_addInput(m, "y", y);

  literal *yi;
  // y is variables, but assumed to all be 0
  forBv(yi, y) bfPushAssumption(b, -*yi);

  literal cout = 0;
  bitvec *sum = bfvAdd(m->man, x, y, LITERAL_FALSE, &cout);
  assert(cout != 0);
  mitre_addInput(m, "s", sum);

  literal eqbv = bfvEqual(m->man, sum, x);
  mitre_addInputLit(m, "eqbv", eqbv);
  literal ca = bfNewEqual(m->man, cout, LITERAL_FALSE);
  mitre_addInputLit(m, "ca", ca);

  m->output = bfNewAnd(m->man, eqbv, ca);

  return m;
}

mitre *testBigAnd(const uint32_t size)
{
  mitre *m = mitre_init("testBigAnd", size);

  bitvec *x = bfvInit(m->man, size);

  literal naive = LITERAL_TRUE;
  literal *xi;
  forBv(xi, x) naive = bfNewAnd(m->man, naive, *xi);

  literal actual = bfBigAnd(m->man, x);
  
  m->output = bfNewEqual(m->man, naive, actual);

  return m;
}

mitre *testBigOr(const uint32_t size)
{
  mitre *m = mitre_init("testBigOr", size);

  bitvec *x = bfvInit(m->man, size);

  literal naive = LITERAL_FALSE;
  literal *xi;
  forBv(xi, x) naive = bfNewOr(m->man, naive, *xi);

  literal actual = bfBigOr(m->man, x);
  
  m->output = bfNewEqual(m->man, naive, actual);

  return m;
}

//TODO: make this useful (save literal value in some state for later prop_solve calls?)
int test_check_status_f()
{
  mitre *m = mitre_init("test_check_status_f", 1);
  printf("Running check_status_f ...\n");
  literal x = bfNewVar(m->man), y = bfNewVar(m->man);
  mitre_addInputLit(m, "x", x), mitre_addInputLit(m, "y", y);
  bfSetEqual(m->man, x, y);
  literal xr = bfNewXor(m->man, x, y);
  bfstatus status = bfCheckStatus(m->man, xr);
  mitre_destroy(m);
  return !(status == STATUS_MUST_BE_FALSE);
}

int test_check_status_t()
{
  mitre *m = mitre_init("test_check_status_t", 1);
  printf("Running check_status_t ...\n");
  literal x = bfNewVar(m->man), y = bfNewVar(m->man);
  mitre_addInputLit(m, "x", x), mitre_addInputLit(m, "y", y);
  bfSetEqual(m->man, x, y);
  literal xr = -bfNewXor(m->man, x, y);
  int status = bfCheckStatus(m->man, xr);
  mitre_destroy(m);
  return !(status == STATUS_MUST_BE_TRUE);
}



int test_check_status_tf()
{
  mitre *m = mitre_init("test_check_status_tf", 1);
  printf("Running check_status_tf ...\n");
  literal x = bfNewVar(m->man), y = bfNewVar(m->man);
  mitre_addInputLit(m, "x", x), mitre_addInputLit(m, "y", y);
  bfSetEqual(m->man, x, y);
  literal xr = bfNewAnd(m->man, x, y);
  int status = bfCheckStatus(m->man, xr);
  mitre_destroy(m);
  return !(status == STATUS_TRUE_OR_FALSE);
}



/// x + y == y + x
mitre *test_add_commutes(const uint32_t size)
{
  mitre *m = mitre_init("test_add_commutes", size);

  // x, y unconstrained bvs
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x);
  mitre_addInput(m, "y", y);

  literal cin = bfNewVar(m->man);
  literal c_xy, c_yx;
  bitvec *xy = bfvAdd(m->man, x, y, cin, &c_xy);
  bitvec *yx = bfvAdd(m->man, y, x, cin, &c_yx);

  literal eqbv = bfvEqual(m->man, xy, yx);
  literal ca = bfNewEqual(m->man, c_xy, c_yx);

  m->output = bfNewAnd(m->man, eqbv, ca);

  return m;
}

mitre *test_x_minus_x(const uint32_t width)
{
  mitre *m = mitre_init("test_x_minus_x", width);
  
  bitvec *x = bfvInit(m->man, width);
  mitre_addInput(m, "x", x);

  bitvec *difference = bfvSubtract(m->man, x, x);

  // test x - x = 0
  m->output = bfvEqual(m->man, bfvConstant(m->man, width, 0, true), difference);

  return m;
}

mitre *testAddSubInverse(const uint32_t size)
{
  mitre *m = mitre_init("testAddSubInverse", size);
  
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  /* top bit of x always 0 so negative result can always be represented */
  /* bfvPush(m->assumps, -x->data[width-1]); */

  bitvec *diff = bfvSubtract(m->man, x, y);
  bitvec *sum  = bfvAdd(m->man, x, bfvNegate(m->man, y), LITERAL_FALSE, NULL);
  mitre_addInput(m, "diff", diff), mitre_addInput(m, "sum", sum);

  m->output = bfvEqual(m->man, diff, sum);

  return m;
}

mitre *test_negate_is_invert_plus_1(const uint32_t width)
{
  mitre *m = mitre_init("test_negate_is_invert_plus_1", width);

  bitvec *x = bfvInit(m->man, width);
  mitre_addInput(m, "x", x);
  bitvec *sum = bfvAdd(m->man, bfvInvert(m->man, x), bfvConstant(m->man, width, 1, false), LITERAL_FALSE, NULL);
  bitvec *neg = bfvNegate(m->man, x);

  /* make sure inv(x)+1 = negate(x) */
  m->output = bfvEqual(m->man, neg, sum);

  return m;
}

mitre *test_lt_max(const uint32_t size)
{
  mitre *m = mitre_init("test_lt_max", size);
  bfman *b = m->man;
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  literal *p;
  forBv(p, y) bfPushAssumption(b, *p);

  m->output = bfNewXor(m->man, bfvEqual(m->man, x, y), bfvUlt(m->man, x, y));
                        
  return m;
}

mitre *test_slt_max(const uint32_t size)
{
  mitre *m = mitre_init("test_slt_max", size);
  bfman *b = m->man;
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "INT_MAX", y);

  for (uintptr_t i = 0; i < size-1; i++) {
    bfPushAssumption(b, y->data[i]);
  }
  bfPushAssumption(b, -y->data[size-1]);

  m->output = bfNewXor(m->man, bfvEqual(m->man, x, y),
                       bfvSlt(m->man, x, y));
  return m;
}

/// x != UINT_MAX  ==>  x < x+1
mitre *test_lt_succ(const uint32_t size)
{
  mitre *m = mitre_init("test_lt_succ", size);
  bitvec *x = bfvInit(m->man, size), *uint_max = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "uint_max", uint_max);
  literal *p;
  forBv(p, uint_max) bfPushAssumption(m->man, *p);

  bitvec *one = bfvConstant(m->man, size, 1, false);
  bitvec *succ = bfvAdd(m->man, x, one, LITERAL_FALSE, NULL);

  m->output = bfNewImplies(m->man, -bfvEqual(m->man, x, uint_max),
                           bfvUlt(m->man, x, succ));

  return m;
}

// x!=INT_MAX  ==>  x+1 > x
mitre *test_slt_succ(const uint32_t size)
{
  mitre *m = mitre_init("test_slt_succ", size);
  bitvec *x = bfvInit(m->man, size), *int_max = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "int_max", int_max);

  for (uintptr_t i = 0; i < size-1; i++) {
    bfPushAssumption(m->man, int_max->data[i]);
  }
  bfPushAssumption(m->man, -int_max->data[size-1]);

  bitvec *one = bfvConstant(m->man, size, 1, true);
  bitvec *succ = bfvAdd(m->man, x, one, LITERAL_FALSE, NULL);

  m->output = bfNewImplies(m->man, -bfvEqual(m->man, x, int_max),
                           bfvSlt(m->man, x, succ));

  return m;
}

mitre *test_lte(const uint32_t size)
{
  mitre *m = mitre_init("test_lte", size);
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  literal x_le_y = bfvUlte(m->man, x, y);
  mitre_addInputLit(m, "x<=y", x_le_y);
  literal rhs = bfNewOr(m->man, bfvUlt(m->man, x,y), bfvEqual(m->man, x,y));
  m->output = bfNewIff(m->man, x_le_y, rhs);
  return m;
}

mitre *test_lte_19a(const uint32_t size)
{
  mitre *m = mitre_init("test_lte_19a", size);
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  literal cout;
  CONSUME_BITVEC(bfvAdd(m->man, y, bfvInvert(m->man, x), LITERAL_TRUE, &cout));
  m->output =
    bfNewIff(m->man, bfNewOr(m->man, bfvUlt(m->man, x, y), bfvEqual(m->man, x, y)),
             cout);

  return m;
}

/* Hacker's Delight, page 24 */
mitre *test_less_19a(const uint32_t size)
{
  mitre *m = mitre_init("test_less_19a", size);
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  literal cout;
  /* ~(x + ~y + 1).cout = x <_u y */
  bitvec *ny = bfvInvert(m->man, y);
  mitre_addInput(m, "~y", ny);
  CONSUME_BITVEC(bfvAdd(m->man, x, ny, LITERAL_TRUE, &cout));
  mitre_addInputLit(m, "cout", cout);
  m->output = bfNewIff(m->man, bfvUlt(m->man, x, y), -cout);


  return m;
}

/* Hacker's Delight, p. 23 */
mitre *test_sless_19a(const uint32_t size)
{
  mitre *m = mitre_init("test_sless_19a (x<y <=> ...)", size);
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  /* x < y = (x <u y) xor x[31] xor y[31]
   * where <u is unsigned less than */
  literal lt = bfvSlt(m->man, x, y);
  bitvec *forXor = bfvAlloc(size);
  bfvPush(forXor, bfvUlt(m->man, x, y));
  bfvPush(forXor, x->data[x->size-1]);
  bfvPush(forXor, y->data[y->size-1]);
  literal _xor = bfBigXor(m->man, forXor);
  mitre_addInputLit(m, "x<y", lt);
  mitre_addInputLit(m, "rhs", _xor);
  m->output = bfNewIff(m->man, lt, _xor);
  return m;
}

mitre *test_sless_19b(const uint32_t size)
{
  mitre *m = mitre_init("test_sless_19b (x<y <=> ...)", size);
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  bitvec *two_31 = bfvInit(m->man, size);
  bfPushAssumption(m->man, two_31->data[two_31->size-1]);
  for (uint32_t i = 0; i < size; i++) {
    if (i != (two_31->size-1u)) {
      bfPushAssumption(m->man, -two_31->data[i]);
    }
  }
  mitre_addInput(m, "maxpow", two_31);

  literal lt = bfvSlt(m->man, x, y);
  mitre_addInputLit(m, "x<y", lt);
  literal cout;
  bitvec *sum =
    bfvAdd(m->man,
           mitre_addInput(m, "a=x+maxpow", bfvAdd(m->man, x, two_31, LITERAL_FALSE, NULL)),
           bfvInvert(m->man, mitre_addInput(m, "b=y+maxpow", bfvAdd(m->man, y, two_31, LITERAL_FALSE, NULL))),
           LITERAL_TRUE, &cout);
  mitre_addInput(m, "a+~b+1", sum);
  mitre_addInputLit(m, "carry(a+~b+1)", cout);

  m->output = bfNewIff(m->man, lt, -cout);
  return m;
}

mitre *test_lshift_for_realzies(const uint32_t width, const uint32_t distwidth)
{
  mitre *m = mitre_init("test_lshift_for_realzies", width);
  bitvec *x = bfvInit(m->man, width), *dist = bfvInit(m->man, distwidth);
  mitre_addInput(m, "x", x), mitre_addInput(m, "s", dist);

  bitvec *bvWidth = bfvHold(bfvConstant(m->man, width, x->size, false));

  bitvec *lshifted = bfvLshift(m->man, x, dist);
  mitre_addInput(m, "lshifted", lshifted);
  bitvec *rshifted = bfvRshift(m->man, lshifted, dist, LITERAL_FALSE);
  mitre_addInput(m, "rshifted", rshifted);

  literal out = LITERAL_TRUE;
  // test lshifted fill bits
  for (uint32_t i = 0; i < width; i++) {
    bitvec *index_i = bfvConstant(m->man, width, i, false);
    out = bfNewAnd(
      m->man,
      out,
      bfNewImplies(
        m->man,
        bfvUlt(m->man, index_i, bfvExtend(m->man, width, dist, EXTEND_ZERO)),
        bfNewEqual(m->man, lshifted->data[i], LITERAL_FALSE)));
  }

  // test bits that were shifted back to their orig. positions
  for (uint32_t i = 0; i < width; i++) {
    bitvec *index_i = bfvConstant(m->man, width, i, false);
    out = bfNewAnd(
      m->man,
      out,
      bfNewImplies(
        m->man,
        bfvUlt(m->man, bfvExtend(m->man, width, dist, EXTEND_ZERO), bfvSubtract(m->man, bvWidth, index_i)),
        bfNewEqual(m->man, x->data[i], rshifted->data[i])));
  }
  m->output = out;
  CONSUME_BITVEC(bfvRelease(bvWidth));
  return m;
}

mitre *test_lshift_const(const uint32_t width)
{
  mitre *m = mitre_init("test_lshift_const", width);
  bitvec *x = bfvInit(m->man, width);
  mitre_addInput(m, "x", x);
  bfman *b = m->man;

  m->output = LITERAL_TRUE;
  for (uint32_t dist = 0; dist < pow(2, (uint32_t) log2(width)); dist++) {
    bitvec *shifted = bfvLshiftConst(b, x, dist);
    char buf[1024];
    sprintf(buf, "shifted_%" PRIu32, dist);
    mitre_addInput(m, buf, shifted);

    for (uint32_t i = 0; i < x->size; i++) {
      if (i < dist) {
        m->output = bfNewAnd(b, m->output,
                             bfNewEqual(b, LITERAL_FALSE,
                                        shifted->data[i]));
      } else {
        m->output = bfNewAnd(b, m->output,
                             bfNewEqual(b, x->data[i-dist], shifted->data[i]));
      }
    }
  }
  
  

  return m;
}

mitre *test_rshift_for_realzies(const uint32_t width, const uint32_t distwidth)
{
  mitre *m = mitre_init("test_rshift_for_realzies", width);
  bfman *b = m->man;
  bitvec *x = bfvInit(m->man, width), *dist = bfvInit(m->man, distwidth);
  mitre_addInput(m, "x", x), mitre_addInput(m, "s", dist);

  bitvec *bvWidth = bfvHold(bfvConstant(b, width, x->size, false));
  /* bitvec *ub = bfvSubtract(b, bvWidth, bfvExtend(b, width, dist, EXTEND_ZERO)); */
  literal fill = bfNewVar(b);

  bitvec *rshifted = bfvRshift(b, x, dist, fill);
  mitre_addInput(m, "rshifted", rshifted);
  bitvec *lshifted = bfvHold(bfvLshift(b, rshifted, dist));
  mitre_addInput(m, "lshifted", lshifted);
  mitre_addInputLit(m, "fill", fill);

  m->output = LITERAL_TRUE;
  // test rshifted fill bits
  for (uint32_t j = width; j > 0; j--) {
    uint32_t i = j-1;
    bitvec *index_i = bfvConstant(b, width, (uint32_t) i, false);
    // bvWidth-dist < i  ==>  rshifted[i]==fill
    m->output = bfNewAnd(
      b, m->output,
      bfNewImplies(
        b,
        bfvUlt(
          b,
          bfvSubtract(b, bvWidth, index_i),
          bfvExtend(m->man, width, dist, EXTEND_ZERO)),
        bfNewEqual(b, rshifted->data[i], fill)));
  }

  // test bits that were shifted back to their orig. positions
  for (uint32_t j = width; j > 0; j--) {
    uint32_t i = j-1;
    bitvec *index_i = bfvHold(bfvConstant(b, width, (uint32_t) i, false));
    m->output = bfNewAnd(
      b,
      m->output,
      bfNewSelect(
        b,
        bfNewOr(
          b,
          bfvEqual(b, bfvExtend(b, width, dist, EXTEND_ZERO), index_i),
          bfvUlt(b, bfvExtend(b, width, dist, EXTEND_ZERO), index_i)),
        bfNewEqual(b, lshifted->data[i], x->data[i]),
        bfNewEqual(b, lshifted->data[i], LITERAL_FALSE)));
    CONSUME_BITVEC(bfvRelease(index_i));
  }

  CONSUME_BITVEC(bfvRelease(bvWidth));
  CONSUME_BITVEC(bfvRelease(lshifted));
  return m;
}

mitre *test_rshift_const(const uint32_t width)
{
  mitre *m = mitre_init("test_rshift_const", width);
  bfman *b = m->man;
  bitvec *x = bfvInit(m->man, width);
  mitre_addInput(m, "x", x);

  m->output = LITERAL_TRUE;
  literal fill = bfNewVar(b);
  for (uint32_t dist = 0; dist < pow(2, (uint32_t) log2(width)); dist++) {
    bitvec *shifted = bfvRshiftConst(b, x, dist, fill);
    char buf[1024];
    sprintf(buf, "shifted_%" PRIu32, dist);
    mitre_addInput(m, buf, shifted);

    for (uint32_t i = 0; i < x->size; i++) {
      if (i >= width-dist) {
        m->output = bfNewAnd(b, m->output, bfNewEqual(b, fill, shifted->data[i]));
      } else {
        m->output = bfNewAnd(b, m->output, bfNewEqual(b, x->data[i+dist], shifted->data[i]));
      }
    }
  }
  return m;
}

mitre *test_slte(const uint32_t size)
{
  mitre *m = mitre_init("test_slte", size);
  bitvec *x = bfvInit(m->man, size), *y = bfvInit(m->man, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  bitvec *two_31 = bfvInit(m->man, size);
  bfPushAssumption(m->man, two_31->data[two_31->size-1]);
  for (uint32_t i = 0; i < size-1; i++) {
    bfPushAssumption(m->man, -two_31->data[i]);
  }
  mitre_addInput(m, "2^31", two_31);

  m->output =
    bfNewIff(m->man,
             bfvSlte(m->man, x,y),
             bfvUlte(m->man,
                     bfvAdd(m->man, x, two_31, LITERAL_FALSE, NULL),
                     bfvAdd(m->man, y, two_31, LITERAL_FALSE, NULL)));
  return m;
}

mitre *test_sltZero(const uint32_t width)
{
  mitre *m = mitre_init("test_slt_0", width);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, width), *y = bfvInit(b, width);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  m->output =
    bfNewIff(b, bfvSlt(b, x, bfvConstant(b, width, 0, false)),
             bfvSltZero(b,x));
  return m;
}

mitre *test_sgreater_than_zero(const uint32_t width)
{
  mitre *m = mitre_init("test_sgreater_than_zero", width);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, width), *y = bfvInit(b, width);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  m->output =
    bfNewIff(b, bfvSlt(b, bfvConstant(b, width, 0, false), x),
             bfvSgtZero(b,x));

  return m;
}

mitre *test_sgteZero(const uint32_t width)
{
  mitre *m = mitre_init("test_sgteZero", width);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, width), *y = bfvInit(b, width);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  m->output =
    bfNewIff(b, bfNewOr(b, bfvSlt(b, bfvConstant(b, width, 0, false), x),
                        bfvEqual(b, x, bfvConstant(b, width, 0, false))),
             bfvSgteZero(b,x));

  return m;
}

mitre *test_lselect()
{
  mitre *m = mitre_init("test_lselect", 1);
  bfman *b = m->man;
  literal tst = mitre_addInputLit(m, "tst", bfNewVar(b));
  literal thn = mitre_addInputLit(m, "thn", bfNewVar(b)),
    els = mitre_addInputLit(m, "els", bfNewVar(b));

  literal res = bfNewSelect(b, tst, thn, els);

  m->output =
    bfNewAnd(b, bfNewImplies(b, tst, bfNewEqual(b, thn, res)),
             bfNewImplies(b, -tst, bfNewEqual(b, els, res)));
  return m;
}


mitre *test_mul_commutes(const uint32_t size)
{
  mitre *m = mitre_init("test_mul_commutes", size);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, size), *y = bfvInit(b, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  bitvec *xy = bfvMul(b,x,y);
  bitvec *yx = bfvMul(b,y,x);

  m->output = bfvEqual(b,xy,yx);

  return m;
}

/// q=(x/y), r=(x%y), q*y + r = x
mitre *test_quotRem_mul(const uint32_t size)
{
  mitre *m = mitre_init("test_quotRem_mul", size);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, size), *y = bfvInit(b, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  bitvec *q, *r;
  bfvQuotRem(b,x,y,&q,&r);
  mitre_addInput(m, "q", q), mitre_addInput(m, "r", r);

  bitvec *mul = bfvMul(b,q,y);
  bitvec *sum = bfvAdd(b, mul, r, LITERAL_FALSE, NULL);

  m->output = bfvEqual(b, sum, x);

  return m;
}

/// q=(x/y), r=(x%y), q*y + r = x
mitre *test_div_rem(const uint32_t size)
{
  mitre *m = mitre_init("test_div_rem", size);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, size), *y = bfvInit(b, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  bitvec *q = bfvDiv(b, x, y);
  bitvec *r = bfvRem(b, x, y);

  m->output = bfvEqual(b, bfvAdd0(b, bfvMul(b, q, y), r), x); 

  return m;
}

/// q=(x/y), r=(x%y), q*y + r = x
mitre *test_sdiv_srem(const uint32_t size)
{
  mitre *m = mitre_init("test_sdiv_srem", size);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, size), *y = bfvInit(b, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  bitvec *q = bfvSdiv(b, x, y);
  bitvec *r = bfvSrem(b, x, y);

  m->output = bfvEqual(b, bfvAdd0(b, bfvMul(b, q, y), r), x); 

  return m;
}

mitre *test_concat(const uint32_t size)
{
  mitre *m = mitre_init("test_concat", size);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, size), *y = bfvInit(b, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y);

  bitvec *c = bfvConcat(b,x,y);

  m->output = LITERAL_TRUE;
  for (unsigned i = 0; i < x->size; i++) {
    m->output = bfNewAnd(b, m->output, bfNewEqual(b, c->data[i], x->data[i]));
  }

  for (unsigned i = x->size; i < c->size; i++) {
    m->output = bfNewAnd(b, m->output, bfNewEqual(b, c->data[i], y->data[i-x->size]));
  }

  CONSUME_BITVEC(c);
  return m;
}

mitre *test_extract(const uint32_t width)
{
  mitre *m = mitre_init("test_extract", width);
  bfman *b = m->man;

  bitvec *x = bfvInit(b, width);
  mitre_addInput(m, "x", x);
  bitvec *e1 = bfvExtract(b, x, 0, width);
  m->output = bf_bool2lit(e1->size == width);
  for (unsigned i = 0; i < x->size; i++) {
    m->output = bfNewAnd(b, m->output, bf_bool2lit(x->data[i] == e1->data[i]));
  }

  bitvec *e2 = bfvExtract(b, x, 1, width/3);
  m->output = bfNewAnd(b, m->output, bf_bool2lit(e2->size == width/3));
  for (unsigned i = 0; i < e2->size; i++) {
    m->output = bfNewAnd(b, m->output, bf_bool2lit(x->data[i+1] == e2->data[i]));
  }

  CONSUME_BITVEC(e1), CONSUME_BITVEC(e2);
  return m;
}


static int tacLevel = 5;

mitre *test_xor_x_x_is_0(const uint32_t width)
{
  mitre *m = mitre_init("test_xor_x_x_is_0", width);
  bfman *b = m->man;

  bitvec *x = bfvInit(b, width);
  mitre_addInput(m, "x", x);
  m->output = -bfBigAnd(b, bfvXor(b, x, x));
  return m;
}

mitre *test_xor_x_y_x_is_y(const uint32_t width)
{
  mitre *m = mitre_init("test_xor_x_y_x_is_y", width);
  bfman *b = m->man;

  bitvec *x = bfvInit(b, width), *y = bfvInit(b, width);
  mitre_addInput(m, "x", x);
  mitre_addInput(m, "y", y);
  m->output = bfvEqual(b, y, bfvXor(b, bfvXor(b, x, y), x));
  return m;
}

mitre *test_xor_assoc(const uint32_t width)
{
  mitre *m = mitre_init("test_xor_assoc", width);
  bfman *b = m->man;

  bitvec *x = bfvInit(b, width),
         *y = bfvInit(b, width),
         *z = bfvInit(b, width);
  mitre_addInput(m, "x", x);
  mitre_addInput(m, "y", y);
  mitre_addInput(m, "z", z);
  m->output = bfvEqual(b, bfvXor(b, x, bfvXor(b, y, z)),
                          bfvXor(b, bfvXor(b, x, y), z));
  return m;
}

mitre *test_zero_value_flag() 
{
  uint32_t addrsize=8;
  uint32_t bits_in_byte = 8;
  uint32_t valsize = bits_in_byte; //One byte test
  mitre *m = mitre_init("test_zero_value_flag", addrsize);
  bfman *b = m->man;

  bitvec *const1 = bfvUconstant(b,valsize,0);
  bitvec *zf = bfvUconstant(b,valsize,0);
  literal lit = bfvEqual(b,zf,const1);
  bfPushAssumption(b,lit);
  if (BF_SAT == bfSolve(b)) { m->output = LITERAL_TRUE; } else { m->output = LITERAL_FALSE; }

  return m;
}
 
mitre *test_memory_aliasing()
{
  uint32_t addrsize=32;
  uint32_t bits_in_byte=8;
  uint32_t bytes_in_word=4;
  uint32_t valsize = bits_in_byte*bytes_in_word;
  mitre *m = mitre_init("test_memory_aliasing", addrsize);
  bfman *b = m->man;
  memory *mem = bfmInit(b,addrsize,bits_in_byte);

  bitvec *rbx = bfvHold(bfvInit(b,addrsize));
  bitvec *temp1 = bfvHold(bfvInit(b,valsize));
  bfmStore_be(b,mem,rbx,temp1);
  bitvec *rax = bfvHold(bfmLoad_be(b,mem,rbx,bytes_in_word));
  assert(bfvSame(rax, temp1));
  CONSUME_BITVEC(bfvRelease(temp1));
  literal lit1=bfvEqual(b,bfvUconstant(b,valsize,0xdeadbeef),rax);
  assert(BF_UNSAT != bfPushAssumption(b,lit1));

  bitvec *rdx = bfvHold(bfvInit(b,addrsize));
  bitvec *temp2 = bfvHold(bfvInit(b,valsize));
  bfmStore_be(b,mem,rdx,temp2);
  bitvec *rcx = bfvHold(bfmLoad_be(b,mem,rdx,bytes_in_word));
  assert(bfvSame(rcx, temp2));
  CONSUME_BITVEC(bfvRelease(temp2));
  bitvec *const2=bfvHold(bfvUconstant(b,valsize,0xdeadbeef));
  literal lit2=bfvEqual(b,const2,rcx);
  assert(BF_UNSAT != bfPushAssumption(b,lit2));

  /* addresses do not alias */
  for (unsigned i = 0; i < 4; i++) {
    literal lit=bfvEqual(b,bfvAdd0(b, rbx, bfvUconstant(b, addrsize, i)),bfvAdd0(b, rdx, bfvUconstant(b, addrsize, i)));
    assert(BF_UNSAT != bfPushAssumption(b,-lit));
  }

//  bfPushAssumption(b, -767);

  int result=bfSolve(b);

  if (BF_SAT == result) {
    memory *cMEM=bfmFromCounterExample(b,mem);
    fprintf(stderr,"Satisfiable\n");
    bitvec *cRBX=bfvHold(bfvFromCounterExample(b,rbx));
    bitvec *cRAX=bfvHold(bfvFromCounterExample(b,rax));
    bitvec *loadedRBX=bfvHold(bfmLoad_be(b,cMEM,cRBX,4));
    uint64_t loadedRBXvalue=bfvGet(b,loadedRBX);
    fprintf(stderr,"rax = %" PRIu64 "\n", bfvGet(b,cRAX));
    fprintf(stderr,"rbx = %#x, [rbx] = %#x\n", (int)bfvGet(b,cRBX),(int)loadedRBXvalue);

    bitvec *cRCX=bfvHold(bfvFromCounterExample(b,rcx));
    bitvec *cRDX=bfvHold(bfvFromCounterExample(b,rdx));
    bitvec *loadedRDX=bfvHold(bfmLoad_be(b,cMEM,cRDX,4));
    uint64_t loadedRDXvalue=bfvGet(b,loadedRDX);
    fprintf(stderr,"rcx = %#x\n",(int)bfvGet(b,cRCX));
    fprintf(stderr,"rdx = %#x, [rdx] = %x\n",(int)bfvGet(b,cRDX),(int)loadedRDXvalue);
    m->output = LITERAL_TRUE; //intentionally exit
    CONSUME_BITVEC(bfvRelease(cRAX));
    CONSUME_BITVEC(bfvRelease(cRBX));
    CONSUME_BITVEC(bfvRelease(cRCX));
    CONSUME_BITVEC(bfvRelease(cRDX));
    CONSUME_BITVEC(bfvRelease(loadedRBX));
    CONSUME_BITVEC(bfvRelease(loadedRDX));
    bfmDestroy(m->man, cMEM);
  } else { 
	fprintf(stderr,"Unsatisfiable"); 
    m->output = LITERAL_FALSE;
  }


  //m->output = bfvEqual(b, y, bfvXor(b, bfvXor(b, x, y), x));

  CONSUME_BITVEC(bfvRelease(rax));
  CONSUME_BITVEC(bfvRelease(rbx));
  CONSUME_BITVEC(bfvRelease(rcx));
  CONSUME_BITVEC(bfvRelease(rdx));
  CONSUME_BITVEC(bfvRelease(const2));
  bfmDestroy(m->man, mem);

  return m;
}

mitre *test_rotl(const uint32_t sz)
{
  mitre *mt = mitre_init("test_zero_value_flag", sz);
  bfman *bf = mt->man;
  bitvec *x = bfvUconstant(bf, 32, 0x55555555);
  
  bitvec *x_rot = bfvLrotate(bf, x, bfvOne(32));
  bfvPrintConcrete(bf, stderr, x_rot, PRINT_HEX);

  return mt;
}

int testBetween(uint32_t size)
{
  fprintf(stderr, "Running testBetween %" PRIu32 " ...", size);
  assert(size > 10);
  mitre *m = mitre_init("testBetween", size);
  bfman *b = m->man;
  bitvec *x = bfvInit(b, size), *y = bfvInit(b, size), *z = bfvInit(b, size);
  mitre_addInput(m, "x", x), mitre_addInput(m, "y", y), mitre_addInput(m, "z", z);

  
  bfSet(b, bfvEqual(b, x, bfvUconstant(b, size, 15)));
  bfSet(b, bfvUlt(b, y, bfvUconstant(b, size, 17)));
  bfSet(b, bfvUlt(b, x, y));

  literal answer = bfvEqual(b, y, bfvUconstant(b, size, 16));

  int ret = 0;
  if (ret==0) ret = !(true == bfMayBeTrue(b, answer));
  if (ret==0) ret = !(true == bfMustBeTrue(b, answer));
  if (ret==0) ret = !(BF_SAT == bfSolve(b));
  if (ret==0) ret = !(16 == bfvGet(b, y));
  mitre_destroy(m);
  if (!ret) {
    fprintf(stderr, " passed.\n");
  }
  return ret;
}

mitre *test_setClause()
{
  mitre *m = mitre_init("test_setClause", 32);
  bitvec *b1 = bfvHold(bfvAlloc(2));

  literal x = bfNewVar(m->man), y = bfNewVar(m->man);
  bfvPush(b1, -x);
  bfvPush(b1, y);
  bfSetClause(m->man, b1);

  bfvClear(b1);
  bfvPush(b1, x);

  m->output = -y;

  CONSUME_BITVEC(bfvRelease(b1));
  return m;
}

static int runTests()
{
  int ret = 0;
  srandom(1);

  /* if (ret==0) ret = mitre_run(test_rotl(32)); */
  if (ret==0) ret = mitre_run(test_setClause());
  if (ret==0) ret = testConstProp(32);
  if (ret==0) ret = testConstProp(64);
  if (ret==0) ret = test_check_status_f();
  if (ret==0) ret = test_check_status_t();
  if (ret==0) ret = test_check_status_tf();
  if (ret==0) ret = mitre_run(test_zero_value_flag());
  if (ret==0) ret = mitre_run(test_memory_aliasing());
  if (ret==0) ret = mitre_run(test_iff());
  if (ret==0) ret = mitre_run(test_constants());
  if (ret==0) ret = mitre_run(testExtension(1));
  if (ret==0) ret = mitre_run(testBigAnd(0));
  if (ret==0) ret = mitre_run(testBigAnd(1));
  if (ret==0) ret = mitre_run(testBigAnd(2));
  if (ret==0) ret = mitre_run(testBigAnd(32));
  if (ret==0) ret = mitre_run(testBigOr(0));
  if (ret==0) ret = mitre_run(testBigOr(1));
  if (ret==0) ret = mitre_run(testBigOr(2));
  if (ret==0) ret = mitre_run(testBigOr(32));
  if (ret==0) ret = mitre_run(test_add_0_is_identity(1));
  if (ret==0) ret = mitre_run(test_add_0_is_identity(2));
  if (ret==0) ret = mitre_run(test_add_0_is_identity(3));
  if (ret==0) ret = mitre_run(test_add_0_is_identity(32));
  if (ret==0) ret = mitre_run(test_add_0_is_identity(64));
  if (ret==0) ret = mitre_run(test_add_commutes(1));
  if (ret==0) ret = mitre_run(test_add_commutes(32));
  if (ret==0) ret = mitre_run(test_add_commutes(64));
  if (ret==0) ret = mitre_run(test_x_minus_x(1));
  if (ret==0) ret = mitre_run(test_x_minus_x(32));
  if (ret==0) ret = mitre_run(test_x_minus_x(64));
  if (ret==0) ret = mitre_run(testAddSubInverse(1));
  if (ret==0) ret = mitre_run(testAddSubInverse(32));
  if (ret==0) ret = mitre_run(testAddSubInverse(64));
  if (ret==0) ret = mitre_run(test_negate_is_invert_plus_1(2));
  if (ret==0) ret = mitre_run(test_negate_is_invert_plus_1(32));
  if (ret==0) ret = mitre_run(test_negate_is_invert_plus_1(64));
  if (ret==0) ret = mitre_run(test_lt_max(1));
  if (ret==0) ret = mitre_run(test_lt_max(32));
  if (ret==0) ret = mitre_run(test_lt_max(64));
  if (ret==0) ret = mitre_run(test_slt_max(1));
  if (ret==0) ret = mitre_run(test_slt_max(2));
  if (ret==0) ret = mitre_run(test_slt_max(32));
  if (ret==0) ret = mitre_run(test_slt_max(64));
  if (ret==0) ret = mitre_run(test_lt_succ(1));
  if (ret==0) ret = mitre_run(test_lt_succ(32));
  if (ret==0) ret = mitre_run(test_lt_succ(64));
  if (ret==0) ret = mitre_run(test_slt_succ(1));
  if (ret==0) ret = mitre_run(test_slt_succ(32));
  if (ret==0) ret = mitre_run(test_slt_succ(64));
  if (ret==0) ret = mitre_run(test_lte(1));
  if (ret==0) ret = mitre_run(test_lte(32));
  if (ret==0) ret = mitre_run(test_lte(64));
  if (ret==0) ret = mitre_run(test_lte_19a(1));
  if (ret==0) ret = mitre_run(test_lte_19a(32));
  if (ret==0) ret = mitre_run(test_lte_19a(64));
  if (ret==0) ret = mitre_run(test_less_19a(1));
  if (ret==0) ret = mitre_run(test_less_19a(32));
  if (ret==0) ret = mitre_run(test_less_19a(64));
  if (ret==0) ret = mitre_run(test_sless_19a(1));
  if (ret==0) ret = mitre_run(test_sless_19a(32));
  if (ret==0) ret = mitre_run(test_sless_19b(1));
  if (ret==0) ret = mitre_run(test_sless_19b(2));
  if (ret==0) ret = mitre_run(test_sless_19b(32));
  if (ret==0) ret = mitre_run(test_sless_19b(64));
  if (ret==0) ret = mitre_run(test_slte(1));
  if (ret==0) ret = mitre_run(test_slte(32));
  if (ret==0) ret = mitre_run(test_slte(64));
  if (ret==0) ret = mitre_run(test_sltZero(1));
  if (ret==0) ret = mitre_run(test_sltZero(32));
  if (ret==0) ret = mitre_run(test_sltZero(64));
  if (ret==0) ret = mitre_run(test_sgreater_than_zero(1));
  if (ret==0) ret = mitre_run(test_sgreater_than_zero(32));
  if (ret==0) ret = mitre_run(test_sgreater_than_zero(64));
  if (ret==0) ret = mitre_run(test_sgteZero(1));
  if (ret==0) ret = mitre_run(test_sgteZero(32));
  if (ret==0) ret = mitre_run(test_sgteZero(64));
  if (ret==0) ret = mitre_run(test_lshift_const(1));
  if (ret==0) ret = mitre_run(test_lshift_const(32));
  if (ret==0) ret = mitre_run(test_lshift_const(64));
  if (ret==0) ret = mitre_run(test_lshift_for_realzies(1, 1));
  if (ret==0) ret = mitre_run(test_lshift_for_realzies(1, 0));
  if (ret==0) ret = mitre_run(test_lshift_for_realzies(32, 2));
  if (ret==0) ret = mitre_run(test_lshift_for_realzies(32, 5));
  if (ret==0) ret = mitre_run(test_lshift_for_realzies(32, 32));
  if (ret==0) ret = mitre_run(test_lshift_for_realzies(64, 4));
  if (ret==0) ret = mitre_run(test_lshift_for_realzies(64, 6));
  if (ret==0) ret = mitre_run(test_lshift_for_realzies(64, 32));
  if (ret==0) ret = mitre_run(test_rshift_const(1));
  if (ret==0) ret = mitre_run(test_rshift_const(32));
  if (ret==0) ret = mitre_run(test_rshift_const(64));
  if (ret==0) ret = mitre_run(test_rshift_for_realzies(1, 1));
  if (ret==0) ret = mitre_run(test_rshift_for_realzies(1, 0));
  if (ret==0) ret = mitre_run(test_rshift_for_realzies(32, 2));
  if (ret==0) ret = mitre_run(test_rshift_for_realzies(32, 5));
  if (ret==0) ret = mitre_run(test_rshift_for_realzies(32, 32));
  if (ret==0) ret = mitre_run(test_rshift_for_realzies(64, 4));
  if (ret==0) ret = mitre_run(test_rshift_for_realzies(64, 6));
  if (ret==0) ret = mitre_run(test_rshift_for_realzies(64, 32));
  if (ret==0) ret = mitre_run(test_lselect());
  if (ret==0) ret = mitre_run(test_mul_commutes(1));
  if (ret==0) ret = mitre_run(test_mul_commutes(2));
  if (ret==0) ret = mitre_run(test_mul_commutes(7));
  if (ret==0) ret = mitre_run(test_quotRem_mul(1));
  if (ret==0) ret = mitre_run(test_quotRem_mul(8));
  if (ret==0) ret = mitre_run(test_div_rem(1));
  if (ret==0) ret = mitre_run(test_div_rem(8));
  if (ret==0) ret = mitre_run(test_sdiv_srem(1));
  if (ret==0) ret = mitre_run(test_sdiv_srem(8));
  if (ret==0) ret = mitre_run(test_concat(1));
  if (ret==0) ret = mitre_run(test_concat(32));
  if (ret==0) ret = mitre_run(test_concat(64));
  if (ret==0) ret = mitre_run(test_extract(1));
  if (ret==0) ret = mitre_run(test_extract(32));
  if (ret==0) ret = mitre_run(test_extract(64));
  if (ret==0) ret = mitre_run(test_xor_x_x_is_0(1));
  if (ret==0) ret = mitre_run(test_xor_x_x_is_0(32));
  if (ret==0) ret = mitre_run(test_xor_x_x_is_0(64));
  if (ret==0) ret = mitre_run(test_xor_x_y_x_is_y(1));
  if (ret==0) ret = mitre_run(test_xor_x_y_x_is_y(32));
  if (ret==0) ret = mitre_run(test_xor_x_y_x_is_y(64));
  if (ret==0) ret = mitre_run(test_xor_assoc(1));
  if (ret==0) ret = mitre_run(test_xor_assoc(32));
  if (ret==0) ret = mitre_run(test_xor_assoc(64));
  if (ret==0) ret = testBetween(32);
  return ret;

}

float toseconds(clock_t t) {
  return (float)t/CLOCKS_PER_SEC;
}

int main(int UNUSED(argc), char **UNUSED(argv))
{
  int ret = 0;
  clock_t start, end;
  clock_t aig = 0, cnf = 0;


#if HAVE_PERFPROBES
  probe_update_rate = 0.1;
  void *ds = dirfilesampler_start_name("bitvec-test.perf", 0.5);
  //void *ss = stdiosampler_start(stderr, 1);
#endif

  fprintf(stderr, "Running AIG mode tests ...\n");
  start = clock();
  if (ret==0) ret = runTests();
  end = clock();
  aig = end-start;
  fprintf(stderr, "AIG time: %g\n", toseconds(aig));

#if HAVE_PERFPROBES
  //stdiosampler_stop(ss);
  dirfilesampler_stop(ds);
#endif

  return ret;
}

