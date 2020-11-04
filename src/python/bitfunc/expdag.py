# Copyright 2012 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
# on behalf of the U.S. Government. Export of this program may require a license
# from the United States Government.

from math import ceil, log
from sys import stderr
from bitfunc import bitvec, literal
import bitfunc
import StringIO
import itertools
from copy import copy




# In order to solve expdags with bitfunc, you need to set the BF attribute to be
# a bitfunc.bfman instance. This global instance is used whenever conversion to
# bit vectors is necessary. It's not a parameter because usually you set it once
# and forget it.
BF = None

class RewriteBoundExceeded(Exception):
  pass

class RewriteContext(object):
  def __init__(self,
               rewrite_recursive_max_depth=4096,
               rewrite_level=3):
    self.rewrite_recursive_calls = 0
    self.rewrite_level = rewrite_level
    self.rewrite_recursive_max_depth = rewrite_recursive_max_depth

  def __enter__(self):
    if self.rewrite_recursive_calls >= self.rewrite_recursive_max_depth:
      raise RewriteBoundExceeded
    self.rewrite_recursive_calls += 1

  def __exit__(self, exc_type, exc_value, traceback):
    self.rewrite_recursive_calls -= 1

class Stats(object):
  def __init__(self):
    pass

  def __str__(self):
    s = 'Stats:\n'
    for attr,val in self.__dict__.iteritems():
      s += '  %s: %s\n' % (str(attr), str(val))
    return s

STATS = Stats()


class memoproperty(object):
  """A read-only @property that is only evaluated once.

Found here: http://www.reddit.com/r/Python/comments/ejp25/cached_property_decorator_that_is_memory_friendly/"""
  def __init__(self, fget, doc=None):
    self.fget = fget
    self.__doc__ = doc or fget.__doc__
    self.__name__ = fget.__name__

  def __get__(self, obj, cls):
    if obj is None:
      return self
    obj.__dict__[self.__name__] = result = self.fget(obj)
    return result

class OpNotImplemented(Exception):
  pass

def zero(width):
  return ConstExp(0, width=width)
def allones(width):
  return ConstExp(~0, width=width)

def mkIsZero(exp):
  return EqExp(exp, ConstExp(0, width=len(exp)))

def mkIsPos(exp, signed=False):
  if signed:
    return SGTExp(exp, ConstExp(0, width=len(exp)))
  else:
    return UGTExp(exp, ConstExp(0, width=len(exp)))


class ThreeVal(list):
  """a list of characters that represent the three-valued simulation of a bit
  vector"""
  def __init__(self, *args, **kwargs):
    super(ThreeVal, self).__init__(*args, **kwargs)
    for c in self:
      assert isinstance(c, basestring)
      assert c=='?' or c=='1' or c=='0'

  def __setitem__(self, i, c):
    assert c=='?' or c=='1' or c=='0'
    super(ThreeVal, self).__setitem__(i,c)

  def is_const(self):
    for c in self:
      if c == '?':
        return False
    return True

class BaseExp(object):
  """normally expdags should be immutable. if need be, though, you can call
  _rehash() after making a change.

  Expected properties of subclasses:
  width -- bit width of value (int)
  threeval -- three-valued simulation value (ThreeVal)"""

  shash = {}

  @staticmethod
  def find_node(typ, key):
    """search for another identical object, and return that if found"""
    return BaseExp.shash.get((typ, key))
  @staticmethod
  def add_node(typ, key, node):
    BaseExp.shash[(typ, key)] = node

  def __new__(typ, *args, **kwargs):
    # print >>stderr, '__new__ %s(%s, %s)' % (str(typ), str(args), str(kwargs))
    # Input & Unknown should always be constructed when asked
    if typ is InputExp or typ is UnknownExp:
      return object.__new__(typ, *args, **kwargs)
    key = (args, tuple(list(kwargs.iteritems())))
    obj = BaseExp.find_node(typ, key)
    if obj is None:
      obj = object.__new__(typ, *args, **kwargs)
      BaseExp.add_node(typ, key, obj)
    return obj

  def __init__(self, kids):
    """
    all subclasses should call this to set up the children. this method will
    check to make sure the children are all expressions.

    Attributes:
    simplified -- an equisatisfiable, but simpler, expression (BaseExp)
    """
    self.hashval = None
    self.kids = kids
    def check_kids():
      for k in kids:
        if not isinstance(k, BaseExp):
          return k
    assert check_kids() is None

  def set_kid(self, v, i):
    self.kids[i] = v
  k0 = property(lambda self: self.kids[0], lambda self,v: set_kid(v,0))
  k1 = property(lambda self: self.kids[1], lambda self,v: set_kid(v,1))

  simplified = property(lambda self: self)

  def is_const(self):
    return False

  def iterkids(self):
    return iter(self.kids)

  def set_width(self, w):
    raise OpNotImplemented
    
  def get_parent(self):
    return self.parent

  def __len__(self):
    """Get the bit width of this expression, in bits"""
    return self.width

  def _key(self):
    return (type(self), repr(map(hash, self.kids)), len(self))

  def _rehash(self):
    self.hashval = hash(self._key())

  def __hash__(self):
    if self.hashval is None:
      self._rehash()
    return self.hashval

  def __eq__(self, other):
    if self is other:
      return True
    elif hash(self) == hash(other):
      return self.kids == other.kids
    else:
      return False

  def get_name(self):
    """Returns a human readable name for the class. By default includes the
    width and the name of the class (e.g. UGTExp)"""
    return '[%d] ' % len(self) + self.__class__.__name__

  def __str__(self):
    return self.get_name()

  def as_bitvec(self, input_map):
    """
    returns a bit vector that represents the result of evaluating this node on
    the given input.
    """
    raise OpNotImplemented

  def as_dot(self):
    """Returns a string in DOT syntax that describes this expression."""
    s = StringIO.StringIO()
    as_dot_nodes({self: ''}, s)
    return s.getvalue()

class IdCounter:
  """
  This class works around the fact that python doesn't have static scoping, like
  a real language. It is used by `as_dot_nodes'.
  """
  pass

def as_dot_nodes(nodes, output,
                 number_edges=True,
                 root_color='lightgray',
                 input_color='yellow'):
  """
  Writes to `output' a graph in DOT syntax that describes all the expressions in
  the given iterable. 

  nodes -- a dictionary mapping BaseExps to names
  output -- anything with a `write(basestring)' method

  Keyword arguments:
  number_edges -- whether to number edges in the order they occur as kids
  root_color -- color used for the root nodes
  input_color -- color used for InputExp nodes
  """
  fmt_attr = lambda (var,val): '%s="%s"' % (var,val)
  def mkattrs(attrs):
    return ', '.join(map(fmt_attr, attrs))
  output.write('digraph G {\n')
  seen = set()
  queue = list(nodes.keys())
  roots = set(nodes.keys())
  inputs = set()
  ctr = IdCounter()
  ctr.val = 0
  ctr.ids = {}                    # each node gets an ID, mapped here
  def get_id(e):
    e_id = ctr.ids.get(e)
    if e_id is None:
      ctr.val += 1
      ctr.ids[e] = e_id = ctr.val
    return e_id

  while len(queue) > 0:
    e = queue.pop()
    if e in seen:
      continue
    e_id = get_id(e)

    if isinstance(e, BaseExp):
      logical_name = nodes.get(e)
      if logical_name is None:
        logical_name = ''
      else:
        logical_name = ' -- %s' % logical_name
      node_attrs = [('label', e.get_name()+logical_name),
                    ('shape','box')]
      if isinstance(e, InputExp):
        node_attrs += [('fillcolor', input_color),('style','filled')]
      elif e in roots:
        node_attrs += [('fillcolor', root_color),('style','filled')]
      output.write('  %d [%s]; // EDB\n' % (e_id, mkattrs(node_attrs)))
      ki = 0
      for k in e.iterkids():
        k_id = get_id(k)
        edge_attrs = []
        if number_edges:
          edge_attrs.append(('label', str(ki)))
        if ki == 0:
          edge_attrs.append(('color','blue'))
        output.write('  %d -> %d [%s]; // EDB edge\n' %\
                       (e_id, k_id, mkattrs(edge_attrs)))
        ki += 1

      if isinstance(e, LoadExp):
        k = e.upd
        k_id = get_id(k)
        edge_attrs = [('arrowhead', 'diamond')]
        output.write('  %d -> %d [%s]; // Load\n' % (e_id, k_id, mkattrs(edge_attrs)))
        queue.extend(iter([k]))

      queue.extend(e.iterkids())

    elif isinstance(e, ArrayUpdate):
      node_attrs = [('label', e.get_name()), ('shape', 'octagon')]
      output.write('  %d [%s]; // AU\n' % (e_id, mkattrs(node_attrs)))
      k = e.next
      if k is not None:
        k_id = get_id(k)
        edge_attrs = []
        output.write('  %d -> %d [%s]; // AU2\n' % (e_id, k_id, mkattrs(edge_attrs)))
        queue.extend(iter([e.next]))

    seen.add(e)
  output.write('}\n')

class Array(object):
  """
  An array represents a possibly-bounded, contiguous region of memory.

  Updates to an array are represented by an ArrayUpdate.
  """

  def __init__(self, idxsz, eltsz, base=None, length=None):
    """
    idxsz -- index (address) size in bits (int)
    eltsz -- element size in bits (int)

    Optional arguments:
    base -- first address of memory (int)
    len -- number of eltsz things this array can hold
    """
    self.idxsz = idxsz
    self.eltsz = eltsz
    self.base = base
    self.len = length

  def __len__(self):
    assert self.len is not None
    return self.len

  def __eq__(self, other):
    return self is other

class ArrayUpdate(object):
  """
  Abstract class that represents an update (some store operations) to an
  Array. The semantics of reading from a group depends on the specific
  group. Each group has specific aliasing semantics.

  An update is part of a singly-linked list of updates which is traversed by
  following `self.next'.

  Each ArrayUpdate knows how to find the update corresponding to a given
  address. In the simplest case, this just means returning a LoadExp on
  self. However, for particular ArrayUpdates, we can often prove that certain
  updates do not correspond to the address -- so symbolic execution of the read
  will be faster.
  """

  def __init__(self, arr, next_grp):
    """
    We need the array and the next ArrayUpdate.
    """
    self.arr = arr
    self.next = next_grp
    self.hashval = None

  @classmethod
  def mk(cls, *args, **kwargs):
    """Create an array update (no structural hashing)"""
    return cls(*args, **kwargs)

  def read(self, addr):
    """
    Just returns a LoadExp on this object
    """
    return LoadExp(addr, self)

  def get_name(self):
    return self.__class__.__name__

  def _rehash(self):
    self.hashval = hash(self._key())

  def __hash__(self):
    if self.hashval is None:
      self._rehash()
    return self.hashval

  def __eq__(self, other):
    if self is other:
      return True
    elif hash(self) == hash(other):
      return self.arr == other.arr and self.next == other.next
    else:
      return False

class VanillaUpdateGroup(ArrayUpdate):
  """
  Represents a strict sequence of updates that may alias each other. This
  corresponds to the most general memory model, because it conservatively
  assumes that any update may alias any other.
  """

  def __init__(self, arr, next_grp=None):
    super(VanillaUpdateGroup, self).__init__(arr, next_grp)
    self.mappings = []

  def update(self, addr, valu):
    assert isinstance(addr, BaseExp)
    assert isinstance(valu, BaseExp)
    assert len(addr) == self.arr.idxsz
    assert len(valu) == self.arr.eltsz
    self.mappings.append((addr, valu))

  def with_update(self, addr, valu):
    return VanillaUpdateGroup(self.arr, self)

  def __eq__(self, other):
    return super(VanillaUpdateGroup, self).__eq__(self, other) and\
        self.mappings == other.mappings

  def _key(self):
    return (type(self), self.arr, repr(map(hash, self.mappings)), self.next)

class DisjointUpdateGroup(ArrayUpdate):
  """
  Represents a group of mutually-disjoint updates. Pairwise, all the addresses
  in this object do not alias each other, but may alias writes in the `next'
  DisjointUpdateGroup.

  Semantically, reading from this group means reading from this group of writes
  first, only afterward examining the `self.next' group of writes.

  Subclasses may override read() to more efficiently prune the resulting
  expression.
  """

  def __init__(self, arr, addr, valu, next_g=None):
    # (addr, valu) pairs of ExpDag objects. last is most recent.
    self.mappings = {addr: valu}
    super(DisjointUpdateGroup, self).__init__(arr, next_g)

  def update(self, addr, valu):
    """add mappings in place"""
    self.mappings[addr] = valu

  def read(self, addr):
    return LoadExp(addr, self)

class BlockUpdate(ArrayUpdate):
  """
  Represents an entire block update (contiguous range of memory
  addresses). `start' should be an address and `contents' a list of
  expressions. All the expressions must have the same width.
  """
  def __init__(self, arr, start, contents, next_g=None):
    assert len(contents) > 0
    self.start = start
    self.contents = contents
    width = len(self.contents[0])
    def check_width():
      for x in self.contents:
        if width != len(x):
          return False
    assert check_width()
    super(BlockUpdate, self).__init__(arr, next_g)



##############################################################################
# Expression types
##############################################################################

# Each expression has a threeval property.
#   normalize(cxt) -- returns a normalized versions of the dag rooted at
#   self. takes a RewriteContext
#
#   rewrite(cxt) -- returns a simplified version of the dag rooted at
#   self. takes a RewriteContext


# Normalization
# - constant arguments on the left or something

def const_to_threeval(c, width):
  """returns the threeval for any python constant at the given width. if the
  width is too small, then bits will be missing."""
  tv = ThreeVal(itertools.repeat('?', width))
  for i in xrange(width):
    if c%2 == 1:
      tv[width-i-1] = '1'
    else:
      tv[width-i-1] = '0'
    c /= 2
  return tv

def threeval_to_const(tv):
  """returns a python constant for the given threeval"""
  def tv_char_to_int(i):
    if tv[i] == '1': return 1
    elif tv[i] == '0': return 0
    else: assert False

  # uses the definition of 2s complement
  m = len(tv)
  ret = -tv_char_to_int(0)*(2**(m-1))
  for i in xrange(m-1):
    ret += tv_char_to_int(m-i-1)*(2**i)
  return ret

def isneginstance(exp, ty):
  if isinstance(exp, InvertExp):
    chld = exp.k0
    return isinstance(chld, ty)
  return False
def isnegof(n0, n1):
  """returns True if n0 is InvertExp(n1) or vice versa"""
  return (isinstance(n0, InvertExp) and n0.k0 is n1) or\
      (isinstance(n1, InvertExp) and n1.k0 is n0)
  

class ConstExp(BaseExp):
  """constants -- turn an int into its 2s complement representation"""
  def __init__(self, const, width=None):
    super(ConstExp, self).__init__([])
    width_min = 1
    if width is None:
      if const == 0:
        width_min = 1
      elif const > 0:
        width_min = int(ceil(log(const, 2)))
      elif const < 0:
        # make room for the sign bit
        width_min = int(ceil(log(abs(const)<<1, 2)))
      width = width_min
    assert const == 0 or width >= width_min

    self.width = width
    self.const = const

  threeval = memoproperty(lambda self: const_to_threeval(self.const, self.width))

  def __str__(self):
    return 'ConstExp(%xh)' % self.const

  def __eq__(self, other):
    if BaseExp.__eq__(self, other):
      if isinstance(other, ConstExp):
        return self.const == other.const
    return False

  def get_name(self):
    return super(ConstExp, self).get_name() + ('(%xh)' % self.const)

  def is_const(self):
    return True

  def is_zero(self):
    return 0 == self.const
  def is_one(self):
    return 1 == self.const
  def is_allones(self):
    return 0 == ~self.const

  def as_bitvec(self, input_map={}):
    bv = bitvec.alloc(self.width)
    for c in reversed(self.threeval):
      bv.vPush(literal.fromBool(c == '1'))
    assert self.width == len(bv)
    return bv
    # c = self.uconst
    # b = bitvec.alloc(len(self))
    # for i in xrange(len(self)):
    #   b.vPush(literal.fromBool(c & 0x1))
    #   c = c >> 1
    # return b

class InputExp(BaseExp):
  """symbolic input bit vector"""
  def __init__(self, width):
    super(InputExp, self).__init__([])
    self.width = width

  threeval = property(lambda self: ThreeVal(itertools.repeat('?', self.width)))

  def __eq__(self, other):
    return self is other

  def as_bitvec(self, input_map={}):
    return input_map[self]

class UnknownExp(BaseExp):
  """unknown but not symbolic bit vector"""
  def __init__(self, width):
    super(UnknownExp, self).__init__([])
    self.width = width

  def __eq__(self, other):
    return self is other

class InvertExp(BaseExp):
  """invert all the bits"""

  def __new__(typ, *args, **kwargs):
    assert len(args)==1
    if isinstance(args[0], InvertExp): # InvertExp(InvertExp(x)) --> x
      return args[0].k0
    else:
      return BaseExp.__new__(typ, *args, **kwargs)

  def __init__(self, x):
    super(InvertExp, self).__init__([x])
    self.width = x.width

  def __len__(self):
    return len(self.kids[0])

  truth_table = {
    '0': '1',
    '1': '0',
    }

  @classmethod
  def threeop(cls, x):
    result = ThreeVal(itertools.repeat('?', len(x)))
    for i in xrange(len(x)):
      result[i] = cls.truth_table[x[i]]
    return result

  def as_bitvec(self, input_map={}):
    return BF.vInvert(self.kids[0].as_bitvec(input_map))

class BinOpExp(BaseExp):
  """
  common superclass of binary operator expressions.

  kids is assumed to have at least 2 elements. if it has more, then it had
  better be left associative.

  Subclass methods:
  threeopimpl -- 
  """
  
  def as_bitvec(self, input_map={}):
    """
    self.bvop is used to evaluate this node. if self.bvop is a string, it is
    assumed to be a function of the bfman. If it is callable, it will be called
    with two bit vector arguments.
    """
    if isinstance(self.bvop, basestring):
      f = getattr(BF, self.bvop)
    else:
      f = self.bvop

    v0 = self.k0.as_bitvec(input_map)
    v1 = self.k1.as_bitvec(input_map)

    v = f(v0, v1)
    if isinstance(v, bitfunc.literal):
      v = bitvec.fromLiteral(v)

    return v

  threeval = memoproperty(lambda self: self.threeop(self.k0, self.k1))

  @classmethod
  def threeop(cls, x, y, **kwargs):
    """3-valued simulation"""
    assert isinstance(x, basestring) or isinstance(x, ThreeVal)
    assert isinstance(y, basestring) or isinstance(y, ThreeVal)
    unpackret = False
    if isinstance(x, basestring):
      assert isinstance(y, basestring)
      assert len(x) == 1
      x = ThreeVal([x])
      y = ThreeVal([y])
      unpackret = True
    assert len(x) == len(y)
    ret = cls.threeopimpl(x, y, **kwargs)
    if unpackret:
      return ret[0]
    else:
      return ret

  @classmethod
  def threeopimpl(cls, x, y, **kwargs):
    """default threeopimpl -- calls cls.threeopsingle(x[i], y[i])"""
    ret = ThreeVal(itertools.repeat('?', len(x)))
    for i in xrange(len(x)):
      ret[i] = cls.threeopsingle(x[i], y[i])
    return ret

  @classmethod
  def threeopsingle(cls, x, y):
    """
    default threeopsingle -- if either arg is unknown, then
    unknown. otherwise, return whatever the truth table says.
    """
    if '?' == x or '?' == y: return '?'
    return cls.truth_table[(x,y)]

  @classmethod
  def normalize_comm_ass(cls, cxt, k0, k1):
    """normalize a commutative, associative operation. splits up the left and
    right sides in an attempt to find common arguments to a whole chain of the
    same binary operator

    cxt -- a RewriteContext
    k0,k1 -- the left and right child of a binary op"""
    left   = {}
    right  = {}
    common = {}

    # look @ the right dag, putting all subexps that aren't this op into the
    # `right' dictionary.
    q = [k1]
    while len(q) > 0:
      x = q.pop()
      if isinstance(x, cls):
        q.append(x.k0)
        q.append(x.k1)
      else:
        in_right = right.get(x)
        if in_right is None:
          right[x] = 1
        else:
          right[x] = in_right+1

    # look @ the left dag, attempting to discover common operands. if an operand
    # is common, put it into the `common' dictionary and remove it from `left'
    # and `right'.
    q = [k0]
    while len(q) > 0:
      x = q.pop()
      if isinstance(x, cls):
        q.append(x.k0)
        q.append(x.k1)
      else:
        in_right = right.get(x)
        if in_right is None:
          # we haven't seen this on the right side of anything; put it in `left'
          in_left = left.get(x)
          if in_left is None:
            left[x] = 1
          else:
            left[x] = in_left+1
        else:
          # found common operand
          if in_right > 1:
            right[x] = in_right-1
          else:
            del right[x]
          in_common = common.get(x)
          if in_common is None:
            common[x] = 1
          else:
            common[x] = in_common+1

    if len(common) > 2:
      print >>stderr, 'found %d commons' % len(common)
      commonexp = None
      for x,c in common.iteritems():
        # exp x occurs c times in common
        for _ in xrange(c):
          if commonexp is None:
            commonexp = copy(x)
            continue
          commonexp = cls.rewrite(cxt, x, commonexp)
      k1 = copy(commonexp)
      for x,c in right.iteritems():
        for i in xrange(c):
          k1 = cls.rewrite(cxt, k1, x)
      k0 = copy(commonexp)
      for x,c in left.iteritems():
        for i in xrange(c):
          k0 = cls.rewrite(cxt, k0, x)
    return cls.rewrite(cxt, k0, k1)

  def rewrite_binary_exp(self, k0, k1, cxt):
    result = self
    if k0.is_const() and k1.is_const():
      b0 = k0.threeval
      b1 = k1.threeval
      result = ConstExp(threeval_to_const(self.threeop(b0, b1)), self.width)
    elif k0.is_const() and k0.is_zero():
      if isinstance(self, EqExp):
        if k0.width == 1:
          result = InvertExp(k1)
        elif isinstance(k1, XorExp):
          # 0 == (a^b) --> a=b
          a = k1.k0
          b = k1.k1
          try:
            with cxt:
              result = EqExp(a,b).rewrite(cxt)
          except:
            pass
        elif isneginstance(k1, AndExp):
          # 0 == a|b --> a==0 and b==0
          a = k1.k0
          b = k1.k1
          try:
            with cxt:
              left = InvertExp(EqExp(a, k0)).rewrite(cxt)
              right = InvertExp(EqExp(b, k0)).rewrite(cxt)
              result = AndExp(left, right).rewrite(cxt)
          except:
            pass
      elif isinstance(self, ULTExp):
        # 0 < k1 --> k1 != 0
        result = InvertExp(EqExp(k0, k1)).rewrite(cxt)
      elif isinstance(self, AddExp):
        # 0 + k1 --> k1
        result = k1
      elif isinstance(self, MulExp) or isinstance(self, ShlExp) or\
            isinstance(self, LShrExp) or isinstance(self, URemExp) or\
            isinstance(self, AndExp):
        result = ConstExp(0, len(k0))
    return result
    

class LoadExp(BaseExp):
  """load from an array"""
  def __init__(self, idx, upd):
    self.idx = idx
    self.upd = upd
    super(LoadExp, self).__init__([self.idx])

  def _key(self):
    return (self.upd, super(LoadExp, self)._key())

  def __len__(self):
    return self.upd.arr.eltsz

class ConcatExp(BinOpExp):
  """concatenation of two expressions"""
  def __init__(self, x, y):
    super(ConcatExp, self).__init__([x,y])
    self.width = len(x)+len(y)
    self.bvop = 'vConcat'

  def __len__(self):
    return self.width

class ExtractExp(BaseExp):
  """extraction (sub-bit vector selection)"""
  def __init__(self, exp, start, length):
    assert start < len(exp)
    assert start+length <= len(exp)
    self.exp = exp
    self.start = start
    self.length = length
    super(ExtractExp, self).__init__([self.exp])

  def __len__(self):
    return self.length

  def as_bitvec(self, input_map={}):
    return BF.vExtract(self.exp.as_bitvec(BF, input_map), self.start, self.length)

class ReverseExp(BaseExp):
  """bit reversal"""
  def __init__(self, exp):
    self.exp = exp
    super(ReverseExp, self).__init__([self.exp])

  def as_bitvec(self, input_map={}):
    return BF.vReverse(self.exp.as_bitvec(BF, input_map))

  def __len__(self):
    return len(self.exp)

  @classmethod
  def threeop(cls, tv0):
    return ThreeVal(list(reversed(tv0)))

  @classmethod
  def rewrite(cls, cxt, k0):
    k0 = k0.simplified
    tvo = ThreeVal(itertools.repeat('?', len(k0)))
    result = cls(k0)
    width = len(k0)
    if cxt.rewrite_level > 1:
      tv0 = k0.threeval
      tvo = cls.threeop(tv0)
      if tvo.is_const():
        return ConstExp(threeval_to_const(tvo), width)

    return result

class ZExtExp(BaseExp):
  def __init__(self, exp, new_width):
    assert new_width >= len(exp)
    self.exp = exp
    self.width = new_width
    super(ZExtExp, self).__init__([self.exp])

  def as_bitvec(self, input_map={}):
    return BF.vZextend(self.width, self.exp.as_bitvec(BF, input_map))

class SExtExp(BaseExp):
  def __init__(self, exp, new_width):
    assert new_width >= len(exp)
    self.exp = exp
    self.width = new_width
    super(SExtExp, self).__init__(args)

  def as_bitvec(self, input_map={}):
    return BF.vSextend(self.width, self.exp.as_bitvec(BF, input_map))

class AddExp(BinOpExp):
  def __init__(self, x, y):
    # assert carry_in == 0 or carry_in == 1
    # self.carry_in = carry_in
    kids = [x, y]
    assert len(x) == len(y)
    super(AddExp, self).__init__(kids)
    self.width = len(x)
    self.bvop = 'vAdd0'
    
  @classmethod
  def threeopimpl(cls, x, y, carry='0'):
    """3-valued simulation"""
    result = ThreeVal(itertools.repeat('?', len(x)))
    for i in reversed(xrange(len(x))):
      result[i] = XorExp.threeop(carry, XorExp.threeop(x[i], y[i]))
      carry = MuxExp.threeop(carry, OrExp.threeop(x[i], y[i]), AndExp.threeop(x[i], y[i]))[0]
    return result

  @classmethod
  def rewrite(cls, cxt, k0, k1):
    """rewrite two children k0 and k1 of an AddExp"""
    assert len(k0) == len(k1)
    k0 = k0.simplified
    k1 = k1.simplified
    tvo = ThreeVal(itertools.repeat('?', len(k0)))
    result = cls(k0, k1)
    width = len(k0)
    if cxt.rewrite_level > 1:
      tv0 = k0.threeval
      tv1 = k1.threeval
      tvo = cls.threeop(tv0, tv1)
      if tvo.is_const():
        return ConstExp(threeval_to_const(tvo), width)


    return cls(k0, k1)

    if width == 1:              # a+b is xor at bit width 1
      try:
        with cxt:
          result = XorExp.rewrite(cxt, k0, k1)
      except:
        pass

    else:                       # width>1
      if k0 is isnegof(k1, k0): # a - a = 0
        result = zero(width)
      elif k0.is_const() and isinstance(k1, cls):
        if k1.k0.is_const():    # c1 + (c2+y)
          try:
            with cxt:
              tmp = cls.rewrite(cxt, k0, k1.k0)     # c1+c2
              result = cls.rewrite(cxt, tmp, k1.k1) # ... + y
          except:
            pass
        elif k1.k1.is_const():  # c1 + (y+c2)
          try:
            with cxt:
              tmp = cls.rewrite(cxt, k0, k1.k1)     # c1+c2
              result = cls(tmp, k1.k0).rewrite(cxt) # ... +y
          except:
            pass
      elif k1.is_const() and isinstance(k0, cls): # symmetric case
        if k0.k1.is_const():
          try:
            with cxt:
              tmp = cls(k1, k0.k0).rewrite(cxt)
              result = cls(tmp, k0.k1).rewrite(cxt)
          except:
            result = self
        elif k0.k1.is_const():
          try:
            with cxt:
              tmp = cls(k1, k0.k1).rewrite(cxt)
              result = cls(tmp, k0.k0).rewrite(cxt)
          except:
            result = self

      if cxt.rewrite_level > 2 and isinstance(k0, MulExp) and isinstance(k1, MulExp):
        k0 = k0.normalize(cxt)
        k1 = k1.normalize(cxt)
        
      result = self.rewrite_binary_exp(k0, k1, cxt)
      if result is None:
        result = self

    return result

class SubExp(BinOpExp):
  def __init__(self, x, y):
    kids = [x,y]
    assert len(x) == len(y)
    super(SubExp, self).__init__(kids)
    self.width = len(x)
    self.bvop = 'vSubtract'

  @classmethod
  def threeopimpl(cls, x, y):
    """3-valued simulation"""
    return AddExp.threeop(x, InvertExp.threeop(y), carry='1')

  @classmethod
  def rewrite(cls, cxt, k0, k1):
    """rewrite two children k0 and k1 of a SubExp"""
    assert len(k0) == len(k1)
    k0 = k0.simplified
    k1 = k1.simplified
    tvo = ThreeVal(itertools.repeat('?', len(k0)))
    result = cls(k0, k1)
    width = len(k0)
    if cxt.rewrite_level > 1:
      tv0 = k0.threeval
      tv1 = k1.threeval
      tvo = cls.threeop(tv0, tv1)
      if tvo.is_const():
        return ConstExp(threeval_to_const(tvo), width)


    return cls(k0, k1)


class MulExp(BinOpExp):
  def __init__(self, x, y):
    kids = [x,y]
    assert len(x) == len(y)
    self.width = len(x)
    super(MulExp, self).__init__(kids)
    self.bvop = 'vMul'

class UDivExp(BaseExp):
  def __init__(self, x, y):
    assert len(x)==len(y)
    kids = [x,y]
    super(UDivExp, self).__init__(kids)
    self.width = len(x)

  def as_bitvec(self, input_map={}):
    (q, r) = BF.vQuotRem(self.kids[0].as_bitvec(input_map),
                         self.kids[1].as_bitvec(input_map))
    return q

class SDivExp(BaseExp):
  def __init__(self, x, y):
    assert len(x)==len(y)
    kids = [x,y]
    super(SDivExp, self).__init__(kids)
    self.width = len(x)

  def as_bitvec(self, input_map={}):
    return BF.vSdiv(self.kids[0].as_bitvec(input_map),
                    self.kids[1].as_bitvec(input_map))

class URemExp(BaseExp):
  def __init__(self, x, y):
    assert len(x)==len(y)
    kids = [x,y]
    super(URemExp, self).__init__(kids)
    self.width = len(x)

  def as_bitvec(self, input_map={}):
    (q, r) = BF.vQuotRem(self.kids[0].as_bitvec(input_map),
                         self.kids[1].as_bitvec(input_map))
    return r


class SRemExp(BaseExp):
  def __init__(self, x, y):
    assert len(x)==len(y)
    kids = [x,y]
    super(SRemExp, self).__init__(kids)
    self.width = len(x)

  def as_bitvec(self, input_map={}):
    return BF.vSrem(self.kids[0].as_bitvec(input_map),
                    self.kids[1].as_bitvec(input_map))

class AndExp(BinOpExp):
  """
  Logical and bitwise and.

  If the expressions are both 1 bit, it's a logical AND. Otherwise, it's a bitwise AND.
  """
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(AndExp, self).__init__(kids)
    self.width = len(x)
    self.bvop = 'vAnd'

  truth_table = {
    ('0','0'): '0',
    ('0','1'): '0',
    ('1','0'): '0',
    ('1','1'): '1',
    }

class OrExp(BinOpExp):
  """Logical and bitwise or. See AndExp"""
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(OrExp, self).__init__(kids)
    self.width = len(x)
    self.bvop = 'vOr'

  truth_table = {
    ('0','0'): '0',
    ('0','1'): '1',
    ('1','0'): '1',
    ('1','1'): '1',
    }

class ImpliesExp(BinOpExp):
  """implication"""
  def __init__(self, x, y):
    assert len(x) == 1
    assert len(x) == len(y)
    kids = [x,y]
    super(ImpliesExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vImplies'

  truth_table = {
    ('0','0'): '1',
    ('0','1'): '1',
    ('1','0'): '0',
    ('1','1'): '1',
    }

class XorExp(BinOpExp):
  """Logical and bitwise xor. See AndExp"""
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(XorExp, self).__init__(kids)
    self.width = len(x)
    self.bvop = 'vXor'

  @classmethod
  def rewrite(cls, cxt, k0, k1):
    return cls(k0, k1)

  truth_table = {
    ('0','0'): '0',
    ('0','1'): '1',
    ('1','0'): '1',
    ('1','1'): '0',
    }

class ShlExp(BaseExp):
  def __init__(self, v, d):
    l = len(v)
    assert (l & (l-1)) == 0 # width not a power of two
    kids = [v,d]
    super(ShlExp, self).__init__(kids)
    self.width = len(v)
    self.bvop = 'vLshift'

class LShrExp(BinOpExp):
  def __init__(self, v, d):
    l = len(v)
    assert (l & (l-1)) == 0 and 'width not a power of two'
    kids = [v,d]
    super(LShrExp, self).__init__(kids)
    self.width = len(v)
    self.bvop = lambda v,d: BF.vRshift(v, d, bitfunc.LITERAL_FALSE)

class AShrExp(BinOpExp):
  def __init__(self, v, d):
    l = len(v)
    assert (l & (l-1)) == 0 # width not a power of two
    kids = [v,d]
    super(AShrExp, self).__init__(kids)
    self.width = len(v)
    self.bvop = lambda v,d: BF.vRshift(v, d, v[len(v)-1])

class EqExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    super(EqExp, self).__init__([x,y])
    self.width = 1
    self.bvop = 'vEqual'

class NEExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(NEExp, self).__init__(kids)
    self.width = 1
    self.bvop = lambda x,y: -BF.vEqual(x,y)

class ULTExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(ULTExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vUlt'

class UGTExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(UGTExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vUgt'


class ULEExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(ULEExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vUlte'


class UGEExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(UGEExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vUgte'


class SLTExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(SLTExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vSlt'


class SLEExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(SLEExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vSlte'


class SGTExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(SGTExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vSgt'


class SGEExp(BinOpExp):
  def __init__(self, x, y):
    assert len(x) == len(y)
    kids = [x,y]
    super(SGEExp, self).__init__(kids)
    self.width = 1
    self.bvop = 'vSgte'

class MuxExp(BaseExp):
  def __init__(self, c, t, e):
    assert len(c)==1
    assert len(t)==len(e)
    kids = [c,t,e]
    super(MuxExp, self).__init__(kids)
    self.width = len(t)

  def as_bitvec(self, input_map={}):
    c = self.kids[0].as_bitvec(input_map).bitvec
    t = self.kids[1].as_bitvec(input_map).bitvec
    e = self.kids[2].as_bitvec(input_map).bitvec
    return BF.vSelect(c[0], t, e)

  @classmethod
  def threeopsingle(cls, tvc, tvt, tve):
    """mux three-valued bits"""
    assert isinstance(tvc, basestring) and isinstance(tvt, basestring) and\
        isinstance(tve, basestring)
    if tvt==tve:
      return tvt
    elif tvc=='?':
      return '?'
    elif tvc=='1':
      return tvt
    elif tvc=='0':
      return tve
    assert False

  @classmethod
  def threeop(cls, tvc, tvt, tve):
    assert len(tvt) == len(tve)
    ret = ThreeVal(itertools.repeat('?', len(tvt)))
    if isinstance(tvc, basestring):
      cond = tvc
    else:
      assert len(tvc)==1
      cond = tvc[0]
    for i in xrange(len(tvt)):
      ret[i] = cls.threeopsingle(cond, tvt[i], tve[i])
    return ret

class memoized(object):
  '''Decorator. Caches a function's return value each time it is called.
  If called later with the same arguments, the cached value is returned 
  (not reevaluated).
  '''
  def __init__(self, func):
    self.func = func
    self.cache = {}
  def __call__(self, *args):
    try:
      return self.cache[args]
    except KeyError:
      value = self.func(*args)
      self.cache[args] = value
      return value
    except TypeError:
      # uncachable -- for instance, passing a list as an argument.
      # Better to not cache than to blow up entirely.
      return self.func(*args)
  def __repr__(self):
    '''Return the function's docstring.'''
    return self.func.__doc__
  def __get__(self, obj, objtype):
    '''Support instance methods.'''
    return functools.partial(self.__call__, obj)
