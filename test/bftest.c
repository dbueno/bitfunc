#include <bitfunc/config.h>

#include <assert.h>
#include <bitfunc.h>
#include <funcsat.h>
#include <funcsat_internal.h>
#include <funcsat/hashtable.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "mitre.h"
#include "genrand.h"

#define ifok(ret) if ((ret) == 0)

extern bitvec *popcnt_x86(bfman *b, bitvec *ebx);

static int test_bfSimulate_popcnt()
{
  fprintf(stderr, "bfSimulate_popcnt ...\n");
  bfman *b = bfInit(AIG_MODE);

  bitvec *x = bfvHold(bfvInit(b, 32));
  bitvec *r = bfvHold(popcnt_x86(b, x));

  bool *input;
  CallocX(input, bfNumInputs(b), sizeof(*input));
  memset(input, (int)false, sizeof(*input) * bfNumInputs(b));
  int ret = 0;

  for (unsigned i = 0; i < 256; i++) {
    unsigned cnt = 0;
    for (unsigned j = 0; j < x->size; j++) {
      bool val = mumble();
      input[bfGetInputIndex(b, fs_lit2var(x->data[j]))] = val;
      if (val) cnt++;
    }

    bfSimulate(b, input);
    bitvec *rp = bfvFromCounterExample(b, r);
    bfvPrintConcrete(b, stderr, rp, PRINT_HEX);
    fprintf(stderr, " (%03x)\n", cnt);
    ifok (ret) ret = !(bfvGet(b, rp) == cnt);
    CONSUME_BITVEC(rp);
  }

  free(input);
  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(r));
  bfDestroy(b);

  return ret;
}

/* runs 256 concrete vectors through an addition function, and checks the
 * results.
 */
static int test_bfSimulate()
{
  fprintf(stderr, "bfSimulate ...\n");
  bfman *b = bfInit(AIG_MODE);

  bitvec *x = bfvHold(bfvInit(b, 128));
  bitvec *y = bfvHold(bfvInit(b, 128));
  bitvec *z = bfvHold(bfvAdd0(b, x, y));

  /* to simulate, allocate an array that can hold as many inputs as the circuit
   * takes. */
  bool *input;
  CallocX(input, bfNumInputs(b), sizeof(*input));
  memset(input, (int)false, sizeof(*input) * bfNumInputs(b));

  /* then use bfGetInputIndex to map variables to their input locations. */
  int ret = 0;
  for (unsigned i = 0; i < x->size; i++) {
    unsigned index = bfGetInputIndex(b, fs_lit2var(x->data[i]));
    /* fprintf(stderr, "index = %u\n", index); */
    input[index] = true;

    bfSimulate(b, input);
    bitvec *zp = bfvFromCounterExample(b, z);
    bfvPrintConcrete(b, stderr, zp, PRINT_HEX);
    fprintf(stderr, "\n");
    ifok (ret) ret = !(zp->data[i] == LITERAL_TRUE);
    for (unsigned j = 0; j < zp->size; j++) {
      if (j != i) {
        ifok (ret) ret = !(zp->data[j] == LITERAL_FALSE);
      }
    }
    CONSUME_BITVEC(zp);

    input[index] = false;
  }
  for (unsigned i = 0; i < y->size; i++) {
    unsigned index = bfGetInputIndex(b, fs_lit2var(y->data[i]));
    /* fprintf(stderr, "index = %u\n", index); */
    input[index] = true;

    bfSimulate(b, input);
    bitvec *zp = bfvFromCounterExample(b, z);
    bfvPrintConcrete(b, stderr, zp, PRINT_HEX);
    fprintf(stderr, "\n");
    ifok (ret) ret = !(zp->data[i] == LITERAL_TRUE);
    for (unsigned j = 0; j < zp->size; j++) {
      if (j != i) {
        ifok (ret) ret = !(zp->data[j] == LITERAL_FALSE);
      }
    }
    CONSUME_BITVEC(zp);

    input[index] = false;
  }

  CONSUME_BITVEC(bfvRelease(x));
  CONSUME_BITVEC(bfvRelease(y));
  CONSUME_BITVEC(bfvRelease(z));
  free(input);

  bfDestroy(b);

  return ret;
}

#ifdef HAVE_LIBABC

extern int bfEqCheck_ABC(aiger *aig, aiger_literal x, aiger_literal y);
int test_bfEqCheck()
{
  fprintf(stderr, "bfEqCheck ...\n");
  bfman *b = bfInit(AIG_MODE);

  bitvec *x = bfvHold(bfvInit(b, 8));
  bitvec *y = bfvHold(bfvInit(b, 8));

  bitvec *r0 = bfvMul(b, x, y);
  bitvec *r1 = bfvMul(b, y, x);

  /* returns 1 on equivalent */
  int r = bfEqCheck_ABC(b->aig, bf_lit2aiger(r0->data[r0->size-1]), bf_lit2aiger(r1->data[r1->size-1])); 

  return (r != 1);
}
#endif

#if defined(HAVE_SETJMP_LONGJMP) && defined(USE_LONGJMP)
static int test_longjmp()
{
  bfman *bf = bfInit(AIG_MODE);
  int ret = 1;

  switch (BF_SETJMP(bf)) {

  case 0: {
    bitvec *x = bfvInit(bf, 8);
    bitvec *y = bfvInit(bf, 7);
    bfvEqual(bf, x, y);
  }

  case BF_INPUTERROR:
    fprintf(stderr, "caught error: %s\n", BF_ERROR_MSG(bf));
    ret = 0;
  }

  return ret;
}
#endif

int main(int UNUSED(argc), char **UNUSED(argv))
{
  int ret = 0;
  srandom(1);

  ifok (ret) ret = test_bfSimulate();
  ifok (ret) ret = test_bfSimulate_popcnt();
#ifdef HAVE_LIBABC
  ifok (ret) ret = test_bfEqCheck();
#endif
#if defined(HAVE_SETJMP_LONGJMP) && defined(USE_LONGJMP)
  ifok (ret) ret = test_longjmp();
#endif

  return ret;
}
