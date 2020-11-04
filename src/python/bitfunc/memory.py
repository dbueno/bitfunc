# Copyright 2012 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
# on behalf of the U.S. Government. Export of this program may require a license
# from the United States Government.

import bitfunc as bf
import sys

from collections import OrderedDict, namedtuple
from itertools import *
import heapq

import __builtin__ as bi

def trace(*args, **kwargs):
  getattr(bi, 'print')(*args, **kwargs)
  return args[-1]

class MemoryBase(object):

  _instancecount = 0

  __slots__ = ('b')

  def __new__(cls, *args, **kwargs):
    if not hasattr(cls,'_instancecount'):
      cls._instancecount = 1
    else:
      cls._instancecount += 1
    return object.__new__(cls, args, kwargs)

  def __del__(self):
    self.__class__._instancecount -= 1

  def __init__(self, b):
    self.b = b

  def mLoad_le(self, address, width):
    #print 'load'
    ret = self.b.vAlloc(0)
    for o in xrange(width):
      #print 'byte',o
      a = self.b.vAdd0(address, self.b.vUconstant(len(address), o))
      v = self.read(a)
      ret = self.b.vConcat(ret, v)
    #print
    return ret

  def mStore_le(self, address, value):
    nbytes = len(value)/8
    for o in xrange(nbytes):
      a = self.b.vAdd0(address, self.b.vUconstant(len(address), o))
      v = self.b.vExtract(value, o*8, 8)
      self.write(a, v)

class PageMem(MemoryBase):

  WriteInfo = namedtuple('WriteInfo', 'address value time')

  def __init__(self, parent, address_width, page_size):
    MemoryBase.__init__(self, parent.b)
    self.parent = parent
    self.npages = 2**(address_width-page_size)
    self.pages = [list() for x in range(self.npages)]
    self.address_width = address_width
    self.page_size = page_size
    self.clock = 0
    self.holes = 0

  def page_base(self, pagen):
    return pagen*2**self.page_size

  def lists_for_address(self, address):
    def generator(lo, hi):
      if lo + 1 == hi:
        yield self.pages[lo]
      else:
        mid = (lo+hi)//2
        midbv = self.b.vUconstant(self.address_width, self.page_base(mid))
        st = self.b.checkStatus(self.b.vUlt(address, midbv))
        if st == bf.STATUS_MUST_BE_TRUE:
          for x in generator(lo, mid):
            yield x
        elif st == bf.STATUS_MUST_BE_FALSE:
          for x in generator(mid, hi):
            yield x
        elif st == bf.STATUS_TRUE_OR_FALSE:
          for x in generator(lo, mid):
            yield x
          for x in generator(mid, hi):
            yield x
    return generator(0, self.npages)

  def write(self, address, value):
    w = PageMem.WriteInfo(address, value, self.clock)
    self.clock += 1
    lists = list(self.lists_for_address(address))
    for l in lists:
      self.add_write(w, l)
    #self.cleanup()

  def add_write(self, w, l):
    holes = 0
    for i in xrange(len(l)):
      ow = l[i]
      if ow is None:
        holes += 1
      elif self.b.checkStatus(self.b.vEqual(ow.address, w.address)) == bf.STATUS_MUST_BE_TRUE:
        l[i] = None
        holes += 1
    if holes * 2 > len(l):
      keep = [x for x in l if x is not None]
      del l[:]
      l.extend(keep)
    l.append(w)

  def cleanup(self):
    for i in xrange(self.npages):
      self.pages[i] = [w for w in self.pages[i] if w is not None]
    self.holes = 0

  def read(self, address):
    iters = [((-x.time,x) for x in reversed(l) if x is not None) for l in self.lists_for_address(address)]
    writes = heapq.merge(*iters)
    candidates = list()
    base_value = None
    for t1,w in writes:
      if w is None:
        continue
      a,v,t = w
      eq = self.b.vEqual(address, a)
      st = self.b.checkStatus(eq)
      if st == bf.STATUS_MUST_BE_TRUE:
        base_value = v
        break
      elif not st == bf.STATUS_MUST_BE_FALSE:
        candidates.append((eq,v))
    if False:
      if len(candidates) != 0 or base_value is not None:
        print '%x %d/%d %s' % (id(self), len(candidates),len(writes),base_value is not None)
      else:
        sys.stdout.write('.')
        sys.stdout.flush()
    if base_value is None:
      #print 'read parent'
      base_value = self.parent.read(address)
    def reducer(prev_val, candidate):
      return self.b.vSelect(candidate[0], candidate[1], prev_val)
    val = reduce(reducer, reversed(candidates), base_value)
    return val

  def mCopy(self):
    return PageMem(self, self.address_width, self.page_size)

class WriteableMem(MemoryBase):

  __slots__ = ('parent','writes','holes')

  def __init__(self, parent):
    MemoryBase.__init__(self, parent.b)
    self.parent = parent
    self.writes = list()
    self.holes = 0

  def write(self, address, value):
    if True:
      for i in xrange(len(self.writes)):
        ai = self.writes[i][0]
        #if ai is not None and self.b.vSame(ai, address) == 1:
        if ai is not None and self.b.checkStatus(self.b.vEqual(ai, address)) == bf.STATUS_MUST_BE_TRUE:
          #print 'replaced %d' % i
          self.writes[i] = (None,None)
          self.holes += 1
    self.writes.append((address, value))
    if self.holes > 100:
      self.cleanup()

  def cleanup(self):
    s = len(self.writes)
    self.writes = [w for w in self.writes if w[0] is not None]
    self.holes = 0
    #print 'cleanup %d/%d' % (len(self.writes), s)

  def read(self, address):
    if not bf.bfvIsConstant(address):
      print 'symbolic read'
    candidates = list()
    base_value = None
    for a,v in reversed(self.writes):
      if a is None:
        continue
      eq = self.b.vEqual(address, a)
      st = self.b.checkStatus(eq)
      if st == bf.STATUS_MUST_BE_TRUE:
        base_value = v
        break
      elif not st == bf.STATUS_MUST_BE_FALSE:
        candidates.append((eq,v))
    if False:
      if len(candidates) != 0 or base_value is not None:
        print '%x %d/%d %s' % (id(self), len(candidates),len(self.writes),base_value is not None)
      else:
        sys.stdout.write('.')
        sys.stdout.flush()
    if base_value is None:
      #print 'read parent'
      base_value = self.parent.read(address)
    def reducer(prev_val, candidate):
      return self.b.vSelect(candidate[0], candidate[1], prev_val)
    val = reduce(reducer, reversed(candidates), base_value)
    #self.writes.append((address, val))
    return val

  def mCopy1(self):
    return WriteableMem(self)

  def mCopy(self):
    dup = WriteableMem(self.parent)
    dup.writes = list(self.writes)
    dup.cleanup()
    return dup

class WriteableXMem(MemoryBase):

  __slots__ = ('parent','writes')

  def __init__(self, parent):
    MemoryBase.__init__(self, parent.b)
    self.parent = parent
    self.writes = OrderedDict()

  def write(self, address, value):
    if address in self.writes:
      del self.writes[address]

    self.writes[address] = value

  def read(self, address):
    if False and address in self.writes:
      candidates = islice(dropwhile(lambda x: x != address, self.writes), 1)
      base_value = self[address]
    else:
      candidates = self.writes
      base_value = self.parent.read(address)
    def reducer(prev_val, candidate_address):
      return self.b.vSelect(self.b.vEqual(candidate_address, address), self.writes[candidate_address], prev_val)
    return reduce(reducer, candidates, base_value)

  def mCopy(self):
    return WriteableMem(self)

class SelectMem(MemoryBase):

  __slots__ = ('cond','iftrue','iffalse')

  def __init__(self, cond, iftrue, iffalse):
    assert(iftrue.b is iffalse.b)
    MemoryBase.__init__(self, iftrue.b)
    self.cond = cond
    self.iftrue = iftrue
    self.iffalse = iffalse

  def read(self, address):
    #print 'read select',
    if self.b.lIsTrue(self.cond):
      #print 'true'
      return self.iftrue.read(address)
    elif self.b.lIsFalse(self.cond):
      #print 'false'
      return self.iffalse.read(address)
    else:
      #print 'both'
      return self.b.vSelect(self.cond, self.iftrue.read(address), self.iffalse.read(address))

class ZeroMem(MemoryBase):

  __slots__ = ('zero')

  def __init__(self, b):
    MemoryBase.__init__(self, b)
    self.zero = b.vUconstant(8, 0)

  def read(self, address):
    #print 'zero'
    return self.zero

class MemsetMem(MemoryBase):

  __slots__ = ('parent', 'base','top','value')

  def __init__(self, parent, base, length, value):
    MemoryBase.__init__(self, parent.b)
    self.parent = parent
    self.base = base
    self.top = self.b.vAdd0(base, length)
    self.value = value

  def read(self, address):
    inbounds = self.b.newAnd(self.b.vUlte(self.base, address), self.b.vUlt(address, self.top))
    st = self.b.checkStatus(inbounds)
    if st == bf.STATUS_MUST_BE_TRUE:
      return self.value
    elif st == bf.STATUS_MUST_BE_FALSE:
      return self.parent.read(address)
    else:
      return self.b.vSelect(inbounds, self.value, self.parent.read(address))

class MemcpyMem(MemoryBase):

  __slots__ = ('parent','src','dest','top')

  def __init__(self, parent, src, dest, length):
    MemoryBase.__init__(self, parent.b)
    self.parent = parent
    self.src = src
    self.dest = dest
    self.top = self.b.vAdd0(dest, length)

  def read(self, address):
    return self.readWithSolver(address)
    #print 'read memcpy',
    inbounds = self.b.newAnd(self.b.vUlte(self.dest, address), self.b.vUlt(address, self.top))
    if self.b.lIsFalse(inbounds):
      #print 'out of bounds'
      return self.parent.read(address)
    off = self.b.vSubtract(address, self.dest)
    sa = self.b.vAdd0(self.src, off)
    #print 'reading source'
    sv = self.parent.read(sa)
    if self.b.lIsTrue(inbounds):
      return sv
    else:
      #print 'reading other'
      return self.b.vSelect(inbounds, sv, self.parent.read(address))

  def readWithSolver(self, address):
    #print 'read memcpy',
    inbounds = self.b.newAnd(self.b.vUlte(self.dest, address), self.b.vUlt(address, self.top))
    stat = self.b.checkStatus(inbounds)
    if stat == bf.STATUS_MUST_BE_FALSE:
      #print 'out of bounds'
      return self.parent.read(address)
    off = self.b.vSubtract(address, self.dest)
    sa = self.b.vAdd0(self.src, off)
    #print 'reading source'
    sv = self.parent.read(sa)
    if stat == bf.STATUS_MUST_BE_TRUE:
      return sv
    elif stat == bf.STATUS_TRUE_OR_FALSE:
      #print 'reading other'
      return self.b.vSelect(inbounds, sv, self.parent.read(address))
    else:
      assert not 'reached'
