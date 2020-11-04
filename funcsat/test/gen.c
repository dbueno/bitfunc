
#include <assert.h>
#include "genrand.h"
#include "funcsat_internal.h"

bool mumble()
{
  long int b = labs(random()) % 2;
  assert(b == 0 || b == 1);
  return b;
}

double randomDouble()
{
  return (double) random() / (double) INT_MAX;
}

long int randomPosNum(uint32_t ub)
{
  long int r = labs(random()) % (ub-1);
  r++;
  assert(r > 0);
  return r;
}

literal randomLit(unsigned maxVar)
{
  long int p = randomPosNum(maxVar);
  long int r = randomPosNum(100);
  if (r % 2 == 0) {
    p = -p;
  }
  assert(p != 0);
  return (literal) p;
}


void genClause(clause *clause, uint32_t maxLen, unsigned maxVar)
{
  unsigned i;
  /* long int r = randomPosNum(maxLen); */
  for (i = 0; i < maxLen; i++) {
    literal p = randomLit(maxVar);
    /* dimacsPrintClause(stderr, clause); */
    /* clauseInsert(clause, p); */
    clausePush(clause, p);
  }
}
