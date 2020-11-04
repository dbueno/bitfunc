/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <bitfunc/circuits.h>
#include <bitfunc/array.h>
#include <funcsat.h>
#include <bitfunc/mem.h>
#include <bitfunc/bitvec.h>
#include <funcsat/hashtable_itr.h>
#include <bitfunc/program.h>

/*
memory *memoryCopy(bfman *b, memory *m)
{
  memory *ret = memoryInit(b, memoryIdxSize(m), memoryEltSize(m));
  ret->memt = m;
  return ret;
}

memory *memorySelect(bfman *b, literal c, memory *t, memory *f)
{
  assert(memoryIdxSize(t) == memoryIdxSize(f));
  assert(memoryEltSize(t) == memoryEltSize(f));
  if (bf_litIsTrue(c)) return t;
  if (bf_litIsFalse(c)) return f;
  memory *r = memoryInit(b, memoryIdxSize(t), memoryEltSize(t));
  r->c = c;
  r->memt = t;
  r->memf = f;
  return r;
}
*/

void conditional_branch(bfman *b, machine_state *ms, literal c, instruction *t_branch, instruction *f_branch, uint8_t call_SAT_solver) {
  
  if(c == LITERAL_TRUE) {
    t_branch->impl(b,ms);
    return;
  } else if(c == LITERAL_FALSE) {
    f_branch->impl(b,ms);
    return;
  } else {
    //push c==LITERAL_FALSE
    if(BF_UNSAT == bfPushAssumption(b, -c)) {
      t_branch->impl(b,ms);
      return;
    } else if(call_SAT_solver && (bfSolve(b) == BF_UNSAT)) {
      //c==LITERAL_TRUE
      bfPopAssumptions(b, 1); //pop c==LITERAL_FALSE
      t_branch->impl(b,ms);
      return;
    }
    bfPopAssumptions(b, 1); //pop c==LITERAL_FALSE

    //push c==LITERAL_TRUE
    if(BF_UNSAT == bfPushAssumption(b, c)) {
      f_branch->impl(b,ms);
      return;
    } else if(call_SAT_solver && (bfSolve(b) == BF_UNSAT)) {
      //c==LITERAL_FALSE
      bfPopAssumptions(b, 1); //pop c==LITERAL_TRUE
      f_branch->impl(b,ms);
      return;
    }
    bfPopAssumptions(b, 1); //pop c==LITERAL_TRUE

    machine_state *ms_copy_t = bfmsCopy(b, ms);
    
    t_branch->impl(b, ms_copy_t);

    machine_state *ms_copy_f = bfmsCopy(b, ms);

    f_branch->impl(b, ms_copy_f);

    //no need to free m (I think).

    ms = bfmsSelect(b, c, ms_copy_t, ms_copy_f);
    
  }
  
}
