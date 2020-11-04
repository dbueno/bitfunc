#include <stdlib.h>
#include <stdio.h>

#include "funcsat.h"
#include "funcsat/internal.h"
#include "funcsat/learning.h"
#include "gen.h"

extern funcsat_config funcsatDefaultConfig;

int main(int argc, char **argv)
{
  bool passed = true;
  srandom(1);
  funcsat *f = funcsatInit(&funcsatDefaultConfig, NULL);

  unsigned i, j;
  for (i = 0; i < 100; i++) {
    clause c1, c2, r;
    clauseInit(&c1, 5);
    clauseInit(&c2, 5);
    clauseInit(&r, 5);
    genClause(&c1, 5, 20);
    genClause(&c2, 5, 20);
    variable v = lit2var(randomLit(5)) + 20;
    clausePush(&c1, var2lit(v));
    clausePush(&c2, -var2lit(v));
    clauseCopy(&r, &c1);
    resolve(f, -var2lit(v), &r, &c2);

    for (j = 0; j < r.size; j++) {
      literal p = r.data[j];
      int inC1 = findLiteral(p, &c1);
      int inC2 = findLiteral(p, &c2);
      passed = passed && (inC1 != -1 || inC2 != -1);
    }
  }

  funcsatDestroy(f);
  return passed ? 0 : 1;
}
