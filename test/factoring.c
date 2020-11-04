#include <bitfunc.h>
#include <stdio.h>
#include <stdlib.h>

#define BITS 800

int main(int argc, char **argv)
{
  fprintf(stderr, "using %d bits\n", BITS);

  bfman *BF = bfInit(AIG_MODE);
  bfConfigurePicosat(BF);
  bitvec *x = bfvHold(bfvInit(BF, BITS));
  bitvec *y = bfvHold(bfvInit(BF, BITS));
  bitvec *z = bfvMul(BF, x, y);
  bitvec *one = bfvHold(bfvUconstant(BF, BITS, 1));

  bitvec *expected = bfvAlloc(BF, z->size);
  for (unsigned i = 0; i < BITS; i++) {
    bfvPush(expected, random() % 2 ? LITERAL_TRUE : LITERAL_FALSE);
  }
  fprintf(stderr, "expected = ");
  bfvPrintConcrete(BF, stderr, expected, PRINT_HEX);
  fprintf(stderr, "\n");

  if (BF_UNSAT == bfPushAssumption(BF, bfvEqual(BF, z, expected))) {
    goto unsat;
  }

  if (BF_UNSAT == bfPushAssumption(BF, -bfvEqual(BF, x, one))) {
    goto unsat;
  }

  if (BF_UNSAT == bfPushAssumption(BF, -bfvEqual(BF, y, one))) {
    goto unsat;
  }

  fprintf(stderr, "solving...\n");
  bfresult result = bfSolve(BF);
  if (result == BF_SAT) {
    fprintf(stderr, "x = ");
    bfvPrintConcrete(BF, stderr, x, PRINT_HEX);
    fprintf(stderr, "\n");
    fprintf(stderr, "y = ");
    bfvPrintConcrete(BF, stderr, y, PRINT_HEX);
    fprintf(stderr, "\n");
    goto end;
  }

unsat:
  fprintf(stderr, "unsat\n");

end:
  return 0;
}
