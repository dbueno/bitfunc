/* From Klee's test cases */

#include <bitfunc.h>
#include <bitfunc/mem.h>
#include <bitfunc/machine_state.h>

struct safe_memory
{
  memory *mem;
  bitvec *addr;
  bitvec *len;
  struct safe_memory *next;
};

static bool check_bounds(bfman *BF, struct safe_memory *m,
                         bitvec *addr, unsigned num_elts,
                         literal *ib, literal *oob)
{
  literal above = bfvUlte(BF, m->addr, addr);
  literal below = bfvUlte(BF, addr, bfvAdd0(BF, m->addr, m->len));
  literal both = bfNewAnd(BF, above, below);
  literal oob  = bfNewOr(BF, -above, -below);
  if (both == LITERAL_TRUE) {
    return bfmLoad_le(BF, m->mem, addr, num_elts);
  }
  if (BF_UNSAT != bfPushAssumption(BF, both)) {
    bfresult result = bfSolve(BF);
    literal foundthis = bfNewVar(BF);
    if (result == BF_SAT) {
      bfPopAssumptions(BF, 1);
      if (BF_UNSAT == bfPushAssumption(BF, oob)) {
      }
      result = bfSolve(BF);
      if (result == BF_SAT) {
        fprintf(stderr, "oob found\n");
      }
      bfPopAssumptions(BF, 1);
    }
  }  
}

static bitvec *safe_memory_load_le(bfman *BF, struct safe_memory *m, bitvec *addr, unsigned num_elts)
{
  literal ib = 0, oob = 0;
  if (check_bounds(BF, m, addr, num_elts, &ib, &oob)) {
    bitvec *val = bfmLoad_le(BF, m->mem, addr, num_elts);
    if (BF_UNSAT == bfPushAssumption(BF, 
      return bfvSelect(BF, foundthis, 
                       safe_memory_load_le(BF, m->next, addr, num_elts));

  return safe_memory_load_le(BF, m->next, addr, num_elts);
}

static inline void safe_memory_store_le(bfman *BF, machine_state *s,
                                    bitvec *addr, bitvec *val)
{
  
}


static bitvec *matchhere(bitvec *x);

static bitvec *matchstar(bfman *BF, machine_state *state,
                         bitvec *c, bitvec *re_addr, bitvec *text_addr)
{
  bitvec *whilecond = NULL;

  do {
    bitvec *mh = matchhere(BF, state, re_addr, text_addr);
    if (BF_UNSAT == bfPushAssumption(BF, -bfNewEqual(mh, bfvUconstant(b, 32, 0)))) {
      ;
    }
    bfresult result = bfSolve(BF);
  } while (true);
}

static bitvec *matchhere(bitvec *x);


#define SIZE 7

int main(int argc, char **argv)
{
  bfman *BF = bfInit(AIG_MODE);
  machine_state *init = bfmsInit(BF, 0, 32, 8);

  bitvec *re_addr = bfvHold(bfvUconstant(BF, 32, 0x808da0));
  bitvec *tmp = bfvCopy(re_addr);
  for (int i = 0; i < SIZE; i++) {
    bfmStore(BF, init->ram, tmp, bfvInit(BF, 8));
    tmp = bfvAdd(BF, tmp, bfvUconstant(b, 32, 0), LITERAL_TRUE);
  }
  CONSUME_BITVEC(tmp);

  bitvec *hello_addr = bfvHold(bfvUconstant(BF, 32, 0x809da0));
  tmp = bfvCopy(hello_addr);
  bfmStore(BF, init->ram, tmp, bfvUconstant(b, 8, (uint64_t)'h'));
  tmp = bfvAdd(BF, tmp, bfvUconstant(b, 32, 0), LITERAL_TRUE);
  bfmStore(BF, init->ram, tmp, bfvUconstant(b, 8, (uint64_t)'e'));
  tmp = bfvAdd(BF, tmp, bfvUconstant(b, 32, 0), LITERAL_TRUE);
  bfmStore(BF, init->ram, tmp, bfvUconstant(b, 8, (uint64_t)'l'));
  tmp = bfvAdd(BF, tmp, bfvUconstant(b, 32, 0), LITERAL_TRUE);
  bfmStore(BF, init->ram, tmp, bfvUconstant(b, 8, (uint64_t)'l'));
  tmp = bfvAdd(BF, tmp, bfvUconstant(b, 32, 0), LITERAL_TRUE);
  bfmStore(BF, init->ram, tmp, bfvUconstant(b, 8, (uint64_t)'o'));
  tmp = bfvAdd(BF, tmp, bfvUconstant(b, 32, 0), LITERAL_TRUE);
  bfmStore(BF, init->ram, tmp, bfvUconstant(b, 8, (uint64_t)'\0'));
  CONSUME_BITVEC(tmp);

  bitvec *result = match(BF, state, re_addr, hello_addr);

  return 0;
}
