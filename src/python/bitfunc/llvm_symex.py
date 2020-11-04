# Copyright 2012 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
# on behalf of the U.S. Government. Export of this program may require a license
# from the United States Government.

from llvm import *
from llvm.core import *
from llvm.ee import *
import expdag as exp
import bitfunc
from sys import stderr
import logging

logger = logging.getLogger(__name__)



# Notes on memory
##############################################################################

# Any memory access must be done through a pointer value associated with an
# address range of the memory access, otherwise the behavior is undefined. Pointer
# values are associated with address ranges according to the following rules:

# - A pointer value is associated with the addresses associated with any value it
#   is based on.

# - An address of a global variable is associated with the address range of the
#   variable's storage.

# - The result value of an allocation instruction is associated with the address
#   range of the allocated storage.

# - A null pointer in the default address-space is associated with no address.

# - An integer constant other than zero or a pointer value returned from a
#   function not defined within LLVM may be associated with address ranges
#   allocated through mechanisms other than those provided by LLVM. Such ranges
#   shall not overlap with any ranges of addresses allocated by mechanisms
#   provided by LLVM.

# A pointer value is based on another pointer value according to the following
#   rules:

# - A pointer value formed from a getelementptr operation is based on the first
#   operand of the getelementptr.

# - The result value of a bitcast is based on the operand of the bitcast.

# - A pointer value formed by an inttoptr is based on all pointer values that
#   contribute (directly or indirectly) to the computation of the pointer's value.

# The "based on" relationship is transitive.

# Note that this definition of "based" is intentionally similar to the definition
# of "based" in C99, though it is slightly weaker.

# LLVM IR does not associate types with memory. The result type of a load merely
# indicates the size and alignment of the memory from which to load, as well as
# the interpretation of the value. The first operand type of a store similarly
# only indicates the size and alignment of the store.


MEM_SZ = 8                      # a byte


class StateQueue(list):
  pass

class StackFrame(object):
  """
  stack -- Stack this is a frame for
  base -- beginning address of the frame (int)
  len -- length of the frame in bytes ?? (int)
  data -- data written (ArrayUpdate)
  prev -- previous frame (StackFrame)
  slots -- list of stack slot allocas, in order ([StackFrame.StackSlot])
  """

  class StackSlot(object):
    _good_keys = set(['name', 'addr', 'sz'])
    def __init__(self, **kwargs):
      """
      acceptable keys: name, addr, sz
      """
      if set(kwargs.keys()) - self._good_keys:
        raise KeyError('unsupported keyword arguments: %s' %\
                         str(kwargs.keys()))
      self.__dict__.update(kwargs)

    def __str__(self):
      return 'StackSlot(%s)' % str(self.__dict__)

  def __init__(self, stack, base,
               prev=None,
               update_group_class=exp.VanillaUpdateGroup):
    self.stack = stack
    self.base = base
    self.slots = []
    self.len = 0
    self.data = update_group_class(exp.Array(stack.MS.idxsz, MEM_SZ))
    self.prev = prev

  def from_parent(self, SF, **kwargs):
    """
    creates a stack frame that is contained within its parent, SF.
    """
    return StackFrame(SF.MS, SF.base + SF.len, prev=SF, **kwargs)
  
  def alloca(self, sz, name=None):
    """
    allocates sz bytes on the stack and returns the address of the new location
    """
    addr = self.base - self.len
    self.len -= sz
    slot = StackFrame.StackSlot(name=name, addr=addr, sz=sz)
    print 'slot added: %s' % str(slot)
    self.slots.append(slot)
    return exp.UConstExp(addr, width=self.stack.MS.idxsz)

class Stack(object):
  """
  MS -- MachineState this Stack is for
  base -- base address (int)
  inmost_frame -- innermost stack from StackFrame
  """
  def __init__(self, MS, base):
    self.MS = MS
    self.base = base
    self.inmost_frame = None

  def alloca(self, *args, **kwargs):
    return self.inmost_frame.alloca(*args, **kwargs)

  def alloc_frame(self, **kwargs):
    """
    allocates a new stack frame
    """
    if self.inmost_frame is None:
      newbase = self.base
    else:
      newbase = self.inmost_frame.base - self.inmost_frame.len
    self.inmost_frame = StackFrame(self, newbase, prev=self.inmost_frame,
                                   **kwargs)
    return self.inmost_frame

  def is_address_within(self, addr):
    return addr <= self.base

class HeapBlock(object):
  """
  base -- base address (int)
  len -- length (int)
  data -- data written (ArrayUpdate)
  """
  pass

class Heap(object):
  """
  base -- base address (int)
  len -- length (int)

  
  Methods:
  find_block -- locate a heap block for a given address
  store(val, ptr) -- *ptr = val (val and ptr are exps)
  """
  
  def __init__(self, MS, base=None, length=None):
    self.base = base
    self.len = length
    self.MS = MS
    # self.arr = exp.Array(M.idxsz, MEM_SZ, base=base, length=length)
    # self.updates = exp.VanillaUpdateGroup(a)

  def is_address_within(self, addr):
    return self.base <= addr < self.base+self.len

  def malloc(self):
    pass

class Global(object):
  """
  addr -- address (bitvec)
  len -- length (bitvec)
  val -- value of the global (ArrayUpdate)
  """
  pass


class MachineState(object):
  """
  BF -- bfman
  m -- llvm Module
  idxsz -- size of pointers (int)
  PC -- program counter (instruction iterator)
  heap -- Heap
  stack -- StackFrame

  env -- maps LLVM Values to exps (dict)
  path_condition -- list of 1-bit expressions (list(exp.ExpDagBase))

  Keyword args:
  SP_initial -- address for the initial stack pointer (it grows with -)
  heap_initial -- address for the heap start (it grows with +)
  """

  def __init__(self, BF, m,
               SP_initial=0x7ffffff,
               heap_initial=0x8000000):
    """
    BF -- bfman
    m -- llvm Module
    """
    self.BF = BF
    self.m = m
    self.PC = 0
    self.heap = Heap(self, base=heap_initial, length=0xfffffff-heap_initial)
    self.stack = Stack(self, SP_initial)
    self.env = {}
    self.path_condition = []
    self.ee = ExecutionEngine.new(self.m)
    self.parent = None

    self.stack.alloc_frame()

  @classmethod
  def init_empty(cls, BF, m, **kwargs):
    """
    The keyword arguments are passed directly to __init__; so look there
    """
    x = cls(BF, m, **kwargs)
    x.parent = None
    return x

  idxsz = property(lambda self: self.m.pointer_size)

  def start_at_function(self, name):
    """
    sets up PC to begin at the first basic block of the named function
    """
    f = self.m.get_function_named(name)
    self.PC = next(iter(f.basic_blocks)).instructions

  @property
  def TD(self):
    """
    The TargetData instance for the code associated with this state
    """
    return self.ee.target_data

  def getenv(self, value):
    """
    Retrieve expression for the llvm value from the environment
    """
    v = self.env.get(value)
    if v is None and self.parent is not None:
      v = self.parent.getenv(value)
    return v

  def store(self, val, ptr):
    if isinstance(ptr, exp.UConstExp):
      print 'ptr is constant: %s' % str(ptr)
      if self.heap.is_address_within(ptr.uconst):
        # store to heap
        self.heap.store(val, ptr)
      elif self.stack.is_address_within(ptr.uconst):
        # store to stack
        self.stack.store(val, ptr)
      else:
        assert False and 'address could not be located'

# we assume BF is set as a global of the module
def symex(path):
  with open(path) as fd:
    m = Module.from_bitcode(fd)

  state_init = MachineState.init_empty(BF, m)
  state_init.start_at_function('popcnt_x86')
  print 'pointer size is %d' % state_init.idxsz

  queue = StateQueue()
  queue.append(state_init)

  while len(queue) > 0:
    state = queue.pop()
    bb = list(state.PC)

    print 'len(bb) = %d' % len(bb)
    # print '%s' % ', '.join(map(lambda x: str(type(x)), bb))
    for insn in bb[:-1]:
      print str(insn)
      exec_nonterminator_insn(insn, state)
    print str(bb[-1])
    exec_terminator_insn(bb[-1], state)

    queue.append(state)



def exec_nonterminator_insn(insn, state):
  assert not insn.is_terminator

  opcode = insn.opcode
  
  if opcode == OPCODE_ALLOCA:
    bytes = state.TD.abi_size(insn.type)
    # print 'alloca.abi_size %d' % bytes
    addr = state.stack.alloca(bytes, name=str(insn))
    state.env[insn] = addr

  elif opcode == OPCODE_STORE:
    val = state.getenv(insn.operands[0])
    ptr = state.getenv(insn.operands[1])
    state.store(val, ptr)

  else:
    print >>stderr, 'unhandled instruction: %s' % str(insn)
    exit(1)


def exec_terminator_insn(insn, state):
  assert insn.is_terminator


