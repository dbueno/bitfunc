/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <strings.h>

#include <funcsat.h>
#include <funcsat/backbone.h>
#include <funcsat/internal.h>


/*
Computing backbones.

A literal $x$ is a \textit{backbone} literal of formula $\phi$ if it has the
same assignment in every model of $\phi$ \cite{Marques-Silva+:FAIA10}. This is
the same as saying that $x$ is a backbone if $\phi \wedge x$ is UNSAT.

*/




/*
how can we reduce the model.

clear model out. clear litpos. stash the model.

1. walk through the model, pushing literals. If any literal is not needed to
satisfy any clause, do not add it to the model.

2. (todo) set covering. find (greedy) minimal model that satisfies all
clauses. associate each variable with the clauses it satisfies.
*/
static inline void reduceModel(funcsat *f, litpos *model)
{
}

void computeBackbone(funcsat *f, clause *vars)
{
  funcsat_result issat;
  struct litpos *varsPos = buildLitPos(vars);
  clause *ref = clauseAlloc(f->numVars); /* reference assignment */
  struct litpos *refPos = buildLitPos(ref);
  clause *bb = clauseAlloc(vars->size);

  issat = funcsatSolve(f);
  if (issat == FS_UNSAT) return;

  reduceModel(f, refPos);

  literal *nextLit;
  forClause (nextLit, varsPos->c) {
    if (*nextLit == 0) continue;
    funcsatPushAssumption(f, *nextLit);
    issat = funcsatSolve(f);
    if (issat == FS_UNSAT) {
      clausePush(bb, -*nextLit); /* backbone identified */
      /* funcsatPopAssumptions(f, 1); */
      /* if (f->assumptions.size==0) trailPush(f, -*nextLit, NULL); */
      continue;
    }

    reduceModel(f, refPos);
    literal *lIt;
    forClause (lIt, varsPos->c) {
      if (!clauseContains(refPos, *lIt) &&
          !clauseContains(refPos, -*lIt)) {
        /* variable filtering */
        literal *lInVars = clauseLitPos(varsPos, *lIt);
        *lInVars = 0;
        lInVars = clauseLitPos(varsPos, -*lIt);
        *lInVars = 0;
      }
    }
    forClause (lIt, refPos->c) {
      if (clauseContains(varsPos, *lIt)) {
        /* variable filtering */
        literal *lInVars = clauseLitPos(varsPos, *lIt);
        *lInVars = 0;
      }
    }
  }

}
