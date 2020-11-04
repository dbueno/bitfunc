from llvm.core import *
import bitfunc.llvm_symex as bfll
import bitfunc


# with open('popcnt_defs.bc') as fd:
#   m = Module.from_bitcode(fd)

# print m


# print popcnt_x86
# for f in m.functions:
#   print f

bfll.BF = bitfunc.bfman()


bfll.symex('popcnt_defs.bc')

# for bb in popcnt_x86.basic_blocks:
#   print '******************** basic block'
#   for insn in bb.instructions:
#     print insn

