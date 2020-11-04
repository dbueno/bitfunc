#include <bitfunc/config.h>

#include <assert.h>
#include <bitfunc.h>
#include <bitfunc/array.h>
#include <bitfunc/aiger.h>
#include <bitfunc/circuits.h>
#include <bitfunc/mem.h>
#include <funcsat.h>
#include <funcsat/hashtable.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mitre.h"

/* ====================================================================== */
/* tests */

int testmemory(const uint32_t width)
{
  printf("testmemory\n");
  bfman *b = bfInit(AIG_MODE);
  memory *mem = bfmInit(b, width, width);

  bitvec *fortytwo = bfvHold(bfvUconstant(b, width, 42));
  bitvec *sym1 = bfvHold(bfvInit(b, width));
  bitvec *sym2 = bfvAdd0(b, sym1, sym1);

  bfmStore(b, mem, fortytwo, fortytwo);
  bitvec *out = bfmLoad(b, mem, fortytwo);
  bfvPrintConcrete(b, stderr, out, PRINT_HEX), fprintf(stderr, "\n");
  assert(bfvGet(b, out) == 42);
  CONSUME_BITVEC(out);

  bfmStore(b, mem, sym1, fortytwo);
  out = bfmLoad(b, mem, fortytwo);
  bfvPrintConcrete(b, stderr, out, PRINT_HEX), fprintf(stderr, "\n");
  assert(bfvGet(b, out) == 42);
  CONSUME_BITVEC(out);
  memory *mem2 = bfmCopy(b, mem);

  bfmStore(b, mem2, sym1, sym2);
  out = bfmLoad(b, mem2, fortytwo);
  bfvPrint(stderr, out), fprintf(stderr, "\n");
  CONSUME_BITVEC(out);

  CONSUME_BITVEC(bfvRelease(fortytwo));
  CONSUME_BITVEC(bfvRelease(sym1));

  bfDestroy(b);
  return 0;
}

int testrbw(const uint32_t valsz)
{
  printf("testrbw\n");
  const uint8_t addrsz = 32;
  bfman *b = bfInit(AIG_MODE);
  memory *mem = bfmInit(b, 32, valsz);

  bitvec *fortytwo = bfvHold(bfvUconstant(b, addrsz, 42));
  literal rbw = LITERAL_FALSE;
  CONSUME_BITVEC(bfmLoad_le_RBW(b, mem, fortytwo, 4, &rbw));
  assert(rbw == LITERAL_TRUE);

  bitvec *symaddr = bfvHold(bfvInit(b, addrsz));
  bitvec *symaddr2 = bfvHold(bfvInit(b, addrsz));
  bitvec *val = bfvHold(bfvUconstant(b, valsz, 4));
  bfmStore_le(b, mem, symaddr, val);
  CONSUME_BITVEC(bfmLoad_le_RBW(b, mem, symaddr2, 1, &rbw));
  fprintf(stderr, "%u\n", rbw);
  if (BF_UNSAT != bfPushAssumption(b, rbw)) {
    bfresult result;
    bitvec *tmp;
    fprintf(stderr, "  solving");
    result = bfSolve(b);
    assert(result == BF_SAT);
    fprintf(stderr, "  %s\n", bfResultAsString(result));
    fprintf(stderr, "symaddr  ");
    bfvPrintConcrete(b, stderr, tmp = bfvFromCounterExample(b, symaddr), PRINT_HEX);
    CONSUME_BITVEC(tmp);
    fprintf(stderr, "\n");
    fprintf(stderr, "symaddr2 ");
    bfvPrintConcrete(b, stderr, tmp = bfvFromCounterExample(b, symaddr2), PRINT_HEX);
    CONSUME_BITVEC(tmp);
    fprintf(stderr, "\n");
  }
  if (BF_UNSAT != bfPushAssumption(b, bfvEqual(b, symaddr, symaddr2))) {
    bfresult result;
    bitvec *tmp;
    fprintf(stderr, "  solving");
    result = bfSolve(b);
    fprintf(stderr, "  %s\n", bfResultAsString(result));
    assert(result == BF_UNSAT);
    if (result != BF_UNSAT) {
      fprintf(stderr, "symaddr  ");
      bfvPrintConcrete(b, stderr, tmp = bfvFromCounterExample(b, symaddr), PRINT_HEX);
      fprintf(stderr, "\n");
      fprintf(stderr, "symaddr2 ");
      bfvPrintConcrete(b, stderr, tmp = bfvFromCounterExample(b, symaddr2), PRINT_HEX);
      fprintf(stderr, "\n");
    }
  }
  bfPopAssumptions(b, b->assumps->size);
  if (BF_UNSAT != bfPushAssumption(b, -rbw)) {
    bfresult result;
    bitvec *sa, *sa2;
    result = bfSolve(b);
    fprintf(stderr, "  %s\n", bfResultAsString(result));
    fprintf(stderr, "symaddr  ");
    bfvPrintConcrete(b, stderr, sa = bfvFromCounterExample(b, symaddr), PRINT_HEX);
    fprintf(stderr, "\n");
    fprintf(stderr, "symaddr2 ");
    bfvPrintConcrete(b, stderr, sa2 = bfvFromCounterExample(b, symaddr2), PRINT_HEX);
    fprintf(stderr, "\n");
    assert(LITERAL_TRUE == bfvEqual(b, sa, sa2));
  }

  CONSUME_BITVEC(bfvRelease(val));
  CONSUME_BITVEC(bfvRelease(fortytwo));
  CONSUME_BITVEC(bfvRelease(symaddr));
  CONSUME_BITVEC(bfvRelease(symaddr2));
  bfDestroy(b);

  return 0;
}

int test_mem_be(const uint32_t UNUSED(sz))
{
  int ret = 0;
  const uint8_t addrsz = 32;
  const uint8_t valusz = 8;
  bfman *bf = bfInit(AIG_MODE);
  memory *mem = bfmInit(bf, addrsz, valusz);
  bitvec *ld;

  printf("test_mem_be\n");

  bitvec *addr = bfvHold(bfvUconstant(bf, addrsz, 0xdeadbeef));
  bitvec *valu = bfvHold(bfvUconstant(bf, 32, 0xcafebabe));
  bfmStore_be(bf, mem, addr, valu);
  bfmPrint(bf, stderr, mem);

  ld = bfmLoad(bf, mem, addr);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, valusz, 0xca)));

  addr = bfvHold(bfvIncr(bf, bfvRelease(addr)));
  ld = bfmLoad(bf, mem, addr);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, valusz, 0xfe)));

  addr = bfvHold(bfvIncr(bf, bfvRelease(addr)));
  ld = bfmLoad(bf, mem, addr);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, valusz, 0xba)));

  addr = bfvHold(bfvIncr(bf, bfvRelease(addr)));
  ld = bfmLoad(bf, mem, addr);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, valusz, 0xbe)));

  CONSUME_BITVEC(bfvRelease(addr));
  CONSUME_BITVEC(bfvRelease(valu));
  bfmDestroy(bf, mem);
  bfDestroy(bf);
  return ret;
}

int test_mem_le(const uint32_t UNUSED(sz))
{
  int ret = 0;
  const uint8_t addrsz = 32;
  const uint8_t valusz = 8;
  bfman *bf = bfInit(AIG_MODE);
  memory *mem = bfmInit(bf, addrsz, valusz);
  bitvec *ld;

  printf("test_mem_be\n");

  bitvec *addr = bfvHold(bfvUconstant(bf, addrsz, 0xdeadbeef));
  bitvec *valu = bfvHold(bfvUconstant(bf, 32, 0xcafebabe));
  bfmStore_le(bf, mem, addr, valu);
  /* bfmPrint(bf, stderr, mem); */

  ld = bfmLoad(bf, mem, addr);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, valusz, 0xbe)));

  addr = bfvHold(bfvIncr(bf, bfvRelease(addr)));
  ld = bfmLoad(bf, mem, addr);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, valusz, 0xba)));

  addr = bfvHold(bfvIncr(bf, bfvRelease(addr)));
  ld = bfmLoad(bf, mem, addr);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, valusz, 0xfe)));

  addr = bfvHold(bfvIncr(bf, bfvRelease(addr)));
  ld = bfmLoad(bf, mem, addr);
  if (ret==0) ret = !(LITERAL_TRUE == bfvEqual(bf, ld, bfvUconstant(bf, valusz, 0xca)));

  CONSUME_BITVEC(bfvRelease(addr));
  CONSUME_BITVEC(bfvRelease(valu));
  bfmDestroy(bf, mem);
  bfDestroy(bf);
  return ret;
}

static int runTests()
{
  int ret = 0;
  srandom(1);

  if (ret==0) ret = testmemory(8);
  if (ret==0) ret = testrbw(8);
  if (ret==0) ret = test_mem_be(8);
  if (ret==0) ret = test_mem_be(32);
  if (ret==0) ret = test_mem_le(8);
  return ret;

}

static inline float toseconds(clock_t t) {
  return (float)t/CLOCKS_PER_SEC;
}

int main(int UNUSED(argc), char **UNUSED(argv))
{
  int ret = 0;
  clock_t start, end;
  clock_t aig = 0, cnf = 0;

  fprintf(stderr, "Running AIG mode tests ...\n");
  start = clock();
  if (ret==0) ret = runTests();
  end = clock();
  aig = end-start;
  fprintf(stderr, "AIG time: %g\n", toseconds(aig));

  return ret;
}

