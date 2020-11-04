#ifndef llvm_machine_h_included
#define llvm_machine_h_included

#include "llvm/Support/InstIterator.h"
#include "linklist_bbs.h"

#define BITS_IN_BYTE 8

extern "C" {
#  include <funcsat/vec.h>
#  include <funcsat/hashtable.h>
#  include <bitfunc.h>
#  include <inttypes.h>
#  include <funcsat/intvec.h>
#  include "linklist_literal.h"
}
/* 
   alloca is used to add to the stack and such

Any memory access must be done through a pointer value associated with an
address range of the memory access, otherwise the behavior is undefined. Pointer
values are associated with address ranges according to the following rules:

- A pointer value is associated with the addresses associated with any value it
  is based on.

- An address of a global variable is associated with the address range of the
  variable's storage.

- The result value of an allocation instruction is associated with the address
  range of the allocated storage.

- A null pointer in the default address-space is associated with no address.

- An integer constant other than zero or a pointer value returned from a
  function not defined within LLVM may be associated with address ranges
  allocated through mechanisms other than those provided by LLVM. Such ranges
  shall not overlap with any ranges of addresses allocated by mechanisms
  provided by LLVM.

A pointer value is based on another pointer value according to the following
  rules:

- A pointer value formed from a getelementptr operation is based on the first
  operand of the getelementptr.

- The result value of a bitcast is based on the operand of the bitcast.

- A pointer value formed by an inttoptr is based on all pointer values that
  contribute (directly or indirectly) to the computation of the pointer's value.

The "based on" relationship is transitive.

Note that this definition of "based" is intentionally similar to the definition
of "based" in C99, though it is slightly weaker.

LLVM IR does not associate types with memory. The result type of a load merely
indicates the size and alignment of the memory from which to load, as well as
the interpretation of the value. The first operand type of a store similarly
only indicates the size and alignment of the store.

 */

struct bfll_stack_frame
{
  unsigned base;
  unsigned SP;
  unsigned len;
  memory *data;
};

struct bfll_heap_block
{
  uintptr_t base;
  uintptr_t len;
  memory *data;
  struct bfll_heap_block *nx;
};

struct bfll_heap
{
  uintptr_t base;
  uintptr_t len;
  struct bfll_heap_block *blocks;
};

/* every memory op will be in the bounds */
struct bfll_global
{
  bitvec *addr;
  bitvec *len;
  memory *value;
};


/* symbolic machine state */
typedef struct bfll_machine
{
  bfman *BF;

  struct bfll_machine *parent;

  uint32_t idxsize;
  uint32_t eltsize;

  llvm::BasicBlock::iterator PC; /* program counter */
  struct lkl_bbs *BBs;           /* BBs along this path, in order */

  struct bfll_heap *heap;
  struct bfll_stack_frame *stack;

  struct hashtable *env;        /* map of llvm::Value* to bitvec* */
  struct lkl_literal *path_condition;

  bool terminated;
} bfll_machine;

extern "C" {
bfll_machine *bfll_machine_init(bfman *BF, llvm::Module *module);
void bfll_machine_destroy(bfll_machine *);

bitvec *bfll_alloca(bfll_machine *, uint64_t num_bytes);
bitvec *bfll_malloc(bfll_machine *, uint64_t num_bytes);

/* associates the given bit vector with the given LLVM value */
void bfll_env_put(bfll_machine *, llvm::Value *val, bitvec *bv);
bitvec *bfll_env_get(bfll_machine *, llvm::Value *);

void bfll_store(bfll_machine *, bitvec *addr, bitvec *val);
bitvec *bfll_load(bfll_machine *M, bitvec *addr, uint64_t num_bytes);

/* returns a new child state for the given state */
bfll_machine *bfll_mkchild(bfll_machine *N);


DECLARE_HASHTABLE(bfll_env_insert, bfll_env_search, bfll_env_remove, llvm::Value*, bitvec)
}

#endif
