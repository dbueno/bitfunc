

#include <bitfunc/config.h>

#include <assert.h>
#include <bitfunc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define UNUSED(x) (void)(x)

#define BITS_IN_WORD 3
#define CONST_MASK 0xffffffff   /* can be adjusted when BITS_IN_WORD is < 32 */
#define uconst(x) bfvUconstant(b, BITS_IN_WORD, (x) & CONST_MASK)

int main(int argc, char **argv)
{
  UNUSED(argc), UNUSED(argv);
  bfman *b = bfInit(AIG_MODE);
  /* use for SPEEDZORS */
#ifdef INCREMENTAL_PICOSAT_ENABLED
  bfConfigurePicosatIncremental(b);
#endif
  
  //bfConfigureLingeling(b);

  bitvec *A = bfvHold(bfvInit(b, BITS_IN_WORD));
  bitvec *B = bfvHold(bfvInit(b, BITS_IN_WORD));

  bfPushAssumption(b, bfvUlt(b, bfvUconstant(b, A->size, 0), A));
  bfPushAssumption(b, bfvUlt(b, bfvUconstant(b, B->size, 0), B));


  bfresult result;
  while (true) {
      result = bfPushAssumption(b, -bfvEqual(b, A, B));
      if (result == BF_UNSAT)
        break;
      printf("solving...\n");

      result = bfSolve(b);
      if (result == BF_UNSAT)
        break;
      
      bitvec *aminusb = bfvSubtract(b, A, B);
      bitvec *bminusa = bfvSubtract(b, B, A);
      A = bfvHold(bfvSelect(b, bfvUlt(b, B, A), aminusb, A));
      B = bfvHold(bfvSelect(b, bfvUlt(b, B, A), B, bminusa));

  }

  int ret = 1;
  if (result == BF_SAT) {
    printf("not equivalent on ");
    printf("\n");
  } else if (result == BF_UNSAT) {
    ret = 0;
    printf("equivalent!\n");
  } else {
    printf("error!\n");
  }



  bfDestroy(b);
  return ret;
}

int main2(int argc, char **argv)
{
  UNUSED(argc), UNUSED(argv);
  bfman *b = bfInit(AIG_MODE);
  /* use for SPEEDZORS */
  bfConfigurePicosatIncremental(b);
  //bfConfigureLingeling(b);

  bitvec *A = bfvHold(bfvInit(b, BITS_IN_WORD));
  bitvec *B = bfvHold(bfvInit(b, BITS_IN_WORD));
  bitvec *zero = bfvHold(bfvUconstant(b, BITS_IN_WORD, 0));

  bfresult result;
  while (true) {
      bfPushAssumption(b, -bfvEqual(b, zero, B));
      printf("solving...\n");
      result = bfSolve(b);
      if (result == BF_UNSAT)
        break;
      bitvec *T = bfvHold(B);
      bitvec *Q, *R;
      bfvQuotRem(b, A, B, &Q, &R);
      B = bfvHold(R);
      A = T;
  }

  int ret = 1;
  if (result == BF_SAT) {
    printf("not equivalent on ");
    printf("\n");
  } else if (result == BF_UNSAT) {
    ret = 0;
    printf("equivalent!\n");
  } else {
    printf("error!\n");
  }



  bfDestroy(b);
  return ret;
}
