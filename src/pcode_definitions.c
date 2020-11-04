/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat.h>
#include <funcsat/system.h>
#include <funcsat/hashtable.h>
#include <bitfunc.h>
#include <bitfunc/array.h>
#include <bitfunc/circuits.h>
#include <bitfunc/program.h>
#include <bitfunc/pcode_definitions.h>
#include <math.h>
#include <funcsat.h>

void assertBoolByte(bitvec *input){
  for (unsigned i=1; i < input->size; i++)
  {
    assert(input->data[i] == LITERAL_FALSE);
  }
}

//pcode function definitions
bitvec *pCOPY(bfman *b, bitvec *input0) {
  bitvec *r = bfvDup(b, input0);
  return r;
}
 
bitvec *pINT_ADD(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  return bfvAdd0(b, input0, input1);
}

bitvec *pINT_SUB(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  return bfvSubtract(b, input0, input1);
}

bitvec *pINT_MULT(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  return bfvMul(b, input0, input1);
}

bitvec *pINT_DIV(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *quot;
  bitvec *rem;
  bfvQuotRem(b, input0, input1, &quot, &rem);
  CONSUME_BITVEC(rem);
  return quot;
}

bitvec *pINT_REM(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *quot;
  bitvec *rem;
  bfvQuotRem(b, input0, input1, &quot, &rem);
  CONSUME_BITVEC(quot);
  return rem;
}

bitvec *pINT_SDIV(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
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

bitvec *pINT_SREM(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
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

bitvec *pINT_OR(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  return bfvOr(b, input0, input1);
}

bitvec *pINT_XOR(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  return bfvXor(b, input0, input1);
}

bitvec *pINT_AND(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  return bfvAnd(b, input0, input1);
}

bitvec *pINT_LEFT(bfman *b, bitvec *input0, bitvec *input1) {
  double lg2size = log2( (double) input0->size);
  bitvec *ret;
  if (input1->size == lg2size){
    ret =  bfvLshift(b, input0, input1);
  }
  else if (input1->size < lg2size){
    ret =  bfvLshift(b, input0, bfvZextend(b, lg2size, input1));
  }
  else{
    bfvHold(input1), bfvHold(input0);
    ret = bfvSelect(
      b,
      bfvEqual(b,
               bfvRshiftConst(b, input1, lg2size, LITERAL_FALSE),
               bfvUconstant(b, input1->size, 0)),
      bfvLshift(b, input0, bfvTruncate(b, lg2size, input1)),
      bfvUconstant(b, input0->size, 0)
      );
    CONSUME_BITVEC(bfvRelease(input1)); CONSUME_BITVEC(bfvRelease(input0));
  }
  return ret;
}

bitvec *pINT_RIGHT(bfman *b, bitvec *input0, bitvec *input1) {
  double lg2size = log2( (double) input0->size);
  bitvec *ret;
  if (input1->size == lg2size){
    ret =  bfvRshift(b, input0, input1, LITERAL_FALSE);
  }
  else if (input1->size < lg2size){
    ret =  bfvRshift(b, input0, bfvZextend(b, lg2size, input1), LITERAL_FALSE);
  }
  else{
    bfvHold(input1), bfvHold(input0);
    ret = bfvSelect(
      b,
      bfvEqual(b,
               bfvRshiftConst(b, input1, lg2size, LITERAL_FALSE), 
               bfvUconstant(b, input1->size, 0)),
      bfvRshift(b, input0, bfvTruncate(b, lg2size, input1), LITERAL_FALSE),
      bfvUconstant(b, input0->size, 0)
      );  
    CONSUME_BITVEC(bfvRelease(input1)); CONSUME_BITVEC(bfvRelease(input0));
  }
  return ret;
}

bitvec *pINT_SRIGHT(bfman *b, bitvec *input0, bitvec *input1) {
  double lg2size = log2( (double) input0->size);
  bitvec *ret;
  if (input1->size == lg2size){
    ret =  bfvRshift(b, input0, input1, input0->data[input0->size -1]);
  }
  else if (input1->size < lg2size){
    ret =  bfvRshift(b, input0, bfvZextend(b, lg2size, input1), input0->data[input0->size -1]);
  }
  else{
    bfvHold(input1), bfvHold(input0);
    ret = bfvSelect(
      b,
      bfvEqual(b,
               bfvRshiftConst(b, input1, lg2size, LITERAL_FALSE), 
               bfvUconstant(b, input1->size, 0)),
      bfvRshift(b, input0, bfvTruncate(b, lg2size, input1), input0->data[input0->size - 1]),
      bfvSelect(
        b,
        input0->data[input0->size-1],
        bfvSconstant(b, input0->size, -1),
        bfvSconstant(b, input0->size, 0)
        )
      );  
    CONSUME_BITVEC(bfvRelease(input1)); CONSUME_BITVEC(bfvRelease(input0));
  }
  return ret;
}   


bitvec *pINT_EQUAL(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *output = bfvUconstant(b, BITS_IN_BYTE, 0);
  output->data[0] = bfvEqual(b, input0, input1);
   
  return output;
}

bitvec *pINT_NOTEQUAL(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *output = bfvUconstant(b, BITS_IN_BYTE, 0);
  output->data[0] = -(bfvEqual(b, input0, input1));
   
  return output;
}

bitvec *pINT_LESS(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *output = bfvUconstant(b, BITS_IN_BYTE, 0);
  output->data[0] = bfvUlt(b, input0, input1);
  return output;
}

bitvec *pINT_LESSEQUAL(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *output = bfvUconstant(b, BITS_IN_BYTE, 0);
  output->data[0] = bfvUlte(b, input0, input1);
  return output;
}

bitvec *pINT_CARRY(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *output = bfvUconstant(b, BITS_IN_BYTE, 0);
  bitvec *sum = bfvAdd(b, input0, input1, LITERAL_FALSE, &(output->data[0]));
  CONSUME_BITVEC(sum);
  return output;
}

bitvec *pINT_SLESS(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *output = bfvUconstant(b, BITS_IN_BYTE, 0);
  output->data[0] = bfvSlt(b, input0, input1);
  return output;
}

bitvec *pINT_SLESSEQUAL(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  bitvec *output = bfvUconstant(b, BITS_IN_BYTE, 0);
  output->data[0] = bfvSlte(b, input0, input1);
  return output;
}

bitvec *pINT_SCARRY(bfman *b, bitvec *input0, bitvec *input1) {
  INPUTCHECK(b, input0->size == input1->size, "input sizes not equal");
  unsigned i;
  bitvec *output = bfvUconstant(b, BITS_IN_BYTE, 0);

  //Symbolic case
  literal carry = LITERAL_FALSE; //Carry flag is initially false
  literal carryin = carry;
  for(i = 0; i < input0->size; i++) {
    if(i == input0->size-1) carryin = carry;
    //Majority vote
    carry = bfNewSelect(b, carry, bfNewOr(b, input0->data[i], input1->data[i]),
                                  bfNewAnd(b, input0->data[i], input1->data[i]));
  }
  output->data[0] = bfNewXor(b, carryin, carry);

  CONSUME_BITVEC(input0); CONSUME_BITVEC(input1);
  return output;
}

bitvec *pINT_SBORROW(bfman *b, bitvec *input0, bitvec *input1) {
  bitvec *input1c = pINT_2COMP(b, input1);
  bitvec *output = pINT_SCARRY(b, input0, input1c);
  return output;
}

bitvec *pBOOL_OR(bfman *b, bitvec *input0, bitvec *input1) {
  assert(input0->size == BITS_IN_BYTE);
  assert(input1->size == BITS_IN_BYTE);
  assertBoolByte(input0);
  assertBoolByte(input1);
  return bfvOr(b, input0, input1);
}

bitvec *pBOOL_XOR(bfman *b, bitvec *input0, bitvec *input1) {
  assert(input0->size == BITS_IN_BYTE);
  assert(input1->size == BITS_IN_BYTE);
  assertBoolByte(input0);
  assertBoolByte(input1);
  return bfvXor(b, input0, input1);
}

bitvec *pBOOL_AND(bfman *b, bitvec *input0, bitvec *input1) {
  assert(input0->size == BITS_IN_BYTE);
  assert(input1->size == BITS_IN_BYTE);
  assertBoolByte(input0);
  assertBoolByte(input1);
  return bfvAnd(b, input0, input1);
}

bitvec *pBOOL_NEGATE(bfman *b, bitvec *input0) {
  assert(input0->size == BITS_IN_BYTE);
  assertBoolByte(input0);
  return bfvXor(b, input0, bfvUconstant(b, BITS_IN_BYTE, 1));
}

bitvec *pPIECE(bfman *b, bitvec *input0, bitvec *input1) { 
  return bfvConcat(b, input1, input0);
}

//trunc and output_size are given in bytes
bitvec *pSUBPIECE(bfman *b, bitvec *input0, uint32_t input1, uint32_t output_size) {
  bitvec *outval =  bfvExtract(b, input0, (BITS_IN_BYTE * input1), output_size * BITS_IN_BYTE);
  bitvec *ret;
  if (outval->size < (output_size * BITS_IN_BYTE)){
    ret = bfvZextend(b, output_size * BITS_IN_BYTE, outval);
  }
  else if (outval->size > (output_size * BITS_IN_BYTE)){
    ret = bfvTruncate(b, output_size * BITS_IN_BYTE, outval);
  }
  else{
    ret = outval;
  }
  return ret;
}

bitvec *pINT_ZEXT(bfman *b, bitvec *input0, uint32_t output_size) {
  return bfvZextend(b, output_size*BITS_IN_BYTE, input0);
}

bitvec *pINT_SEXT(bfman *b, bitvec *input0, uint32_t output_size) {
  return bfvSextend(b, output_size*BITS_IN_BYTE, input0);
}

bitvec *pINT_2COMP(bfman *b, bitvec *input0) {
  return bfvNegate(b, input0);
}

bitvec *pINT_NEGATE(bfman *b, bitvec *input0) {
  return bfvInvert(b, input0);
}

