/* 
   See
   http://www.openrce.org/blog/view/1963/Finding_Bugs_in_VMs_with_a_Theorem_Prover,_Round_1
 */

#include <bitfunc/config.h>

#include <assert.h>
#include <bitfunc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BITS_IN_WORD 32
#define CONST_MASK 0xffffffff   /* can be adjusted when BITS_IN_WORD is < 32 */
#define uconst(x) bfvUconstant(b, BITS_IN_WORD, (x) & CONST_MASK)


/* In order to easily make sure we free everything exactly when it's needed, no
 * sooner and no later, we need to know which variable is stomped on by the
 * instruction. If we know that, we know it's the right time to Release the
 * (soon to be) old version. Then we Hold the new one until it's used for the
 * last time.
 */
#define safe_insn(out, insn) do{                \
    bfvRelease(out);                            \
    insn;                                       \
    bfvHold(out);                               \
  } while (0);

#define COPY(b, dst, src) bfvCopy(b, dst, src), CONSUME_BITVEC(src)

bitvec *popcnt_x86(bfman *b, bitvec *ebx)
{
  assert(ebx->size == BITS_IN_WORD);
  bitvec *eax = bfvAlloc(BITS_IN_WORD);
  bfvHold(eax), bfvHold(ebx);
 
/* mov eax, ebx       */
  safe_insn(eax, COPY(b, eax, ebx));
/* and eax, 55555555h */
  safe_insn(eax, eax = bfvAnd(b, eax, uconst(0x55555555)));
/* shr ebx, 1         */
  safe_insn(ebx, ebx = bfvRshiftConst(b, ebx, 1, LITERAL_FALSE));
/* and ebx, 55555555h */
  safe_insn(ebx, ebx = bfvAnd(b, ebx, uconst(0x55555555)));
/* add ebx, eax       */
  safe_insn(ebx, ebx = bfvAdd0(b, ebx, eax));
/* mov eax, ebx       */
  safe_insn(eax, COPY(b, eax, ebx));
/* and eax, 33333333h */
  safe_insn(eax, eax = bfvAnd(b, eax, uconst(0x33333333)));
/* shr ebx, 2         */
  safe_insn(ebx, ebx = bfvRshiftConst(b, ebx, 2, LITERAL_FALSE));
/* and ebx, 33333333h */
  safe_insn(ebx, ebx = bfvAnd(b, ebx, uconst(0x33333333)));
/* add ebx, eax       */
  safe_insn(ebx, ebx = bfvAdd0(b, ebx, eax));
/* mov eax, ebx       */
  safe_insn(eax, COPY(b, eax, ebx));
/* and eax, 0F0F0F0Fh */
  safe_insn(eax, eax = bfvAnd(b, eax, uconst(0x0F0F0F0F)));
/* shr ebx, 4         */
  safe_insn(ebx, ebx = bfvRshiftConst(b, ebx, 4, LITERAL_FALSE));
/* and ebx, 0F0F0F0Fh */
  safe_insn(ebx, ebx = bfvAnd(b, ebx, uconst(0x0F0F0F0F)));
/* add ebx, eax       */
  safe_insn(ebx, ebx = bfvAdd0(b, ebx, eax));
/* mov eax, ebx       */
  safe_insn(eax, COPY(b, eax, ebx));
/* and eax, 00FF00FFh */
  safe_insn(eax, eax = bfvAnd(b, eax, uconst(0x00FF00FF)));
/* shr ebx, 8         */
  safe_insn(ebx, ebx = bfvRshiftConst(b, ebx, 8, LITERAL_FALSE));
/* and ebx, 00FF00FFh */
  safe_insn(ebx, ebx = bfvAnd(b, ebx, uconst(0x00FF00FF)));
/* add ebx, eax       */
  safe_insn(ebx ,ebx = bfvAdd0(b, ebx, eax));
/* mov eax, ebx       */
  safe_insn(eax, COPY(b, eax, ebx));
/* and eax, 0000FFFFh */
  safe_insn(eax, eax = bfvAnd(b, eax, uconst(0x0000FFFF)));
/* shr ebx, 16        */
  safe_insn(ebx, ebx = bfvRshiftConst(b, ebx, 16, LITERAL_FALSE));
/* and ebx, 0000FFFFh */
  safe_insn(ebx, ebx = bfvAnd(b, ebx, uconst(0x0000FFFF)));
/* add ebx, eax       */
  safe_insn(ebx, ebx = bfvAdd0(b, ebx, eax));
/* mov eax, ebx       */
  safe_insn(eax, COPY(b, eax, ebx));
 
  CONSUME_BITVEC(bfvRelease(ebx));
  bfvRelease(eax);
  return eax;
}

bitvec *popcnt_loop(bfman *b, bitvec *x)
{
  /* int popcnt = 0; */
  /* for(int mask = 1; mask; popcnt += !!(mask & value_to_compute_popcnt_of), mask <<= 1); */
  bitvec *popcnt = uconst(0);
  bitvec *zero = bfvHold(uconst(0));
  unsigned mask = 1;
  bfvHold(x);
  for (int i = 0; i < BITS_IN_WORD; i++) {
    bitvec *is_one = bfvAnd(b, uconst(mask), x);
    popcnt = bfvAdd0(b, popcnt, bfvSelect(b, bfvEqual(b, is_one, zero), uconst(0), uconst(1)));
    mask <<= 1;
  }

  CONSUME_BITVEC(bfvRelease(zero));
  CONSUME_BITVEC(bfvRelease(x));
  return popcnt;
}
