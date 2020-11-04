#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>

#define DEBUG_TYPE "bfss"


#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Instructions.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Debug.h"
#include "bfll_bitcode.h"
#include "linklist_bbs.h"

extern "C" {
#  include <bitfunc.h>
#  include <funcsat/vec.h>
#  include "llvm_machine.h"
#  include <bitfunc/llvm_definitions.h>
}

using namespace llvm;


static void print_function_inputs(bfll_machine *M, Function *F)
{
  for (Function::arg_iterator AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI) {
    Value *AI_v = dyn_cast<Value>(AI);
    bitvec *AI_bv = bfll_env_get(M, AI_v);
    errs() << *AI_v << " = ";
    bfvPrintConcrete(M->BF, stderr, bfvFromCounterExample(M->BF, AI_bv), PRINT_HEX);
    errs() << "\n";
  }
}

// #define DEBUG(arg) do{ arg; } while (false)

/* sets the program counter in M to point to `insn' and records the BB history
 * properly. if the PC is being set because of a new conditional branch, then
 * `condition' is used for that. If `condition' is LITERAL_TRUE, it is ignored. */
static void set_PC(bfll_machine *M, Instruction *insn, literal condition)
{
  M->PC = insn;
  M->BBs = lkl_bbs_cons(insn->getParent(), M->BBs);
  assert(condition != LITERAL_FALSE);
  if (condition != LITERAL_TRUE) {
    M->path_condition = lkl_literal_cons(condition, M->path_condition);
  }
}

static void symex_loop(const char *path)
{
  bfman *BF = bfInit(AIG_MODE);

  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();

  Module *mod = ll_load_bitcode(BF, path);
  const char *func_name = "popcnt_loop";
  Function *popcnt_x86 = mod->getFunction(func_name);
  if (!popcnt_x86) {
    fprintf(stderr, "function '%s' not found in module.\n", func_name);
    return;
  }

  const TargetData &TD = TargetData(mod);

  bfll_machine *M = bfll_machine_init(BF, mod);
  Function::iterator funit = popcnt_x86->begin();
  BasicBlock::iterator bbit = funit->begin();
  set_PC(M, bbit, LITERAL_TRUE);

  // put formals in environment
  DEBUG(errs()<<"formals\n");
  for (Function::arg_iterator AI = popcnt_x86->arg_begin(),
         AE = popcnt_x86->arg_end(); AI != AE; ++AI) {
    Value *AI_v = dyn_cast<Value>(AI);
    // TODO handle allocating complexer types
    uint64_t bytes = TD.getTypeAllocSize(AI->getType());
    bitvec *bv = bfvInit(BF, bytes*BITS_IN_BYTE);
    DEBUG(bfvPrint(stderr, bv));
    DEBUG(errs()<<" for "<<*AI_v<<"\n");
    bfll_env_put(M, AI_v, bv);
  }

  // TODO put globals in environment

  vector *states = vectorAlloc(2);
  vector *states_term = vectorAlloc(2);
  vectorPush(states, M);

  do {
    M = (bfll_machine*)vectorPop(states);
    fprintf(stderr, "in state %p\n", M);
    if (M->terminated) {
      vectorPush(states_term, M);
      continue;
    }

    Instruction *I = &*M->PC;
    DEBUG(errs()<<"Executing "<<*I<<" (@"<<I<<")\n");
    switch (I->getOpcode()) {
    case Instruction::Alloca: {
      AllocaInst *AI = dyn_cast<AllocaInst>(I);
      assert(!AI->isArrayAllocation());
      uint64_t bytes = TD.getTypeAllocSize(AI->getAllocatedType());
      bitvec *addr = bfll_alloca(M, bytes);
      bfll_env_put(M, AI, addr);
      break;
    }

    case Instruction::BitCast:
      // alloca point
      DEBUG(fprintf(stderr, "alloca point\n"));
      break;

    case Instruction::Store: {
      StoreInst *SI = dyn_cast<StoreInst>(I);
      Value *val = SI->getValueOperand();
      Value *ptr = SI->getPointerOperand();
      // uint64_t bytes = TD.getTypeStoreSize(val->getType());
      bitvec *val_bv = bfll_env_get(M, val);
      bitvec *ptr_bv = bfll_env_get(M, ptr);
      bfll_store(M, ptr_bv, val_bv);
      break;
    }

    case Instruction::Load: {
      LoadInst *LI = dyn_cast<LoadInst>(I);
      Value *ptr = LI->getPointerOperand();
      PointerType *ty = dyn_cast<PointerType>(ptr->getType());
      uint64_t bytes = TD.getTypeStoreSize(ty->getElementType());
      bitvec *ptr_bv = bfll_env_get(M, ptr);
      bitvec *bv = bfll_load(M, ptr_bv, bytes);
      bfll_env_put(M, I, bv);
      break;
    }

    case Instruction::Add: {
      BinaryOperator *AI = dyn_cast<BinaryOperator>(I);
      Value *op0 = AI->getOperand(0);
      Value *op1 = AI->getOperand(1);
      bitvec *op0_bv = bfll_env_get(M, op0);
      DEBUG(bfvPrint(stderr,op0_bv));
      DEBUG(fprintf(stderr, "+"));
      bitvec *op1_bv = bfll_env_get(M, op1);
      DEBUG(bfvPrint(stderr,op1_bv));
      bitvec *out = bf_llvm_add(BF, op0_bv, op1_bv, NULL, NULL);
      DEBUG({
          fprintf(stderr, "\n  =");
          bfvPrint(stderr,out);
          fprintf(stderr, "\n");
        });
      bfll_env_put(M, I, out);
      break;
    }

    case Instruction::And: {
      BinaryOperator *AI = dyn_cast<BinaryOperator>(I);
      Value *op0 = AI->getOperand(0);
      Value *op1 = AI->getOperand(1);
      bitvec *op0_bv = bfll_env_get(M, op0);
      DEBUG(bfvPrint(stderr,op0_bv));
      DEBUG(fprintf(stderr, "&"));
      bitvec *op1_bv = bfll_env_get(M, op1);
      DEBUG(bfvPrint(stderr,op1_bv)); 
      bitvec *out = bf_llvm_and(BF, op0_bv, op1_bv);
      DEBUG({
          fprintf(stderr, "\n  =");
          bfvPrint(stderr,out);
          fprintf(stderr, "\n");
        });
      bfll_env_put(M, I, out);
      break;
    }

    case Instruction::LShr: {
      BinaryOperator *AI = dyn_cast<BinaryOperator>(I);
      Value *op0 = AI->getOperand(0);
      Value *op1 = AI->getOperand(1);
      bitvec *op0_bv = bfll_env_get(M, op0);
      DEBUG(bfvPrint(stderr,op0_bv));
      DEBUG(fprintf(stderr, ">>"));
      bitvec *op1_bv = bfll_env_get(M, op1);
      DEBUG(bfvPrint(stderr,op1_bv));
      bitvec *out = bf_llvm_lshr(BF, op0_bv, op1_bv, NULL);
      DEBUG({
          fprintf(stderr, "\n  =");
          bfvPrint(stderr,out);
          fprintf(stderr, "\n");
        });
      bfll_env_put(M, I, out);
      break;
    }

    case Instruction::SExt: {
      SExtInst *SE = dyn_cast<SExtInst>(I);
      Value *op = SE->getOperand(0);
      Type *ty = SE->getType();
      bitvec *op_bv = bfll_env_get(M, op);
      bitvec *op_sext = bf_llvm_sext(BF, op_bv, TD.getTypeAllocSize(ty)*BITS_IN_BYTE);
      bfll_env_put(M, I, op_sext);
      break;
    }

    case Instruction::ICmp: {
      CmpInst *CE = dyn_cast<CmpInst>(I);
      CmpInst::Predicate p = CE->getPredicate();
      enum icmp_comparison cmp = ICMP_LAST;
      switch (p) {
      case CmpInst::ICMP_NE:  cmp = ICMP_NE; break;
      case CmpInst::ICMP_UGT: cmp = ICMP_UGT; break;
      case CmpInst::ICMP_UGE: cmp = ICMP_UGE; break;
      case CmpInst::ICMP_ULT: cmp = ICMP_ULT; break;
      case CmpInst::ICMP_ULE: cmp = ICMP_ULE; break;
      case CmpInst::ICMP_SGT: cmp = ICMP_SGT; break;
      case CmpInst::ICMP_SGE: cmp = ICMP_SGE; break;
      case CmpInst::ICMP_SLT: cmp = ICMP_SLT; break;
      case CmpInst::ICMP_SLE: cmp = ICMP_SLE; break;
      default: assert(false && "impossible icmp"); abort();
      }
      Value *op0 = CE->getOperand(0);
      Value *op1 = CE->getOperand(1);
      bitvec *op0_bv = bfll_env_get(M, op0);
      bitvec *op1_bv = bfll_env_get(M, op1);
      bitvec *out = bf_llvm_icmp(BF, cmp, op0_bv, op1_bv);
      bfll_env_put(M, I, out);
      break;
    }

    case Instruction::Br: {
      BranchInst *BI = dyn_cast<BranchInst>(I);
      if (BI->isUnconditional()) {
        BasicBlock *BB = BI->getSuccessor(0);
        set_PC(M, BB->begin(), LITERAL_TRUE);
        vectorPush(states, M);
        continue;
      } else {
        BasicBlock *BB_then = BI->getSuccessor(0);
        BasicBlock *BB_else = BI->getSuccessor(1);
        Value *cond = BI->getCondition();
        bitvec *cond_bv = bfll_env_get(M, cond);
        assert(cond_bv->size==1);
        literal condition = cond_bv->data[0];
        uint8_t poss = 0;       // 01b=THEN, 10b=ELSE
        bfresult result;
        if (BF_UNSAT != bfPushAssumption(BF, condition)) {
          DEBUG(errs()<<"checking 'then' branch\n");
          result = bfSolve(BF);
          DEBUG(errs()<<bfResultAsString(result)<<"\n");
          if (result == BF_SAT) {
            poss |= 1;
          }
          bfPopAssumptions(BF, BF->assumps->size);
        }
        if (BF_UNSAT != bfPushAssumption(BF, -condition)) {
          DEBUG(errs()<<"checking 'else' branch\n");
          result = bfSolve(BF);
          DEBUG(errs()<<bfResultAsString(result)<<"\n");
          if (result == BF_SAT) {
            poss |= 2;
          }
          bfPopAssumptions(BF, BF->assumps->size);
        }

        if ((poss & 0x3) == 3) {
          // both 'then' and 'else' branches are possible
          // we arbitrarily put then 'then' in the state M
          bfll_machine *M_else = bfll_mkchild(M);
          set_PC(M, BB_then->begin(), condition);
          set_PC(M_else, BB_else->begin(), -condition);
          vectorPush(states, M);
          vectorPush(states, M_else);
        } else if ((poss & 0x3) == 2) {
          // just 'else'
          bfll_machine *M_else = bfll_mkchild(M);
          set_PC(M_else, BB_else->begin(), -condition);
          vectorPush(states, M_else);
        } else if ((poss & 0x3) == 1) {
          // just 'then'
          set_PC(M, BB_then->begin(), condition);
          vectorPush(states, M);
        } else assert(false && "both branches unsat?");
        continue;
      }
    }

    case Instruction::PHI: {
      PHINode *PN = dyn_cast<PHINode>(I);
      bool found = false;
      struct lkl_bbs *h = M->BBs;
      // todo: look only at the "last" basic block
      while (h) {
        for (unsigned i = 0; i < PN->getNumIncomingValues(); i++) {
          if (h->data == PN->getIncomingBlock(i)) {
            found = true;
            break;
          }
        }
        if (found) break;
        h = h->next;
      }
      assert(found);
      Value *val = PN->getIncomingValueForBlock(h->data);
      DEBUG(errs()<<"using "<<*val<<" for PHI\n");
      bitvec *val_bv = bfll_env_get(M, val);
      bfll_env_put(M, I, val_bv);
      break;
    }

    case Instruction::Ret: {
      ReturnInst *RI = dyn_cast<ReturnInst>(I);
      Value *ret = RI->getReturnValue();
      bitvec *ret_bv = bfll_env_get(M, ret);
      uint64_t target_cnt = 2;
      fprintf(stderr, "solving with target count %" PRIu64 "...\n", target_cnt);
      if (BF_UNSAT == bfPushAssumption(BF, bfvEqual(BF, ret_bv, bfvUconstant(BF, ret_bv->size, target_cnt)))) {
        fprintf(stderr, "unsat (trivial)\n");
        M->terminated = true;
        break;
      }
      bfresult result = bfSolve(BF);
      if (result == BF_SAT) {
        print_function_inputs(M, popcnt_x86);
      } else fprintf(stderr, "unsat\n");
      M->terminated = true;
      break;
    }

    default:
      fprintf(stderr, "instruction not implemented!\n");
      errs() << *M->PC << "\n";
      M->terminated = true;
      break;
    }


    ++M->PC;
    vectorPush(states, M);
  } while (states->size > 0);


  bfll_machine_destroy(M);
  bfDestroy(BF);
}


int main(int argc, char **argv)
{

  if (argc < 2) {
    fprintf(stderr, "need bitcode file\n");
    exit(1);
  }

  llvm::DebugFlag = true;

  symex_loop(argv[1]);
   exit (0);
}

