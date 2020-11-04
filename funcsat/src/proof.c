/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat/config.h>

#include "funcsat/types.h"
#include "funcsat/proof.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef PROOFS

void printClause(FILE *stream, clause_t *out);

void outputResOp(
  funcsat_t *f,
  literal_t clashLit,
  clause_t *op1,
  clause_t *op2,
  clause_t *out)
{
  fprintf(
    f->conf->pf,
    "%lu %li %lu %lu ",
    out->id + f->stats->numOrigClauses, clashLit, op1->id, op2->id);
  printClause(f->conf->pf, out);
  fprintf(f->conf->pf, "\n");
}


void printClause(FILE *stream, clause_t *out)
{
  variable_t i;
  fprintf(stream, "0 ");
  for (i = 0; i < out->size; i++) {
    fprintf(stream, "%li ", out->data[i]);
  }
  fprintf(stream, "0");
}

#endif
