#ifndef proof_h_included
#define proof_h_included

#include "config.h"
#include "funcsat/types.h"

void outputResOp(
  funcsat *f,
  literal clashLit,
  clause *op1,
  clause *op2,
  clause *out);

#endif
