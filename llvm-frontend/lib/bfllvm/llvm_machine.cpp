#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm_machine.h"
#include <bitfunc.h>
#include <bitfunc/bitvec.h>

// limitations

// - stack & heap pointers can't alias
// - diff. heap pointers can't alias


extern "C" unsigned hashuint64(void *k);

static unsigned hashptr(void *k)
{
  uintptr_t p = (uintptr_t)k;
  uint64_t u = (uint64_t)p;
  return hashuint64(&u);
}

static int equalptr(void *k1, void *k2)
{
  return k1 == k2;
}

DEFINE_HASHTABLE(bfll_env_insert, bfll_env_search, bfll_env_remove, llvm::Value, bitvec)

bfll_machine *bfll_machine_init(bfman *BF, llvm::Module *module)
{
  bfll_machine *M;
  M = (bfll_machine*)calloc(1, sizeof(*M));

  M->BF = BF;
  assert(module->getPointerSize() == llvm::Module::Pointer32 ||
         module->getPointerSize() == llvm::Module::Pointer64);
  if (module->getPointerSize() == llvm::Module::Pointer32) M->idxsize = 32;
  else M->idxsize = 64;
  M->eltsize = 8;               // byte addressed

  M->stack = (struct bfll_stack_frame*)calloc(1, sizeof(*M->stack));
  M->stack->base = 0x0fe00000;
  M->stack->SP   = 0x0fe00000;
  M->stack->len  = 0xf000000;
  M->stack->data = bfmInit(BF, M->idxsize, M->eltsize);

  M->heap = (struct bfll_heap*)calloc(1, sizeof(*M->stack));
  M->heap->base = 0xff00000;

  M->env = create_hashtable(16, hashptr, equalptr);
  M->path_condition = NULL;

  M->BBs = NULL;                // empty

  return M;
}

bfll_machine *bfll_mkchild(bfll_machine *N)
{
  bfll_machine *M;
  M = (bfll_machine*)calloc(1, sizeof(*M));

  M->BF = N->BF;
  M->idxsize = N->idxsize;
  M->eltsize = N->eltsize;

  M->stack = (struct bfll_stack_frame*)calloc(1, sizeof(*M->stack));
  M->stack->base = N->stack->base;
  M->stack->SP   = N->stack->SP;
  M->stack->len  = N->stack->len;
  M->stack->data = bfmCopy(N->BF, N->stack->data);

  M->heap = (struct bfll_heap*)calloc(1, sizeof(*M->stack));
  M->heap->base = N->heap->base;

  M->env = create_hashtable(16, hashptr, equalptr);
  M->path_condition = N->path_condition;
  M->BBs = N->BBs;

  M->parent = N;

  return M;
}

void bfll_machine_destroy(bfll_machine *M)
{
  bfll_machine *P = M->parent;
  bfmDestroy(M->BF, M->stack->data);
  free(M->stack);
  free(M->heap);
  struct lkl_bbs *bbit, *bbnx;
  for_lkl_bbs_mutable(bbit, bbnx, M->BBs) {
    if (P && P->BBs == bbit) {
      break;
    } else lkl_bbs_destroy(bbit);
  }
  struct lkl_literal *llit, *llnx;
  for_lkl_literal_mutable(llit, llnx, M->path_condition) {
    if (P && P->path_condition == llit) break;
    else lkl_literal_destroy(llit);
  }
  free(M);
  if (P) bfll_machine_destroy(P);
}


bitvec *bfll_alloca(bfll_machine *M, uint64_t num_bytes)
{
  bitvec *ret = bfvUconstant(M->BF, M->idxsize, M->stack->SP);
  M->stack->SP += num_bytes;
  return ret;
}


void bfll_env_put(bfll_machine *M, llvm::Value *val, bitvec *bv)
{
  void *found = bfll_env_search(M->env, val);
  assert(!found && "value reassigned");
  bfvHold(bv);
  bfll_env_insert(M->env, val, bv);
}

using namespace llvm;

bitvec *bfll_env_get(bfll_machine *M, llvm::Value *val)
{
  ConstantInt *ci;
  if (dyn_cast<llvm::Constant>(val)) {
    if ((ci = dyn_cast<llvm::ConstantInt>(val))) {
      return bfvUconstant(M->BF, ci->getBitWidth(), ci->getValue().getLimitedValue());
    }
    assert(false && "constant type not handled");
  }
  bitvec *bv = (bitvec*)bfll_env_search(M->env, val);
  if (!bv && M->parent) {
    return bfll_env_get(M->parent, val);
  }
  assert(bv && "get of value not put");
  return bv;
}

void bfll_store(bfll_machine *M, bitvec *addr, bitvec *val)
{
  uint64_t addr_u = 0;
  if (!bfvIsConstant(addr)) {
    bfresult result = bfSolve(M->BF);
    assert(result == BF_SAT);
  }
  addr_u = bfvGet(M->BF, addr);
  if (addr_u >= M->stack->base && addr_u <= M->stack->base+M->stack->len) {
    // stack reference
    bfmStore_le(M->BF, M->stack->data, addr, val);
  } else if (addr_u >= M->heap->base && addr_u <= M->heap->base+M->heap->len) {
    // heap reference
    for (struct bfll_heap_block *blk = M->heap->blocks; blk; blk = blk->nx) {
      if (addr_u >= blk->base && addr_u <= blk->base+blk->len) {
        // found block
        bfmStore_le(M->BF, blk->data, addr, val);
      }
    }
  } else assert(false && "neither stack nor heap");
}

bitvec *bfll_load(bfll_machine *M, bitvec *addr, uint64_t num_bytes)
{
  uint64_t addr_u = 0;
  if (!bfvIsConstant(addr)) {
    bfresult result = bfSolve(M->BF);
    assert(result == BF_SAT);
  }
  addr_u = bfvGet(M->BF, addr);
  if (addr_u >= M->stack->base && addr_u <= M->stack->base+M->stack->len) {
    // stack reference
    return bfmLoad_le(M->BF, M->stack->data, addr, num_bytes);
  } else if (addr_u >= M->heap->base && addr_u <= M->heap->base+M->heap->len) {
    // heap reference
    for (struct bfll_heap_block *blk = M->heap->blocks; blk; blk = blk->nx) {
      if (addr_u >= blk->base && addr_u <= blk->base+blk->len) {
        // found block
        return bfmLoad_le(M->BF, blk->data, addr, num_bytes);
      }
    }
  } else assert(false && "neither stack nor heap");
  abort();
}

