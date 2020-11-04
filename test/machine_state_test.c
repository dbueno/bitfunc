#include <bitfunc/config.h>

#include <assert.h>
#include <bitfunc.h>
#include <bitfunc/machine_state.h>
#include <bitfunc/bitvec.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "genrand.h"
#include "mitre.h"

static int test_reg_be(const uint32_t sz)
{
  int ret = 0;
  bfman *bf = bfInit(AIG_MODE);
  machine_state *st = bfmsInit(bf, sz, 32, 8);
  bitvec *deadbeef = bfvHold(bfvUconstant(bf, 32, 0xdeadbeef));
  bitvec *ld;
  unsigned addr;
  
  for (int i = 0; i < 100; i++) {
    long int n = randomPosNum(sz-4);
    bfRegStore_be(bf, st, n, deadbeef);
    bitvec *ld = bfRegLoad_be(bf, st, n, 4);
    if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, deadbeef));
    /* bfvPrintConcrete(bf, stderr, ld, PRINT_HEX), fprintf(stderr, "\n"); */
  }

  addr = 0;
  bfRegStore_be(bf, st, addr, deadbeef);
  
  ld = bfRegLoad_be(bf, st, addr, 1);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, 8, 0xde)));

  addr++;
  ld = bfRegLoad_be(bf, st, addr, 1);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, 8, 0xad)));

  addr++;
  ld = bfRegLoad_be(bf, st, addr, 1);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, 8, 0xbe)));

  addr++;
  ld = bfRegLoad_be(bf, st, addr, 1);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, 8, 0xef)));


  CONSUME_BITVEC(bfvRelease(deadbeef));
  bfmsDestroy(bf, st);
  bfDestroy(bf);
  return ret;
}

static int test_reg_le(const uint32_t sz)
{
  int ret = 0;
  bfman *bf = bfInit(AIG_MODE);
  machine_state *st = bfmsInit(bf, sz, 32, 8);
  bitvec *deadbeef = bfvHold(bfvUconstant(bf, 32, 0xdeadbeef));
  bitvec *ld;
  unsigned addr;
  
  for (int i = 0; i < 100; i++) {
    long int n = randomPosNum(sz-4);
    bfRegStore_le(bf, st, n, deadbeef);
    bitvec *ld = bfRegLoad_le(bf, st, n, 4);
    if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, deadbeef));
    /* bfvPrintConcrete(bf, stderr, ld, PRINT_HEX), fprintf(stderr, "\n"); */
  }

  addr = 0;
  bfRegStore_le(bf, st, addr, deadbeef);
  
  ld = bfRegLoad_le(bf, st, addr, 1);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, 8, 0xef)));

  addr++;
  ld = bfRegLoad_le(bf, st, addr, 1);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, 8, 0xbe)));

  addr++;
  ld = bfRegLoad_le(bf, st, addr, 1);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, 8, 0xad)));

  addr++;
  ld = bfRegLoad_le(bf, st, addr, 1);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, 8, 0xde)));


  CONSUME_BITVEC(bfvRelease(deadbeef));
  bfmsDestroy(bf, st);
  bfDestroy(bf);
  return ret;
}

int main(int UNUSED(argc), char **UNUSED(argv))
{
  int ret = 0;
  srandom(1);

  if (ret==0) ret = test_reg_be(8);
  if (ret==0) ret = test_reg_be(32);
  if (ret==0) ret = test_reg_le(8);
  if (ret==0) ret = test_reg_le(32);

  return ret;
}
