/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include "funcsat/config.h"

#include <assert.h>
#include <string.h>
#include <float.h>
#include <sys/resource.h>
#include <math.h>

#include "funcsat.h"
#include "funcsat/binwatch.h"
#include "funcsat/debug.h"
/* #include "funcsat/learning.h" */
#include "funcsat/posvec.h"
#include "funcsat/fibheap.h"
#include "funcsat/memory.h"
#include "funcsat/internal.h"


/**
 * Generates a clause that is 
 *   1) satisfied if a given literal violates the current assignment and
 *   2) falsified only under the current assignment.
 */
clause *funcsatSolToClause(funcsat *f) {
  clause *c = clauseAlloc(f->trail.size);
  for(unsigned i = 0; i < f->trail.size; i++)
    clausePush(c, -f->trail.data[i]);
  return c;
}

funcsat_result funcsatFindAnotherSolution(funcsat *f) {
  clause *cur_sol = funcsatSolToClause(f);
  funcsat_result res = funcsatAddClause(f, cur_sol);
  if(res == FS_UNSAT) return FS_UNSAT;
  res = funcsatSolve(f);
  return res;  
}

#define cf(func) ((func)->conf)
int funcsatSolCount(funcsat *f, clause subset, clause *lastSolution)
{
  assert(f->assumptions.size == 0);
  int count = 0;
  for (unsigned i = 0; i < subset.size; i++) {
    funcsatResize(f, fs_lit2var(subset.data[i]));
  }
 
  clause assumptions;
  clauseInit(&assumptions, subset.size);

  unsigned twopn = (unsigned) round(pow(2., (double)subset.size));
  dmsg(f, "countsol", 1, true, "%u incremental problems to solve", twopn);
  for (unsigned i = 0; i < twopn; i++) {
    dmsg(f, "countsol", 2, false, "%u: ", i);
    clauseClear(&assumptions);
    clauseCopy(&assumptions, &subset);

    /* negate literals that are 0 in n. */
    unsigned n = i;
    for (unsigned j = 0; j < subset.size; j++) {
        /* 0 bit */
      if ((n % 2) == 0) assumptions.data[j] *= -1;
      n >>= 1;
    }

    if (funcsatPushAssumptions(f, &assumptions) == FS_UNSAT) {
      continue;
    }

    Debug(f, "countsol", 2, true, dimacsPrintClause(stderr, &assumptions));

    if (FS_SAT == funcsatSolve(f)) {
      count++;
      if (lastSolution) {
        clauseClear(lastSolution);
        clauseCopy(lastSolution, &f->trail);
      }
    }
    
    funcsatPopAssumptions(f, f->assumptions.size);
  }

  clauseDestroy(&assumptions);

  return count;
}
#undef cf
