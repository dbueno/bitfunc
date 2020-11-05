# -*- mode: python -*-
#cython: embedsignature=True, language_level=2
# Copyright 2012 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
# on behalf of the U.S. Government. Export of this program may require a license
# from the United States Government.

cimport _bitfunc_cy as bf
from libc.stdint cimport *
from libc.stdio cimport stdout
import functools
import warnings

 # @memoized
 # def fibonacci(n):
 #    "Return the nth fibonacci number."
 #    if n in (0, 1):
 #       return n
 #    return fibonacci(n-1) + fibonacci(n-2)
 
 # print fibonacci(12)

cdef class literal:

  cdef bf.literal l
  cdef readonly bfman _bfman

  def __cinit__(self, bf.literal l, bfman b):
    self.l = l
    self._bfman = b

  def __reduce__(self):
    return (literal, (self.l,))

  def __iter__(self):
      yield self

  cpdef bint isTrue(self):
    """Whether this is the unique TRUE literal"""
    return self.l == bf.LITERAL_TRUE

  cpdef bint isFalse(self):
    """Whether this is the unique FALSE literal"""
    return self.l == bf.LITERAL_FALSE

  def __invert__(self):
    return self.__neg__()

  def asInt(self): return self.as_int()
  def as_int(self):
    if self.isTrue():
      return 1
    elif self.isFalse():
      return 0
    else:
      raise ValueError('symbolic literal cannot be coerced to int')
  int = property(as_int)

  def asBool(self): return self.as_bool()
  def as_bool(self):
    if self.isTrue():
      return True
    elif self.isFalse():
      return False
    else:
      raise ValueError('symbolic literal cannot be coerced to bool')
  bool = property(as_bool)

  def asBitvec(self): return self.as_bitvec()
  def as_bitvec(self):
    return bitvec.fromLiteral(self)
  bitvec = property(as_bitvec)
  
  cpdef dimacs(self):
      if not self.isSymbolic():
          raise ValueError, 'concrete literals have no dimacs representation'
      else:
          return self.l

  def __neg__(self):
    return literal(-self.l, self._bfman)

  def __abs__(self):
    if self.l < 0:
      return literal(-self.l, self._bfman)
    else:
      return self

  def _apply_binop(self, op, other):
    # Treat bools and 0 or 1 as literal
    if not isinstance(other, literal):
      if isinstance(other, bool) or \
          (isinstance(other, int) and (other == 0 or other == 1)):
        other = literal.fromBool(self._bfman, bool(other))
    if isinstance(other, literal):
      return op(self, other)
    return NotImplemented

  def __and__(self, other):
    return self._apply_binop(self._bfman.newAnd, other)

  def __or__(self, other):
    return self._apply_binop(self._bfman.newOr, other)

  def __xor__(self, other):
    return self._apply_binop(self._bfman.newXor, other)

  def eq(self, literal other):
    """Test that this is the same literal as the other"""
    return self.l == other.l

  def hash(self):
    return int(self.l)

  def __eq__(self, other):
    return self._apply_binop(self._bfman.newEqual, other)

  def __ne__(self, other):
    x = self._apply_binop(self._bfman.newEqual, other)
    if x == NotImplemented:
      return x
    return -x

  def __hash__(self):
    return self.l

  def __str__(self):
    s = str(self.l)
    if self.isTrue():
      s = 'TRUE'
    elif self.isFalse():
      s = 'FALSE'
    return 'literal(' + s + ')'

  def __repr__(self):
    return 'literal(' + str(self.l) + ')'

  @classmethod
  def fromBool(cls, bfman man, b):
    """create a literal representing the truth value of the given Boolean"""
    if b:
      return man.LIT_TRUE
    else:
      return man.LIT_FALSE

  def asBit(self):
    """returns a single character. 1 if the literal is true, 0 if it's false,
    and ? otherwise."""
    if self.isTrue():
      return '1'
    elif self.isFalse():
      return '0'
    else:
      return '?'

  def isSymbolic(self):
    """A literal is symbolic if it is not concrete"""
    return not self.isConcrete()

  def isConcrete(self):
    """self.isTrue() or self.isFalse()"""
    return self.isTrue() or self.isFalse()


STATUS_MUST_BE_TRUE = bf.STATUS_MUST_BE_TRUE
STATUS_MUST_BE_FALSE = bf.STATUS_MUST_BE_FALSE
STATUS_TRUE_OR_FALSE = bf.STATUS_TRUE_OR_FALSE
STATUS_NOT_TRUE_NOR_FALSE = bf.STATUS_NOT_TRUE_NOR_FALSE
STATUS_UNKNOWN = bf.STATUS_UNKNOWN

mbool_false = 0
mbool_true = 1
mbool_unknown = bf.unknown

AIG_MODE = bf.AIG_MODE

cdef class bfresult:
  cdef s

  def __init__(self, s):
    self.s = s

  def __repr__(self):
    return self.s

BF_SAT = bfresult('BF_SAT')
BF_UNSAT = bfresult('BF_UNSAT')
BF_UNKNOWN = bfresult('BF_UNKNOWN')
def wrapbfresult(bf.bfresult x):
  if x == bf.BF_SAT: return BF_SAT
  elif x == bf.BF_UNSAT: return BF_UNSAT
  elif x == bf.BF_UNKNOWN: return BF_UNKNOWN

cdef class bitvec:
  cdef bf.bitvec *v
  cdef readonly bfman _bfman

  def __dealloc__(self):
    bf.CONSUME_BITVEC(bf.bfvRelease(self.v))

  def __reduce__(self):
    return (vAlloc, (self._bfman, self.v.size), list(self))

  def __setstate__(self, lits):
    self.v.size = len(lits)
    for x in range(len(lits)):
      self[x] = lits[x]

  cpdef push(self, literal data):
    """Appends the literal at the MSB position of this bitvec"""
    bf.bfvPush(self.v, data.l)

  cpdef push_back(self, literal data):
    """Appends the literal at the MSB position of this bitvec"""
    bf.bfvPush(self.v, data.l)

  cpdef vPush(self, literal data):
    bf.bfvPush(self.v, data.l)

  def __getitem__(self, size_t index):
    if not index < self.v.size:
      raise IndexError, 'bitvec index out of range'
    return literal(self.v.data[index], self._bfman)

  def __setitem__(self, size_t index, literal value):
    if not index < self.v.size:
      raise IndexError, 'bitvec index out of range'
    self.v.data[index] = value.l

  def __getslice__(self, size_t i, size_t j):
      return self._bfman.vExtract(self, i, j-i)

  def __len__(self):
    return self.v.size

  def eq(self, bitvec other):
    """Test whether this bitvec is structurally identical to other"""
    if len(self) == len(other):
      for i in xrange(len(self)):
        if not self[i].eq(other[i]):
          return False
      return True
    else:
      return False

  def hash(self):
    return hash(tuple(list(self)))

  def __iter__(self):
    for i in xrange(self.v.size):
      yield self[i]

  def __richcmp__(self, other, int op):
    if isinstance(other, (int, long)):
      other = self._bfman.vConstant(len(self), other)
    if isinstance(other, bitvec):
      if op == 2: # eq
        if len(self) == len(other):
          ret = self._bfman.LIT_TRUE
          for i in xrange(len(self)):
            ret = ret & (self[i] == other[i])
          return ret
        else:
          return self._bfman.LIT_FALSE
      
      elif op == 3: # ne
        if len(self) != len(other):
          return self._bfman.LIT_TRUE
        else:
          ret = self._bfman.LIT_FALSE
          for i in xrange(len(self)):
            ret = ret | (self[i] != other[i])
          return ret

      elif op == 0 and len(self) == len(other): # <
        return self._bfman.vSlt(self, other)

      elif op == 4 and len(self) == len(other): # >
        return self._bfman.vSgt(self, other)

      elif op == 1 and len(self) == len(other): # <=
        return self._bfman.vSlte(self, other)

      elif op == 5 and len(self) == len(other): # >=
        return self._bfman.vSgte(self, other)
      
      raise NotImplementedError

  def _binop_apply(self, other, function):
    if isinstance(other, bitvec):
      return function(self, other)
    elif isinstance(other, (int, long)):
      return function(self, self._bfman.vUconstant(len(self), other))
    else:
      raise ValueError, "other neither bitvec nor int"

  def __add__(self, other):
    return self._binop_apply(other, self._bfman.vAdd0)

  def __sub__(self, other):
    return self._binop_apply(other, self._bfman.vSubtract)

  def __mul__(self, other):
    return self._binop_apply(other, self._bfman.vMul)

  def __floordiv__(self, other):
    return self._binop_apply(other, self._bfman.vSdiv)

  def __mod__(self, other):
    return self._binop_apply(other, self._bfman.vSrem)

  def __lshift__(self, other):
    return self._binop_apply(other, self._bfman.vLshift)

  def __rshift__(self, other):
    """Arithmetic right shift (see also SHR)"""
    if isinstance(other, bitvec):
        return self._bfman.vRshift(self, other, self[-1])
    elif isinstance(other, (int, long)):
        return self._bfman.vRshift(self, self._bfman.vConstant(len(self), other), self[-1])
    else:
        raise ValueError, "other neither bitvec nor int"


  def SHR(self, uint32_t dist):
    """Logical right shift (see also __rshift__)"""
    return self._bfman.vRLshift(self, dist)

  def __and__(self, other):
    if isinstance(self, (int, long)): #reverse?!
      self, other = other, self
    return self._binop_apply(other, self._bfman.vAnd)

  def __or__(self, other):
    if isinstance(self, (int, long)): #reverse?!
      self, other = other, self
    return self._binop_apply(other, self._bfman.vOr)

  def __xor__(self, other):
    if isinstance(self, (int, long)): #reverse?!
      self, other = other, self
    return self._binop_apply(other, self._bfman.vXor)

  def __neg__(self):
    return self._bfman.vNegate(self)

  def __abs__(self):
    """Sets the MSB to 0"""
    b = self._bfman.vDup(self)
    b[len(b)-1] = self._bfman.LIT_FALSE
    return b

  def __hex__(self):
    if self.isConcrete():
      return str(self)
    else:
      raise ValueError, 'bit vector is symbolic, cannot convert to hex'

  def __invert__(self):
    return self._bfman.vInvert(self)

  def isConcrete(self):
    return not self.isSymbolic()

  def isSymbolic(self):
    """Test if any literal in the bit vector is symbolic"""
    for i in xrange(0, len(self)):
      if not self[i].isConcrete():
        return True
    return False

  cpdef elseIfFalse(self, c, ifFalse):
    """Whenever c is true, evaluates to this bitvec. Otherwise, evaluates to
    ifFalse"""
    if isinstance(ifFalse, (int, long)):
        ifTrue = self._bfman.vConstant(len(self), ifFalse)
    return self._bfman.vSelect(c, self, ifFalse)

  cpdef elseIfTrue(self, c, ifTrue):
    """Whenever c is false, evaluates to this bitvec. Otherwise, evaluates to
    ifTrue"""
    if isinstance(ifTrue, (int, long)):
        ifTrue = self._bfman.vConstant(len(self), ifTrue)
    return self._bfman.vSelect(c, ifTrue, self)

  @classmethod
  def zero(cls, bfman b, uint32_t sz):
    return wrapbv(b, bf.bfvZero(sz))

  @classmethod
  def ones(cls, bfman b, uint32_t sz):
    return wrapbv(b, bf.bfvOnes(sz))

  @classmethod
  def one(cls, bfman b, uint32_t sz):
    return wrapbv(b, bf.bfvOne(sz))

  def asInt(self): return self.as_int()
  def as_int(self):
    # uses the definition of 2s complement
    if not self.isConcrete():
      raise ValueError('symbolic bitvecs cannot be coerced to int')
    m = len(self)
    ret = -self[m-1].int*(2**(m-1))
    for i in xrange(m-1):
      ret += self[i].int*(2**i)
    return ret
  int = property(as_int)

  def asUint(self): return self.asUint()
  def as_uint(self):
    if not self.isConcrete():
      raise ValueError('symbolic bitvecs cannot be coerced to uint')
    ret = 0
    for i in xrange(len(self)):
      ret = ret | (self[i].int << i)
    return ret
  uint = property(as_uint)

  def asBin(self): return self.as_bin()
  def as_bin(self):
    return bin(self.uint)
  bin = property(as_bin)

  def asBitvec(self): return self.as_bitvec()
  def as_bitvec(self):
    return self
  bitvec = property(as_bitvec)

  def __str__(self):
    """
    Printing for symbolic and concrete bit vectors.
    """
    if self.isSymbolic():
      return self.bitstr()
    else:
      return hex(self.uint)

  def __repr__(self):
    if self.isSymbolic():
      ret = '['
      lits = []
      for i in xrange(0, len(self)):
        lits.append(self[i])
      ret += ', '.join(map(str, lits))
      ret += ']'
      return ret
    else:
      return hex(self.uint)

  def bitstr(self):
      return ''.join(reversed([self[i].asBit() for i in xrange(len(self))]))

  @classmethod
  def fromList(cls, bfman b, l):
    """Input: a list of literals or Boolean-interpretable objects.
    Returns bv where bv[i] = l[i]"""
    ret = vAlloc(b, len(l))
    for x in l:
      if not isinstance(x, literal): 
        x = literal.fromBool(b, x)
      ret.vPush(x)
    return ret

  @classmethod
  def fromLiteral(cls, bfman b, l):
    b = vAlloc(b, 1)
    b.vPush(l)
    return b

  @classmethod
  def alloc(cls, bfman b, n):
    return vAlloc(b, n)

cpdef bitvec vAlloc(bfman b, uint32_t initialCapacity):
  return wrapbv(b, bf.bfvAlloc(initialCapacity))

cdef inline makestr(char *s):
  if s is NULL:
    return None
  else:
    return s

cdef inline char *fromstr(s):
  if s is None:
    return NULL
  else:
    return s

cdef class bfman:
  """The bitfunc manager.

  This is the first class you'll need to use to do anything useful in
  bitfunc. It manages the construction and description of constraints for a
  particular instance. It also lets you solve() that instance.

  Once the instance is solved, the *FromCounterExample family of methods allows
  you to extract the satisfying assignment in terms of bitfunc objects."""
  cdef bf.bfman *b

  cdef readonly literal LIT_TRUE
  cdef readonly literal LIT_FALSE

  def __cinit__(self):
    self.b = bf.bfInit(bf.AIG_MODE)
    self.LIT_TRUE = literal(bf.LITERAL_TRUE, self)
    self.LIT_FALSE = literal(bf.LITERAL_FALSE, self)

  def __reduce__(self):
    state = {}
    state['numVars'] = self.b.numVars
    state['assumps'] = wrapbv(self, bf.bfvHold(self.b.assumps))
    ins = [(s.lit,makestr(s.name)) for s in self.b.aig.inputs[:self.b.aig.num_inputs]]
    outs = [(s.lit,makestr(s.name)) for s in self.b.aig.outputs[:self.b.aig.num_outputs]]
    latches = [(s.lit,s.next,makestr(s.name)) for s in self.b.aig.latches[:self.b.aig.num_latches]]
    ands = [(a.lhs,a.rhs0,a.rhs1) for a in self.b.aig.ands[:self.b.aig.num_ands]]
    state['aig'] = (ins, outs, latches, ands)
    return (bfman, (), state)

  def __setstate__(self, state):
    bf.bfSetNumVars(self.b, state['numVars'])
    ins, outs, latches, ands = state['aig']
    for lit,name in ins:
      bf.aiger_add_input(self.b.aig, lit, fromstr(name))
    for lit,next,name in latches:
      bf.aiger_add_latch(self.b.aig, lit, next, fromstr(name))
    for lit,name in outs:
      bf.aiger_add_output(self.b.aig, lit, fromstr(name))
    for lhs,rhs0,rhs1 in ands:
      bf.aiger_add_and(self.b.aig, lhs, rhs0, rhs1)
    bf.bfvPushAssumption(self.b, (<bitvec>state['assumps']).v)

  def __dealloc__(self):
    bf.bfDestroy(self.b)

  cpdef enableDebug(self, char *category, int level):
    bf.bfEnableDebug(self.b, category, level)

  cpdef printCNF(self, char *filename):
    """Prints the current CNF to the given filename"""
    bf.bfPrintCNF(self.b, filename)

  cpdef printAIG(self):
    """Prints the AIG to stdout"""
    bf.bfPrintAIG(self.b, bf.stdout)

  cpdef bfresult pushAssumption(self, literal p):
    """Add a literal as an assumption, in LIFO order."""
    return wrapbfresult(bf.bfPushAssumption(self.b, p.l))

  cpdef popAssumptions(self, unsigned num):
    """popAssumptions(num). Pops num assumptions in LIFO order."""
    bf.bfPopAssumptions(self.b, num)

  cpdef configureFuncsat(self): bf.bfConfigureFuncsat(self.b)
  cpdef configurePicosat(self): bf.bfConfigurePicosat(self.b)
  cpdef configurePicosatIncremental(self): bf.bfConfigurePicosatIncremental(self.b)
  cpdef configureLingeling(self): bf.bfConfigureLingeling(self.b)
  cpdef configurePlingeling(self): bf.bfConfigurePlingeling(self.b)
  cpdef configurePrecosat(self): bf.bfConfigurePrecosat(self.b)
  cpdef configurePicosatReduce(self): bf.bfConfigurePicosatReduce(self.b)
  cpdef configureLingelingReduce(self): bf.bfConfigureLingelingReduce(self.b)
  cpdef configureBFSolve(self):
      """Will run the command 'bfsolve' in the path"""
      bf.bfConfigureExternal(self.b)

  cpdef bfresult solve(self):
    """solve(). Assuming the assumptions are true, find a satisfying assignment
    for every variable"""
    return wrapbfresult(bf.bfSolve(self.b))

  cpdef reset(self):
    """you shouldn't need to call this, just make a new bfman"""
    bf.bfReset(self.b)

  cpdef literal lFromCounterExample(self, literal l):
    """lFromCounterExample(l). Get the assignment for literal l"""
    return literal(bf.bflFromCounterExample(self.b, l.l), self)

  cpdef bitvec vFromCounterExample(self, bitvec b):
    """vFromCounterExample(b). Get the assignment for bitvec b"""
    return wrapbv(self, bf.bfvFromCounterExample(self.b, b.v))

  cpdef memory mFromCounterExample(self, memory m):
    """mFromCounterExample(m). Get the assignment for memory m"""
    return wrapmem(self, bf.bfmFromCounterExample(self.b, m.m))

  cpdef fromCounterExample(self, x):
    """Given a literal, bitvec, or memory, return the counterexample value from
    the model"""
    if isinstance(x, literal):
      return self.lFromCounterExample(x)
    elif isinstance(x, bitvec):
      return self.vFromCounterExample(x)
    elif isinstance(x, memory):
      return self.mFromCounterExample(x)
    raise ValueError, 'value not a literal, bitvec, or memory'

  cpdef literal newVar(self):
    return literal(bf.bfNewVar(self.b), self)

  cpdef literal newAnd(self, literal a, literal b):
    """Returns a literal that is the logical and of a and b"""
    return literal(bf.bfNewAnd(self.b, a.l, b.l), self)

  cpdef literal newOr(self, literal a, literal b):
    """Returns a literal that is the logical or of a and b"""
    return literal(bf.bfNewOr(self.b, a.l, b.l), self)

  cpdef literal newXor(self, literal a, literal b):
    """Returns a literal that is the logical xor of a and b"""
    return literal(bf.bfNewXor(self.b, a.l, b.l), self)

  cpdef literal newNand(self, literal a, literal b):
    """Returns a literal that is the logical nand of a and b"""
    return literal(bf.bfNewNand(self.b, a.l, b.l), self)

  cpdef literal newNor(self, literal a, literal b):
    """Returns a literal that is the logical nor of a and b"""
    return literal(bf.bfNewNor(self.b, a.l, b.l), self)

  cpdef literal newEqual(self, literal a, literal b):
    """Returns a literal that is the logical equal of a and b"""
    return literal(bf.bfNewEqual(self.b, a.l, b.l), self)

  cpdef literal newImplies(self, literal a, literal b):
    """Returns a literal that is the logical implies of a and b"""
    return literal(bf.bfNewImplies(self.b, a.l, b.l), self)

  cpdef literal newIff(self, literal a, literal b):
    """Returns a literal that is the logical iff of a and b"""
    return literal(bf.bfNewIff(self.b, a.l, b.l), self)

  cpdef lIsTrue(self, literal l):
    """deprecated"""
    warnings.warn('use l.isTrue()', DeprecationWarning)
    return l.isTrue()

  cpdef lIsFalse(self, literal l):
    """deprecated"""
    warnings.warn('use l.isFalse()', DeprecationWarning)
    return l.isFalse()

  cpdef lAnd(self, literal a, literal b, literal o):
    bf.bfAnd(self.b, a.l, b.l, o.l)

  cpdef lOr(self, literal a, literal b, literal o):
    bf.bfOr(self.b, a.l, b.l, o.l)

  cpdef lXor(self, literal a, literal b, literal o):
    bf.bfXor(self.b, a.l, b.l, o.l)

  cpdef lNand(self, literal a, literal b, literal o):
    bf.bfNand(self.b, a.l, b.l, o.l)

  cpdef lNor(self, literal a, literal b, literal o):
    bf.bfNor(self.b, a.l, b.l, o.l)

  cpdef lImplies(self, literal a, literal b, literal o):
    bf.bfImplies(self.b, a.l, b.l, o.l)

  cpdef lEqual(self, literal a, literal b, literal o):
    bf.bfEqual(self.b, a.l, b.l, o.l)

  cpdef literal bigAnd(self, bitvec bv):
    """Returns a literal equal to the logical and of all the literals in bv"""
    return literal(bf.bfBigAnd(self.b, bv.v), self)

  cpdef literal bigOr(self, bitvec bv):
    """Returns a literal equal to the logical or of all the literals in bv"""
    return literal(bf.bfBigOr(self.b, bv.v), self)

  cpdef literal bigXor(self, bitvec bv):
    """Returns a literal equal to the logical or of all the literals in bv"""
    return literal(bf.bfBigXor(self.b, bv.v), self)

  cpdef literal newSelect(self, literal a, literal b, literal c):
    """Same as lSelect"""
    return literal(bf.bfNewSelect(self.b, a.l, b.l, c.l), self)

  cpdef literal lSelect(self, literal c, literal ifTrue, literal ifFalse):
    """Returns a literal that is ifTrue if c is true, and ifFalse otherwise"""
    return self.newSelect(c, ifTrue, ifFalse)

  cpdef setEqual(self, literal a, literal b):
    """Constraint a and be to equal each other in every assignment"""
    bf.bfSetEqual(self.b, a.l, b.l)

  cpdef bitvec vAnd(self, bitvec x, bitvec y):
    """Returns b = x&y"""
    return wrapbv(self, bf.bfvAnd(self.b, x.v, y.v))

  cpdef bitvec vOr(self, bitvec x, bitvec y):
    """Returns b = x|y"""
    return wrapbv(self, bf.bfvOr(self.b, x.v, y.v))

  cpdef bitvec vXor(self, bitvec x, bitvec y):
    """Returns b = x^y"""
    return wrapbv(self, bf.bfvXor(self.b, x.v, y.v))

  cpdef bitvec vNegate(self, bitvec x):
    """Returns b = -x"""
    return wrapbv(self, bf.bfvNegate(self.b, x.v))

  cpdef bitvec vInvert(self, bitvec x):
    """Returns b = ~x"""
    return wrapbv(self, bf.bfvInvert(self.b, x.v))

  cpdef bitvec vSelect(self, literal t, bitvec thn, bitvec els):
    """Returns a bitvec that is thn whenever t is true, els otherwise"""
    return wrapbv(self, bf.bfvSelect(self.b, t.l, thn.v, els.v))

  cpdef bitvec vZextend(self, uint32_t new_width, bitvec v):
    """Returns a zero-extended bitvec"""
    return wrapbv(self, bf.bfvZextend(self.b, new_width, v.v))

  cpdef bitvec vSextend(self, uint32_t new_width, bitvec v):
    """Returns a sign-extended bitvec"""
    return wrapbv(self, bf.bfvSextend(self.b, new_width, v.v))

  cpdef bitvec vTruncate(self, uint32_t new_width, bitvec v):
    """Returns a new bitvec truncated to new_width"""
    return wrapbv(self, bf.bfvTruncate(self.b, new_width, v.v))

  cpdef vAdd(self, bitvec x, bitvec y, literal cin):
    """Returns a bitvec x+y+c (cin is the carry in bit)"""
    cdef bf.literal cout
    res = wrapbv(self, bf.bfvAdd(self.b, x.v, y.v, cin.l, &cout))
    return (res, literal(cout, self))

  cpdef bitvec vAdd0(self, bitvec x, bitvec y):
    """Returns a bitvec x+y"""
    return wrapbv(self, bf.bfvAdd0(self.b, x.v, y.v))

  cpdef bitvec vSubtract(self, bitvec x, bitvec y):
    """Returns a bitvec x-y"""
    return wrapbv(self, bf.bfvSubtract(self.b, x.v, y.v))

  cpdef literal vUlt(self, x, y):
    """Returns a literal that is true exactly when x<y (unsigned comparison)"""
    if not isinstance(x, bitvec) and not isinstance(y, bitvec):
      raise ValueError, 'need to be passed at least one bit vector'
    if isinstance(x, (int, long)):
      x = self.vUconstant(len(y), x)
    if isinstance(y, (int, long)):
      y = self.vUconstant(len(x), y)
    return self.vUlt_bitvecs(x, y)
  
  cpdef literal vUlte(self, x, y):
    """Returns a literal that is true exactly when x<=y (unsigned comparison)"""
    if not isinstance(x, bitvec) and not isinstance(y, bitvec):
      raise ValueError, 'need to be passed at least one bit vector'
    if isinstance(x, (int, long)):
      x = self.vUconstant(len(y), x)
    if isinstance(y, (int, long)):
      y = self.vUconstant(len(x), y)
    return self.vUlte_bitvecs(x, y)
  
  cdef literal vUlt_bitvecs(self, bitvec x, bitvec y):
    return literal(bf.bfvUlt(self.b, x.v, y.v), self)

  cdef literal vUlte_bitvecs(self, bitvec x, bitvec y):
    return literal(bf.bfvUlte(self.b, x.v, y.v), self)

  cpdef vUgt(self, x, y):
    """Returns a literal that is true exactly when x>y (unsigned comparison)"""
    return self.vUlt(y, x)

  cpdef vUgte(self, x, y):
    """Returns a literal that is true exactly when x>=y (unsigned comparison)"""
    return -self.vUlt(x, y)

  cpdef literal vSlt(self, bitvec x, bitvec y):
    """Returns a literal that is true exactly when x<y (signed comparison)"""
    return literal(bf.bfvSlt(self.b, x.v, y.v), self)

  cpdef literal vSlte(self, bitvec x, bitvec y):
    """Returns a literal that is true exactly when x<=y (signed comparison)"""
    return literal(bf.bfvSlte(self.b, x.v, y.v), self)

  def vSgt(self, x, y):
    """Returns a literal that is true exactly when x>y (signed comparison)"""
    return self.vSlt(y, x)

  def vSgte(self, x, y):
    """Returns a literal that is true exactly when x>=y (signed comparison)"""
    return -self.vSlt(x, y)

  cpdef literal vSltZero(self, bitvec x):
    """Returns a literal that is true exactly when x<0 (signed comparison)"""
    return literal(bf.bfvSltZero(self.b, x.v), self)

  cpdef literal vSgtZero(self, bitvec x):
    """Returns a literal that is true exactly when x>0 (signed comparison)"""
    return literal(bf.bfvSgtZero(self.b, x.v), self)

  cpdef literal vSgteZero(self, bitvec x):
    """Returns a literal that is true exactly when x>=0 (signed comparison)"""
    return literal(bf.bfvSgteZero(self.b, x.v), self)

  cpdef literal vEqual(self, bitvec x, bitvec y):
    """Returns a literal that is true exactly when x==y"""
    return literal(bf.bfvEqual(self.b, x.v, y.v), self)

  cpdef bf.mbool vSame(self, bitvec x, bitvec y):
    return bf.bfvSame(x.v, y.v)

  cpdef bitvec vConstant(self, uint32_t width, object value):
    """vConstant(width, value)"""
    cdef unsigned int i
    cdef bitvec result = bitvec.alloc(self, width)
    for i in range(0, width):
      if value % 2:
        result.vPush(self.LIT_TRUE)
      else:
        result.vPush(self.LIT_FALSE)
      value /= 2
    return result

  cpdef bitvec vSconstant(self, uint32_t width, object value):
    warnings.warn('use vConstant(width, value)', DeprecationWarning)
    return self.vConstant(width, value)

  cpdef bitvec vUconstant(self, uint32_t width, object value):
    warnings.warn('use vConstant(width, value)', DeprecationWarning)
    return self.vConstant(width, value)

  cpdef bitvec vLshift(self, bitvec val, bitvec dist):
    return wrapbv(self, bf.bfvLshift(self.b, val.v, dist.v))

  cpdef bitvec vLshiftConst(self, bitvec val, uint32_t dist):
    return wrapbv(self, bf.bfvLshiftConst(self.b, val.v, dist))

  cpdef bitvec vRshift(self, bitvec val, bitvec dist, literal fill):
    return wrapbv(self, bf.bfvRshift(self.b, val.v, dist.v, fill.l))

  cpdef bitvec vRshiftConst(self, bitvec val, uint32_t dist, literal fill):
    return wrapbv(self, bf.bfvRshiftConst(self.b, val.v, dist, fill.l))

  cpdef bitvec vRLshift(self, bitvec val, uint32_t dist):
    """Logical right shift"""
    return self.vRshiftConst(val, dist, self.LIT_FALSE)

  cpdef bitvec vMul(self, bitvec x, bitvec y):
    return wrapbv(self, bf.bfvMul(self.b, x.v, y.v))

  cpdef vQuotRem(self, bitvec x, bitvec y):
    """vQuotRem(x, y). Returns a pair (quotient, remainder)."""
    cdef bf.bitvec *quot = NULL
    cdef bf.bitvec *rem = NULL
    bf.bfvQuotRem(self.b, x.v, y.v, &quot, &rem)
    return (wrapbv(self, quot), wrapbv(self, rem))

  cpdef bitvec vSdiv(self, bitvec x, bitvec y):
    return wrapbv(self, bf.bfvSdiv(self.b, x.v, y.v))

  cpdef bitvec vSrem(self, bitvec x, bitvec y):
    return wrapbv(self, bf.bfvSrem(self.b, x.v, y.v))

  cpdef vCopy(self, bitvec dst, bitvec src):
    bf.bfvCopy(self.b, dst.v, src.v)

  cpdef bitvec vDup(self, bitvec b):
    return wrapbv(self, bf.bfvDup(self.b, b.v))

  cpdef bitvec vConcat(self, bitvec x, bitvec y):
    """vConcat(x, y). Returns a bit vector of x and y concatenated together"""
    return wrapbv(self, bf.bfvConcat(self.b, x.v, y.v))

  cpdef bitvec vConcat3(self, bitvec x, bitvec y, bitvec z):
    return wrapbv(self, bf.bfvConcat3(self.b, x.v, y.v, z.v))

  cpdef bitvec vExtract(self, bitvec x, uint32_t begin, uint32_t length):
    return wrapbv(self, bf.bfvExtract(self.b, x.v, begin, length))

  cpdef bitvec vReverse(self, bitvec x):
    """Reverse the bits in x"""
    return wrapbv(self, bf.bfvReverse(self.b, x.v))

  cpdef bitvec vInit(self, uint32_t width):
    """Creates a new, unknown bit vector."""
    return wrapbv(self, bf.bfvInit(self.b, width))

  cpdef unsigned getNumVars(self):
    """Returns the number of variables in the underlying SAT
    instance"""
    return bf.bfNumVars(self.b)

  cpdef setNumVars(self, unsigned num):
    bf.bfSetNumVars(self.b, num)

  cpdef lSet(self, literal a):
    """Constrain a literal to be true in every assignment"""
    bf.bfSet(self.b, a.l)

  cpdef bf.mbool lGet(self, literal a):
    return literal(bf.bfGet(self.b, a.l), self)

  cpdef bf.bfstatus checkStatus(self, literal a) except? bf.STATUS_UNKNOWN:
    return bf.bfCheckStatus(self.b, a.l)

  cpdef bf.mbool mayBeTrue(self, literal x):
    """mustBeTrue(x). Returns mbool_true if there is an assignment that can
    make literal true; mbool_false othrewise (and mbool_unknown if the solver
    cannot determine the answer)."""
    return bf.bfMayBeTrue(self.b, x.l)

  cpdef bf.mbool mayBeFalse(self, literal x):
    """mustBeTrue(x). Returns mbool_true if there is an assignment that can
    make literal false; mbool_false othrewise (and mbool_unknown if the solver
    cannot determine the answer)."""
    return bf.bfMayBeFalse(self.b, x.l)

  cpdef bf.mbool mustBeTrue(self, literal x):
    """mustBeTrue(x). Returns mbool_true if there is no assignment that can
    make literal false; mbool_false othrewise (and mbool_unknown if the solver
    cannot determine the answer)."""
    return bf.bfMustBeTrue(self.b, x.l)

  cpdef bf.mbool mustBeFalse(self, literal x):
    """mustBeTrue(x). Returns mbool_true if there is no assignment that can
    make literal true; mbool_false othrewise (and mbool_unknown if the solver
    cannot determine the answer)."""
    return bf.bfMustBeFalse(self.b, x.l)

  cpdef vGet(self, bitvec bv):
    cdef unsigned int shift
    cdef bf.bitvec *shifted
    cdef bf.bitvec *truncated
    if len(bv) > 64:
      r = 0L
      for shift in range(0, len(bv), 64):
        shifted = bf.bfvRshiftConst(self.b, bv.v, shift, bf.LITERAL_FALSE)
        truncated = bf.bfvTruncate(self.b, 64, shifted)
        val = bf.bfvGet(self.b, truncated)
        r |= val << shift
      return r
    else:
      return bf.bfvGet(self.b, bv.v)

  def zero(self, uint32_t size):
    return bitvec.zero(self, size)

  cpdef bitvec pCOPY(self, bitvec input0):
    return wrapbv(self, bf.pCOPY(self.b, input0.v))

  cpdef bitvec pINT_ADD(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_ADD(self.b, input0.v, input1.v))

  cpdef bitvec pINT_SUB(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_SUB(self.b, input0.v, input1.v))

  cpdef bitvec pINT_MULT(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_MULT(self.b, input0.v, input1.v))

  cpdef bitvec pINT_DIV(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_DIV(self.b, input0.v, input1.v))

  cpdef bitvec pINT_REM(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_REM(self.b, input0.v, input1.v))

  cpdef bitvec pINT_SDIV(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_SDIV(self.b, input0.v, input1.v))

  cpdef bitvec pINT_SREM(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_SREM(self.b, input0.v, input1.v))

  cpdef bitvec pINT_OR(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_OR(self.b, input0.v, input1.v))

  cpdef bitvec pINT_XOR(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_XOR(self.b, input0.v, input1.v))

  cpdef bitvec pINT_AND(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_AND(self.b, input0.v, input1.v))

  cpdef bitvec pINT_LEFT(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_LEFT(self.b, input0.v, input1.v))

  cpdef bitvec pINT_RIGHT(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_RIGHT(self.b, input0.v, input1.v))

  cpdef bitvec pINT_SRIGHT(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_SRIGHT(self.b, input0.v, input1.v))

  cpdef bitvec pINT_EQUAL(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_EQUAL(self.b, input0.v, input1.v))

  cpdef bitvec pINT_NOTEQUAL(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_NOTEQUAL(self.b, input0.v, input1.v))

  cpdef bitvec pINT_LESS(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_LESS(self.b, input0.v, input1.v))

  cpdef bitvec pINT_LESSEQUAL(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_LESSEQUAL(self.b, input0.v, input1.v))

  cpdef bitvec pINT_CARRY(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_CARRY(self.b, input0.v, input1.v))

  cpdef bitvec pINT_SLESS(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_SLESS(self.b, input0.v, input1.v))

  cpdef bitvec pINT_SLESSEQUAL(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_SLESSEQUAL(self.b, input0.v, input1.v))

  cpdef bitvec pINT_SCARRY(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_SCARRY(self.b, input0.v, input1.v))

  cpdef bitvec pINT_SBORROW(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pINT_SBORROW(self.b, input0.v, input1.v))

  cpdef bitvec pBOOL_OR(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pBOOL_OR(self.b, input0.v, input1.v))

  cpdef bitvec pBOOL_XOR(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pBOOL_XOR(self.b, input0.v, input1.v))

  cpdef bitvec pBOOL_AND(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pBOOL_AND(self.b, input0.v, input1.v))

  cpdef bitvec pBOOL_NEGATE(self, bitvec input0):
    return wrapbv(self, bf.pBOOL_NEGATE(self.b, input0.v))

  cpdef bitvec pPIECE(self, bitvec input0, bitvec input1):
    return wrapbv(self, bf.pPIECE(self.b, input0.v, input1.v))

  cpdef bitvec pSUBPIECE(self, bitvec input0, uint32_t input1, uint32_t output_size):
    return wrapbv(self, bf.pSUBPIECE(self.b, input0.v, input1, output_size))

  cpdef bitvec pINT_ZEXT(self, bitvec input0, uint32_t output_size):
    return wrapbv(self, bf.pINT_ZEXT(self.b, input0.v, output_size))

  cpdef bitvec pINT_SEXT(self, bitvec input0, uint32_t output_size):
    return wrapbv(self, bf.pINT_SEXT(self.b, input0.v, output_size))

  cpdef bitvec pINT_NEGATE(self, bitvec input0):
    return wrapbv(self, bf.pINT_NEGATE(self.b, input0.v))

  cpdef bitvec pINT_2COMP(self, bitvec input0):
    return wrapbv(self, bf.pINT_2COMP(self.b, input0.v))

  cpdef memory mInit(self, int idxsize, int eltsize):
    mem = bf.bfmInit(self.b, idxsize, eltsize)
    mo = memory(self)
    mo.m = mem
    return mo

  cpdef memory mSelect(self, literal c, memory t, memory f):
    mem = bf.bfmSelect(self.b, c.l, t.m, f.m)
    mo = memory(self)
    mo.m = mem
    return mo

cdef class memory:
  """
  Represents a contiguous region of symbolic memory.

  The lifecycle of a memory:

    - creation: bfman.mInit()
    - loading and storing: python syntax
    - muxing: elseIfTrue(), elseIfFalse()
  """
  cdef bf.memory *m
  cdef bfman _bfman

  def __cinit__(self, bfman b):
    self._bfman = b

  cpdef idxSize(self):
    return bf.bfmIdxSize(self.m)

  cpdef eltSize(self):
    return bf.bfmEltSize(self.m)

  cpdef memory elseIfFalse(self, literal c, memory ifFalse):
    return self._bfman.mSelect(c, self, ifFalse)

  cpdef memory elseIfTrue(self, literal c, memory ifTrue):
    return self._bfman.mSelect(c, ifTrue, self)

  def __setitem__(self, addr, valu):
    if isinstance(addr, (int, long)):
      addr = self._bfman.vConstant(self.idxSize(), addr)
    if isinstance(valu, (int, long)):
      valu = self._bfman.vConstant(self.eltSize(), valu)
    self.store(addr, valu)

  def __getitem__(self, addr):
    if isinstance(addr, (int, long)):
      addr = self._bfman.vConstant(self.idxSize(), addr)
    return self.load(addr)

  cpdef store(self, bitvec addr, bitvec val):
    bf.bfmStore(self._bfman.b, self.m, addr.v, val.v)

  cpdef store_le(self, bitvec addr, bitvec val):
    bf.bfmStore_le(self._bfman.b, self.m, addr.v, val.v)

  cpdef store_be(self, bitvec addr, bitvec val):
    bf.bfmStore_be(self._bfman.b, self.m, addr.v, val.v)

#  cpdef mStoreVector_le(self, bitvec addr, bitvec val):
#    bf.bfmStoreVector_le(self._bfman.b, self.m, addr.v, val.v)
#
#  cpdef mStoreVector_be(self, bitvec addr, bitvec val):
#    bf.bfmStoreVector_be(self._bfman.b, self.m, addr.v, val.v)

  cpdef load(self, bitvec addr):
    return wrapbv(self._bfman, bf.bfmLoad(self._bfman.b, self.m, addr.v))

  cpdef load_rbw(self, bitvec addr):
    """Returns val, rbw. Load from address and return read-before-write
    condition. val is the result of the load; rbw is a literal. If rbw is true,
    then the val returned could be anything. (It was read before addr was
    written.)
    """
    cdef bf.literal *l = NULL
    b = wrapbv(self._bfman, bf.bfmLoad_RBW(self._bfman.b, self.m, addr.v, l)) 
    return b, literal(l[0], self._bfman)

  cpdef load_le(self, bitvec addr, int num):
    """Load num values starting at addr and combine them in little endian
    order"""
    return wrapbv(self._bfman, bf.bfmLoad_le(self._bfman.b, self.m, addr.v, num))

  cpdef load_be(self, bitvec addr, int num):
    return wrapbv(self._bfman, bf.bfmLoad_be(self._bfman.b, self.m, addr.v, num))

#  cpdef mLoadVector_le(self, bitvec addr, int num, int size):
#    return wrapbv(self._bfman.b, bf.bfmLoadVector_le(self._bfman.b, self.m, addr.v, num, size))

#  cpdef mLoadVector_be(self, bitvec addr, int num, int size):
#    return wrapbv(self._bfman.b, bf.bfmLoadVector_be(self._bfman.b, self.m, addr.v, num, size))

  cpdef copy(self):
    m = bf.bfmCopy(self._bfman.b, self.m)
    res = memory(self._bfman)
    res.m = m
    return res

  cpdef dump(self):
    bf.bfmPrint(self._bfman.b, stdout, self.m)

  cpdef memory fromCounterExample(self):
    return self._bfman.mFromCounterExample(self)

cdef memory wrapmem(bfman b, bf.memory *m):
  r = memory(b)
  r.m = m
  return r

cdef bitvec wrapbv(bfman b, bf.bitvec *v):
  w = bitvec()
  w._bfman = b
  w.v = bf.bfvHold(v)
  return w

