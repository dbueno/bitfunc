#include <bitfunc/config.h>

#include "mitre.h"

mitre *mitre_init(const char *name, unsigned width)
{
  mitre *m;
  CallocX(m, 1, sizeof(*m));

  m->width = width;
  m->name = name;
  vectorInit(&m->inputNames, 10);
  vectorInit(&m->inputs,     10);
  m->man = bfInit(AIG_MODE);
  /* bfConfigurePicosat(m->man); */
  /* bfConfigureLingeling(m->man); */

#ifdef INCREMENTAL_PICOSAT_ENABLED
  bfConfigurePicosatIncremental(m->man);
#endif

  bfEnableDebug(m->man, "bf", 0);
  assert(m->man->funcsat);

  /* m->man->funcsat->conf->gc = false; */

  return m;
}

void mitre_destroy(mitre *m)
{
  bfDestroy(m->man);
  bitvec **bit;
  forVector (bitvec **, bit, &m->inputs) {
    assert((*bit)->numHolds == 1);
    CONSUME_BITVEC(bfvRelease(*bit));
  }
  vectorDestroy(&m->inputs);
  vectorDestroy(&m->inputNames);
  free(m);
}

bitvec *mitre_addInput(mitre *m, const char *name, bitvec *b)
{
  bfvHold(b);
  b->name = strdup(name);
  vectorPush(&m->inputNames, (char *)name);
  vectorPush(&m->inputs, b);
  return b;
}

literal mitre_addInputLit(mitre *m, const char *name, literal l)
{
  bitvec *b = bfvAlloc(1);
  bfvPush(b, l);
  mitre_addInput(m, name, b);
  return l;
}

/// tests whether the mitre'd circuit is unsatisfiable
int mitre_run(mitre *m)
{
  clock_t start=0, end=0;
  int ret = 0;
  printf("Running %s", m->name);
  // we require the mitre output to be false and then test for unsatisfiability
  if (m->width > 1) printf(" (%u bits)", m->width);
  printf(" ...");
  fflush(stdout);
  assert(m->output);
  assert(m->name);
  /* assert(m->width); */
  assert(m->man);

  if (bf_litIsFalse(-m->output)) goto Passed;
  else if (bf_litIsTrue(-m->output)) goto Failed;

  /* assume output is false */
  bfPushAssumption(m->man, -m->output);
  
  start = clock();
  bfresult result = bfSolve(m->man);
  end = clock();

  switch (result) {
  case BF_SAT: {
Failed:
    printf("\nMITRE TEST FAILURE: counterexample found\n");
    printf("  MSB .. LSB\n");
    bitvec **elt;
    char **name = (char **) m->inputNames.data;
    forVector(bitvec **, elt, &m->inputs) {
      uint32_t cnt = 0;
      printf("  %s ", *name);
      bitvec *thisone = *elt;
      bfvPrint(stdout, thisone);
      printf(": ");
      literal *elt2;
      forBvRev(elt2, thisone) {
        literal l = *elt2;
        mbool val = bfGet(m->man, l);
        printf("%s", val == true ? "1" : (val == false ? "0" : "unknown"));
        if ((++cnt % 4) == 0) printf(" ");
      }
      bitvec *signedBv = bfvExtend(m->man, 64, thisone, EXTEND_SIGN);
      bitvec *unsignedBv = bfvExtend(m->man, 64, thisone, EXTEND_ZERO);
      int64_t sv = (int64_t) bfvGet(m->man, signedBv);
      uint64_t uv = bfvGet(m->man, unsignedBv);
      printf("  [unsigned: 0x%016" PRIu64 " / signed: %" PRIi64 "]\n", uv, sv);
      ++name;
    }
    ret = 1;
    break;
  }

  case BF_UNSAT:
Passed:
    printf(" passed. (%g sec)\n", (float)(end-start)/CLOCKS_PER_SEC);
    /* funcsatPrintStats(stderr, m->man->funcsat); */
    ret = 0;
    break;

  default:
    //case P_ERROR:
    printf(" MITRE ERROR (%d)\n", result);
    ret = 1;
    break;

  }

  mitre_destroy(m);
  return ret;
}
