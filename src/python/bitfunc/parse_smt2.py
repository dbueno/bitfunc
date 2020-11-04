# Copyright 2012 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
# on behalf of the U.S. Government. Export of this program may require a license
# from the United States Government.

import bitfunc.sexp_parser as sexp
import bitfunc.expdag as exp
from sys import stderr
from copy import copy

def car(l):
  return l[0]
def cdr(l):
  return l[1:]
def cddr(l):
  return l[2:]
def cdddr(l):
  return l[3:]
def cadr(l):
  return car(cdr(l))
def caddr(l):
  return car(cddr(l))

class SMT2File(object):
  def __init__(self):
    self.funs = {}
    self.asserts = []

  def __str__(self):
    return 'SMT2File:\n' +\
        '  funs: %s\n' % str(self.funs) +\
        '  asserts: %s\n' % str(self.asserts)

def toplevel_form(form, result):
  if car(form)=='set-logic':
    result.logic = cadr(form)
  elif car(form)=='set-info':
    pass                        # skip
  elif car(form)=='declare-fun':
    name = cadr(form)
    value = exptype(car(cdddr(form)), result)
    result.funs[name] = value
  elif car(form)=='assert':
    exp = expvalue(cadr(form), result.funs)
    result.asserts.append(exp)
  elif car(form)=='check-sat':
    print >>stderr, 'check-sat'
  elif car(form)=='exit':
    pass

def exptype(form, result):
  if car(form)=='_' and cadr(form)=='BitVec':
    return exp.InputExp(caddr(form))

binops = {
  'bvmul': exp.MulExp,
  'bvadd': exp.AddExp,
  'bvsub': exp.SubExp,
  'bvsle': exp.SLEExp,
  '=>':    exp.ImpliesExp,
  '=':     exp.EqExp,
  }

unops = {
  'bvneg': exp.NegateExp,
  'not':   exp.NegateExp,
  }

assoc_comm_ops = {
  'and': exp.AndExp,
  'or':  exp.OrExp,
  }

# - Bitvector Constants:
#     (_ bvX n) where X and n are numerals, i.e. (_ bv13 32),
#     abbreviates the term #bY of sort (_ BitVec n) such that

#     [[#bY]] = nat2bv[n](X) for X=0, ..., 2^n - 1.

#     See the specification of the theory's semantics for a definition
#     of the functions [[_]] and nat2bv.  Note that this convention implicitly
#     considers the numeral X as a number written in base 10.

def expvalue(form, env):
  """retrieve the exp value for the given form

  the environment allows us to resolve expressions that depend on variable
  definitions"""
  binop = binops.get(car(form))
  unop  = unops.get(car(form))
  acop  = assoc_comm_ops.get(car(form))
  if car(form)=='let':
    bindlist = cadr(form)
    body = caddr(form)
    innerenv = copy(env)
    for bind in bindlist:
      assert env.get(car(bind)) is None
      innerenv[car(bind)] = expvalue(cadr(bind), env)
    print >>stderr, 'env: %s' % str(innerenv)
    return expvalue(body, innerenv)

  elif binop is not None:
    assert len(form)==3
    return binop(expvalue(cadr(form), env), expvalue(caddr(form), env))

  elif unop is not None:
    assert len(form)==2
    return unop(expvalue(cadr(form), env))

  elif acop is not None:
    o = None
    for t in cdr(form):
      if o is None:
        o = expvalue(t, env)
      else:
        o = acop(o, expvalue(t, env))
    return o

  elif car(form)=='_' and cadr(form)[0:2] == 'bv':
    # special bitvec constant
    num = int(cadr(form)[2:])
    sz = caddr(form)
    return exp.ConstExp(num, sz)

  else:
    # identifier
    if not (isinstance(form, str) or isinstance(form, unicode)):
      print >>stderr, 'unhandled: (%s) %s' % (type(form), str(form))
      assert False
    return env[form]

def parseString(f):
  print >>stderr, 'parseString in smt2'
  x=sexp.sexpSequence.parseString(f)
  print >>stderr, str(x)
  r = SMT2File()
  for f in x:
    toplevel_form(f,r)
  return r
