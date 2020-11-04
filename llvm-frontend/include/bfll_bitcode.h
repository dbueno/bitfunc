#ifndef bfll_bitcode_h_included
#define bfll_bitcode_h_included

#include "llvm/Module.h"

extern "C" {
#include <bitfunc.h>
}

llvm::Module *ll_load_bitcode(bfman *BF, const char *bitcode_path);

#endif
