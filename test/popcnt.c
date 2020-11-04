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

#define UNUSED(x) (void)(x)

#define BITS_IN_WORD 32
#define CONST_MASK 0xffffffff   /* can be adjusted when BITS_IN_WORD is < 32 */
#define uconst(x) bfvUconstant(b, BITS_IN_WORD, (x) & CONST_MASK)

extern bitvec *popcnt_x86(bfman *b, bitvec *ebx);
extern bitvec *popcnt_loop(bfman *b, bitvec *x);


int main(int argc, char **argv)
{
  UNUSED(argc), UNUSED(argv);
  bfman *b = bfInit(AIG_MODE);
  /* use for SPEEDZORS */
#ifdef INCREMENTAL_PICOSAT_ENABLED
  bfConfigurePicosatIncremental(b);
#endif

  bitvec *input = bfvHold(bfvInit(b, BITS_IN_WORD));

  bitvec *x86_out = popcnt_x86(b, input);
  bitvec *loop_out = popcnt_loop(b, input);

  bfPushAssumption(b, -bfvEqual(b, x86_out, loop_out));
  printf("solving...\n");
  bfresult result = bfSolve(b);

  int ret = 1;
  if (result == BF_SAT) {
    printf("not equivalent on ");
    bitvec *cx = bfvFromCounterExample(b, input);
    bfvPrintConcrete(b, stdout, cx, PRINT_HEX);
    printf("\n");
  } else if (result == BF_UNSAT) {
    ret = 0;
    printf("equivalent!\n");
  } else {
    printf("error!\n");
  }


  CONSUME_BITVEC(bfvRelease(input));

  bfDestroy(b);
  return ret;
}
