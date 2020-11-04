/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat.h>
#include <funcsat/hashtable.h>
#include <bitfunc.h>
#include <bitfunc/array.h>
#include <bitfunc/circuits.h>
#include <bitfunc/program.h>
#include <bitfunc/llvm_definitions.h>
#include <math.h>

#define UNUSED(x) (void)(x)

bitvec *bf_llvm_add(bfman *b, bitvec *input0, bitvec *input1,
                    literal *sw, literal *uw) {
  UNUSED(sw), UNUSED(uw);
  /* todo support sw and uw */
  return bfvAdd0(b, input0, input1);
}

bitvec *bf_llvm_sub(bfman *b, bitvec *input0, bitvec *input1,
                    literal *sw, literal *uw) {
  UNUSED(sw), UNUSED(uw);
  return bfvSubtract(b, input0, input1);
}

bitvec *bf_llvm_mul(bfman *b, bitvec *input0, bitvec *input1,
                    literal *sw, literal *uw) {
  UNUSED(sw), UNUSED(uw);
  return bfvMul(b, input0, input1);
}

bitvec *bf_llvm_udiv(bfman *b, bitvec *input0, bitvec *input1,
                     literal *exact) {
  UNUSED(exact);
  bitvec *quot;
  bitvec *rem;
  bfvQuotRem(b, input0, input1, &quot, &rem);
  CONSUME_BITVEC(rem);
  return quot;
}

bitvec *bf_llvm_sdiv(bfman *b, bitvec *input0, bitvec *input1,
                     literal *exact) {
  UNUSED(exact);
  assert(input0->size == input1->size);
  bitvec *output;
   
  bitvec *vec_zero = bfvUconstant(b, input0->size, 0);
  literal zerotest, sign, sign0, sign1;
   
  bfvHold(vec_zero), bfvHold(input1);
  zerotest = bfvEqual(b, input1, vec_zero);
  if (bf_litIsTrue(zerotest)) {
    bfvRelease(vec_zero);
    CONSUME_BITVEC(bfvRelease(input1));
    return vec_zero;
  }
   
  sign0 = input0->data[input0->size-1];
  sign1 = input1->data[input1->size-1];
  sign = bfNewXor(b, sign0, sign1);
   
  bitvec *input0_abs = bfvAbsoluteValue(b, input0);
  bitvec *input1_abs = bfvAbsoluteValue(b, input1);
  bitvec *quotient;
  bitvec *remainder;
  bfvQuotRem(b, input0_abs, input1_abs, &quotient, &remainder);
  CONSUME_BITVEC(remainder);
  bfvHold(quotient);
  bitvec *negated = bfvNegate(b, quotient);
  bfvRelease(quotient);
  bitvec *intermediate = bfvSelect(b, sign, negated, quotient);
  bfvRelease(vec_zero);
  output = bfvSelect(b, zerotest, vec_zero, intermediate);
   
  return output;   
}

bitvec *bf_llvm_urem(bfman *b, bitvec *input0, bitvec *input1) {
  bitvec *quot;
  bitvec *rem;
  bfvQuotRem(b, input0, input1, &quot, &rem);
  CONSUME_BITVEC(quot);
  return rem;
}

bitvec *bf_llvm_srem(bfman *b, bitvec *input0, bitvec *input1) {
  assert(input0->size == input1->size);
  bitvec *output;
   
  literal sign = input0->data[input0->size-1];
   
  bitvec *input0_abs = bfvAbsoluteValue(b, input0);
  bitvec *input1_abs = bfvAbsoluteValue(b, input1);
   
  bitvec *quotient;
  bitvec *remainder;
  bfvQuotRem(b, input0_abs, input1_abs, &quotient, &remainder);
  bfvHold(remainder);
  bitvec *negated = bfvNegate(b, remainder);
  bfvRelease(remainder);
   
  output = bfvSelect(b, sign, negated, remainder);
   
  return output;
}

bitvec *bf_llvm_shl(bfman *b, bitvec *input0, bitvec *input1,
                    literal *sw, literal *uw) {
  UNUSED(sw), UNUSED(uw);
  return bfvLshift(b, input0, input1);
}

bitvec *bf_llvm_lshr(bfman *b, bitvec *input0, bitvec *input1,
                     literal *exact) {
  UNUSED(exact);
  return bfvRshift(b, input0, input1, LITERAL_FALSE);
}

bitvec *bf_llvm_ashr(bfman *b, bitvec *input0, bitvec *input1,
                     literal *exact) {
  UNUSED(exact);
  return bfvRshift(b, input0, input1, input0->data[input0->size-1]);
}

bitvec *bf_llvm_sext(bfman *b, bitvec *input0, unsigned new_width)
{
  assert(new_width >= input0->size);
  return bfvSextend(b, new_width, input0);
}

bitvec *bf_llvm_and(bfman *b, bitvec *input0, bitvec *input1) {
  return bfvAnd(b, input0, input1);
}

bitvec *bf_llvm_or(bfman *b, bitvec *input0, bitvec *input1) {
  return bfvOr(b, input0, input1);
}

bitvec *bf_llvm_xor(bfman *b, bitvec *input0, bitvec *input1) {
  return bfvXor(b, input0, input1);
}

bitvec *bf_llvm_icmp(bfman *b, enum icmp_comparison cmp, bitvec *input0, bitvec *input1)
{
  bitvec *ret = bfvAlloc(1);
  switch (cmp) {
  case ICMP_EQ:
    bfvPush(ret, bfvEqual(b, input0, input1)); break;

  case ICMP_NE:
    bfvPush(ret, -bfvEqual(b, input0, input1)); break;

  case ICMP_UGT:
    bfvPush(ret, -bfvUlte(b, input0, input1)); break;

  case ICMP_UGE:
    bfvPush(ret, -bfvUlt(b, input0, input1)); break;

  case ICMP_ULT:
    bfvPush(ret, bfvUlt(b, input0, input1)); break;

  case ICMP_ULE:
    bfvPush(ret, bfvUlte(b, input0, input1)); break;

  case ICMP_SGT:
    bfvPush(ret, -bfvSlte(b, input0, input1)); break;

  case ICMP_SGE:
    bfvPush(ret, -bfvSlt(b, input0, input1)); break;

  case ICMP_SLT:
    bfvPush(ret, bfvSlt(b, input0, input1)); break;

  case ICMP_SLE:
    bfvPush(ret, bfvSlte(b, input0, input1)); break;

  default:
    assert(false && "unsupported comparison");
    abort();
  }
  return ret;
}
