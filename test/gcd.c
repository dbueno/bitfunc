#include <bitfunc/config.h>

#include <assert.h>
#include <bitfunc.h>
#include <bitfunc/array.h>
#include <bitfunc/aiger.h>
#include <bitfunc/circuits.h>
#include <bitfunc/machine_state.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define UNUSED(x) (void)(x)

#define ADDR_WIDTH 32
#define BITS_IN_BYTE 8

#define addr_const(x) bfvUconstant(b, ADDR_WIDTH, (x))

machine_state *GCD_b0(bfman *b, machine_state *ms_in);
machine_state *GCD_bn0(bfman *b, machine_state *ms_in);


/* GCD - greatest common divisor

A common algorithm. One of the ones that isn't symbolically terminating, so you
have to query the solver for the termination of the analysis. Not too bad. */

machine_state *GCD(bfman *b, machine_state *ms)
{
  static int ctr = 0;
  bitvec *y = bfvHold(bfmLoad_le(b, ms->ram, addr_const(0x8), 1));
  bitvec *c_zero = bfvUconstant(b, BITS_IN_BYTE, 0);
  literal is_zero = bfvEqual(b, y, c_zero);

  bfvPrint(stderr, y), fprintf(stderr, "\n");
  CONSUME_BITVEC(bfvRelease(y));

  bfresult result = BF_UNSAT;
  ctr++;
  fprintf(stderr, "ctr == %d\n", ctr);
  if (ctr != 13) result = BF_SAT;
  /* if (BF_UNSAT != bfPushAssumption(b, -is_zero)) { */
  /*   fprintf(stderr, "solving... "); */
  /*   result = bfSolve(b); */
  /* } */

  if (BF_UNSAT == result) {
    fprintf(stderr, "UNSAT!\n");
    return ms;
  }
  fprintf(stderr, "SAT\n");

  machine_state *ms_b0 = GCD_b0(b, bfmsCopy(b, ms));
  machine_state *ms_bn0 = GCD_bn0(b, bfmsCopy(b, ms));
  machine_state *ret = bfmsSelect(b, is_zero, ms_b0, ms_bn0);

  bfmsDestroy(b, ms);
  return ret;
}

/* y==0 -> return x */
machine_state *GCD_b0(bfman *b, machine_state *ms)
{
  bitvec *x = bfmLoad_le(b, ms->ram, addr_const(0x4), 1);
  bfmStore_le(b, ms->ram, addr_const(0x0), x);
  return ms;
}

/* y!=0 -> x=y, y=x mod y */
machine_state *GCD_bn0(bfman *b, machine_state *ms)
{
  bitvec *x = bfmLoad_le(b, ms->ram, addr_const(0x4), 1);
  bitvec *y = bfvHold(bfmLoad_le(b, ms->ram, addr_const(0x8), 1));

  bitvec *x_mod_y = bfvRem(b, x, y);

  bfmStore_le(b, ms->ram, addr_const(0x4), bfvRelease(y));
  bfmStore_le(b, ms->ram, addr_const(0x8), x_mod_y);

  return GCD(b, ms);
}


int main(int argc, char **argv)
{
  UNUSED(argc), UNUSED(argv);
  bfman *b = bfInit(AIG_MODE);
  machine_state *ms = bfmsInit(b, 0, ADDR_WIDTH, BITS_IN_BYTE);

  bitvec *x = bfvInit(b, BITS_IN_BYTE);
  bitvec *y = bfvInit(b, BITS_IN_BYTE);

  bfmStore_le(b, ms->ram, addr_const(0x4), x);
  bfmStore_le(b, ms->ram, addr_const(0x8), y);

  bfmPrint(b, stderr, ms->ram);

  ms = GCD(b, ms);

  bfmPrint(b, stderr, ms->ram);

  bitvec *ret = bfmLoad_le(b, ms->ram, addr_const(0x0), 1);

  bfmsDestroy(b, ms);
  CONSUME_BITVEC(ret);
  bfDestroy(b);

  return 0;
}

