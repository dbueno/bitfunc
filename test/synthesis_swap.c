#include <bitfunc/config.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

#include <bitfunc.h>
#include <bitfunc/circuits.h>
#include <bitfunc/program.h>


/* register labels */
enum
{
  A=0,
  B,
  C,
  D,
};

#define ADDR_NUM_BITS 8
#define REG_NUM_BITS 8

void loada0(bfman *b, machine_state *s)
{
  bfRegStore_le(b, s, A, bfvUconstant(b, REG_NUM_BITS, 0));
}

void loadb0(bfman *b, machine_state *s)
{
  bfRegStore_le(b, s, B, bfvUconstant(b, REG_NUM_BITS, 0));
}

void addaba(bfman *b, machine_state *s)
{
  bitvec *a_r = bfRegLoad_le(b, s, A, 1);
  bitvec *b_r = bfRegLoad_le(b, s, B, 1);
  bfRegStore_le(b, s, A, bfvAdd0(b, a_r, b_r));
}

void addabb(bfman *b, machine_state *s)
{
  bitvec *a_r = bfRegLoad_le(b, s, A, 1);
  bitvec *b_r = bfRegLoad_le(b, s, B, 1);
  bfRegStore_le(b, s, B, bfvAdd0(b, a_r, b_r));
}

void addcdc(bfman *b, machine_state *s)
{
  bitvec *c = bfRegLoad_le(b, s, C, 1);
  bitvec *d = bfRegLoad_le(b, s, D, 1);
  bfRegStore_le(b, s, C, bfvAdd0(b, c, d));
}

void addcdd(bfman *b, machine_state *s)
{
  bitvec *c = bfRegLoad_le(b, s, C, 1);
  bitvec *d = bfRegLoad_le(b, s, D, 1);
  bfRegStore_le(b, s, D, bfvAdd0(b, c, d));
}

void xoraba(bfman *b, machine_state *s)
{
  bitvec *a_r = bfRegLoad_le(b, s, A, 1);
  bitvec *b_r = bfRegLoad_le(b, s, B, 1);
  bfRegStore_le(b, s, A, bfvXor(b, a_r, b_r));
}

void xorabb(bfman *b, machine_state *s)
{
  bitvec *a_r = bfRegLoad_le(b, s, A, 1);
  bitvec *b_r = bfRegLoad_le(b, s, B, 1);
  bfRegStore_le(b, s, B, bfvXor(b, a_r, b_r));
}

void xorcdc(bfman *b, machine_state *s)
{
  bitvec *c = bfRegLoad_le(b, s, C, 1);
  bitvec *d = bfRegLoad_le(b, s, D, 1);
  bfRegStore_le(b, s, C, bfvXor(b, c, d));
}

void xorcdd(bfman *b, machine_state *s)
{
  bitvec *c = bfRegLoad_le(b, s, C, 1);
  bitvec *d = bfRegLoad_le(b, s, D, 1);
  bfRegStore_le(b, s, D, bfvXor(b, c, d));
}

instruction swapInstructions[10] =
{
  { .impl = loada0, .desc = "a=0"},
  { .impl = loadb0, .desc = "b=0"},
  { .impl = addaba, .desc = "a=a+b"},
  { .impl = addabb, .desc = "b=a+b"},
  { .impl = addcdc, .desc = "c=c+d"},
  { .impl = addcdd, .desc = "d=c+d"},
  { .impl = xoraba, .desc = "a=a^b"},
  { .impl = xorabb, .desc = "b=a^b"},
  { .impl = xorcdc, .desc = "c=c^d"},
  { .impl = xorcdd, .desc = "d=c^d"},
};

literal swapOracle(bfman *b, machine_state *init, machine_state *final)
{
  literal c1 =  bfNewAnd(b,
                  bfvEqual(b, init->reg[A], final->reg[B]),
                  bfvEqual(b, init->reg[B], final->reg[A]));

  literal c2 =  bfNewAnd(b,
                  bfvEqual(b, init->reg[C], final->reg[D]),
                  bfvEqual(b, init->reg[D], final->reg[C]));

  /* return c1; */
  return bfNewAnd(b, c1, c2);
}

machine_state *swapInput(bfman *b, unsigned ain, unsigned bin, 
                             unsigned cin, unsigned din)
{
  machine_state *mach = bfmsInit(b, 4, ADDR_NUM_BITS, REG_NUM_BITS);
  bfRegStore_le(b, mach, A, bfvUconstant(b, REG_NUM_BITS, ain));
  bfRegStore_le(b, mach, B, bfvUconstant(b, REG_NUM_BITS, bin));
  bfRegStore_le(b, mach, C, bfvUconstant(b, REG_NUM_BITS, cin));
  bfRegStore_le(b, mach, D, bfvUconstant(b, REG_NUM_BITS, din));
  return mach;
}

/*
 * Inputs can be arbitrary, so, we make every register symbolic.
 */
machine_state *swapInitialState(bfman *b)
{
  machine_state *mach = swapInput(b, 0, 0, 0, 0);
  bfRegStore_le(b, mach, A, bfvInit(b, REG_NUM_BITS));
  bfRegStore_le(b, mach, B, bfvInit(b, REG_NUM_BITS));
  bfRegStore_le(b, mach, C, bfvInit(b, REG_NUM_BITS));
  bfRegStore_le(b, mach, D, bfvInit(b, REG_NUM_BITS));
  return mach;
}


int main()
{
  bfman *b = bfInit(AIG_MODE);
  bfEnableDebug(b, "program", 2);

  specification swapSpec;
  memset(&swapSpec, 0, sizeof(swapSpec));
  swapSpec.sanityCheckFinalStates = true;
  swapSpec.minlen = 0;
  swapSpec.maxlen = 10;
  swapSpec.testVectors = vectorAlloc(10);
  /* vectorPush(swapSpec.testVectors, swapInput(b, 0, 0, 0, 0)); */
  vectorPush(swapSpec.testVectors, swapInput(b, 1, 2, 3, 4));
  vectorPush(swapSpec.testVectors, swapInput(b, 0x47, 0x01, 3, 4));
  swapSpec.initialState = swapInitialState(b);
  swapSpec.oracle = swapOracle;
  swapSpec.instructions = vectorAlloc(10);
  instruction *insn = swapInstructions;
  while (insn != swapInstructions+10) {
    vectorPush(swapSpec.instructions, insn++);
  }

  program *swapProgram = bfSynthesizeProgram(b, &swapSpec);
  if (!swapProgram) {
    printf("Synthesis failed\n");
  } else {
    printf("Yay!\n");
    bfpPrint(b, stderr, &swapSpec, swapProgram);
    bfpPrintStats(b, stderr, &swapSpec, swapProgram);
  }

  machine_state **tv;
  forVector (machine_state **, tv, swapSpec.testVectors) {
    bfmsDestroy(b, *tv);
  }
  bfmsDestroy(b, swapSpec.initialState);
  vectorDestroy(swapSpec.testVectors), free(swapSpec.testVectors);
  vectorDestroy(swapSpec.instructions), free(swapSpec.instructions);
  bfpDestroy(b, swapProgram, &swapSpec);

  bfDestroy(b);

  return NULL == swapProgram;
}
