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


/**
 * We solve three related problems, with some added constraints in the middle,
 * without destroying the solver, and make sure we get the right answer.
 */
static int test_incremental(const uint32_t width)
{
  int ret = 0;
  assert(width > 16);
  mitre *m = mitre_init("test_incremental", width);
  bfman *b = m->man;
  bitvec *x = bfvHold(bfvInit(b, width));
  bitvec *y = bfvHold(bfvInit(b, width));

  printf("Running test_incremental ...");
  fflush(stdout);

  literal x_gt_0 = bfvUlt(b, bfvConstant(b, width, 0, false), x);
  literal x_lt_100 = bfvUlt(b, x, bfvConstant(b, width, 100, false));
  literal y_lt_100 = bfvUlt(b, y, bfvConstant(b, width, 100, false));

  /* x1 = x + y */
  bitvec *x1 = bfvHold(bfvAdd(b,x,y, LITERAL_FALSE, NULL));
    
  /* assume x > 0, x < 100, y < 100 (to avoid overflow when adding)
     assert x1 > 0 */
  literal x1_gt_0 = bfvUlt(b, bfvConstant(b, width, 0, false), x1);

  /* it's possible for x1 = 1 if x and y are just right to overflow to 0. */
  bfPushAssumption(b, x_gt_0);
  bfPushAssumption(b, -x1_gt_0);
  if (!(BF_SAT == bfSolve(b))) {
    printf("TEST FAILURE: expected sat\n");
    ret = 1;
    goto done;
  }

  bfPushAssumption(b, x_lt_100);
  bfPushAssumption(b, y_lt_100);
  bfPushAssumption(b, x_gt_0);
  bfPushAssumption(b, -x1_gt_0);
  if (!(BF_UNSAT == bfSolve(b))) {
    printf("TEST FAILURE: expected unsat\n");
    ret = 1;
    goto done;
  }

  // x2 = x1 + 1
  bitvec *x2 = bfvHold(bfvAdd(b, x1, bfvConstant(b, width, 1, false), LITERAL_FALSE, NULL));
  literal x2_gt_0 = bfvUlt(b, bfvConstant(b, width, 0, false), x2);

  // assume a > 0, a < 100, b < 100
  // assert x2 > 0
  bfPushAssumption(b, x_lt_100);
  bfPushAssumption(b, y_lt_100);
  bfPushAssumption(b, x_gt_0);
  bfPushAssumption(b, -x2_gt_0);
  if (!(BF_UNSAT == bfSolve(b))) {
    printf("TEST FAILURE: expected unsat\n");
    ret = 1;
    goto done;
  }

  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));
  CONSUME_BITVEC(bfvRelease(x1));
  CONSUME_BITVEC(bfvRelease(x2));

done:
  if (ret==0) printf(" passed.\n");

  mitre_destroy(m);
  return ret;
}


int main(int UNUSED(argc), char **UNUSED(argv))
{
  int ret = 0;

  if (ret==0) ret = test_incremental(64);

  return ret;
}
