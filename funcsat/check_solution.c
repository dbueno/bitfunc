#include <funcsat/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <funcsat.h>
#include "funcsat_internal.h"


int main(int argc, char **argv)
{
  /* argv[1] - solution */
  /* argv[2] - cnf */
  if (argc != 3) {
    printf("usage: check_solution [solution-file] [cnf]\n");
    exit(1);
  }
  assert(argc == 3);
  funcsat_config *conf = funcsatConfigInit(NULL);
  funcsat *f = funcsatInit(conf);
  printf("Parsing CNF from '%s' ...\n", argv[2]);
  parseDimacsCnf(argv[2], f);
  FILE *solfile;
  printf("Parsing solution from '%s' ...\n", argv[1]);
  if ((solfile = fopen(argv[1], "r")) == NULL) {perror("fopen"), exit(1);}
  funcsat_result result = fs_parse_dimacs_solution(f, solfile);
  fclose(solfile);

  int ret = 0;
  clause *fail;
  if (result == FS_SAT) {
    for (unsigned i = 0; i < f->origClauses.size; i++) {
      clause *c = f->origClauses.data[i];
      bool sat = false;
      for (unsigned j = 0; j < c->size; j++) {
        if (funcsatValue(f, c->data[j]) == true) {
          sat = true;
          break;
        }
      }
      if (!sat) {
        fail = c;
        ret = 1;
        break;
      }
    }
  }
  if (ret == 0) {
    fprintf(stderr, "passed!\n");
  } else {
    fprintf(stderr, "FAIL\n");
    dimacsPrintClause(stderr, fail);
    fprintf(stderr, "\n");
  }
  return ret;
}
