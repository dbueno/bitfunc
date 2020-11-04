#ifndef klee_mem_h_included
#define klee_mem_h_included

#include <stdint.h>

#include "types.h"
#include "array.h"
#include "bitvec.h"
#include "mem.h"

struct mem_blk
{
  uintptr_t base;               /* base addr */
  size_t    len;
  memory   *mem;
};

typedef struct klee_mem
{
  vector blocks;
} klee_mem;

klee_mem *klee_mem_init(bfman *b);


#endif
