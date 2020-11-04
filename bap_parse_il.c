/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */

#include <bitfunc/config.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <funcsat/hashtable_itr.h>
#include "bitfunc/sparsebits.h"
#include "bitfunc/circuits.h"
#include "bitfunc/bap.h"
#include "bitfunc.h"

extern void dump_sbs(sparse_bitset *);

void dump_mem(bfman *bf, memory *mem)
{
  sparse_bitset *addrs = sbs_create();
  memory *cur = mem;
  while (cur) {
    memoryCell **mcIt;
    forVector (memoryCell **, mcIt, cur->arr) {
      uint64_t addr = bfvGet(bf, (*mcIt)->index);
      if (!sbs_test(addrs, addr)) {
        printf("[0x%08" PRIx64 "] = 0x%02" PRIx64 "\n", addr, bfvGet(bf, (*mcIt)->value));
        sbs_set(addrs, addr);
      }
    }
    if (bfGet(bf, cur->c)) {
      cur = cur->memt;
    } else {
      cur = cur->memf;
    }
  }
  sbs_destroy(addrs);
}

int main(int argc, char **argv)
{

  if (argc <= 1) {
    fprintf(stderr, "please supply a BAP IL file to parse.\n");
    exit(1);
  }

  sparse_bitset *flipped_constraints = sbs_create();
  for (int i = 2; i < argc; ++i) {
    sbs_set(flipped_constraints, atoi(argv[i]));
  }

  fprintf(stderr, "parsing '%s' ...\n", argv[1]);

  //ParseTrace(stderr, "    lmn: ");

  bfman *bf = bfInit(AIG_MODE);

  bap_machine *mach = bap_create_machine(bf);

  bap_execute_il(mach, argv[1]);

  symbol_table s = mach->bindings;

  if (!mach->has_error) {
    printf("solving...");
    for (size_t i = 0; i < mach->path_constraints->size; ++i) {
      if (sbs_test(flipped_constraints, i)) {
        bfPushAssumption(mach->bf, -mach->path_constraints->data[i]);
      } else {
        bfPushAssumption(mach->bf, mach->path_constraints->data[i]);
      }
    }
    bfresult res = bfSolve(bf);
    printf(" done\n");
    if (res != BF_SAT) {
      printf("constraint assignment is invalid\n");
    } else {

      struct hashtable_itr *i = hashtable_iterator(mach->bindings);
      do {
        bap_val *v = (bap_val*)hashtable_iterator_value(i);
        char *name = (char*)hashtable_iterator_key(i);
        printf("%s = ", name);
        fflush(stdout);
        switch (v->typ.kind) {
          case BAP_TYPE_KIND_BV:
            if (v->v.bv != NULL) {
              bfvPrintConcrete(bf, stdout, v->v.bv, PRINT_HEX);
            } else {
              fputs("bot", stdout);
            }
            putc('\n', stdout);
            break;
          case BAP_TYPE_KIND_MEM:
            dump_mem(bf, v->v.mem);
            break;
          case BAP_TYPE_KIND_STR:
            puts(v->v.str);
            break;
          case BAP_TYPE_KIND_ARRAY:
            puts("some array");
            break;
        }
      } while (hashtable_iterator_advance(i));
      free(i);

      printf("Path constraints:\n");
      bitvec *concrete_constraints = bfvFromCounterExample(mach->bf, mach->path_constraints);
      for (size_t i = 0; i < concrete_constraints->size; ++i) {
        printf(" %zu: %s\n", i, bf_litIsTrue(concrete_constraints->data[i]) ? "true" : "false");
      }
    }
  }

  bap_destroy_machine(mach);
  bfDestroy(bf);

  return 0;
}
