from bitfunc.expdag import *
from bitfunc import *
import bitfunc
import random


class UnsatError(Exception):
  pass

BITS = 800
rand = random.Random()
uconst = lambda x: ConstExp(x, width=BITS)

print 'using %d bits' % BITS

BF = bitfunc.bfman()
BF.configurePicosatIncremental()
bitfunc.expdag.BF = BF

x_v = BF.vInit(BITS)
y_v = BF.vInit(BITS)
x = InputExp(BITS)
y = InputExp(BITS)
input_map = {x: x_v, y: y_v}

z = MulExp(x, y)

expected_bv = bitvec.alloc(BITS)
for i in xrange(BITS):
  expected_bv.vPush(literal.fromBool(random.randint(0,1)))
expected = ConstExp(expected_bv.uint, width=BITS)

print 'expected: %s' % str(expected)

# assumptions
eq_expected = EqExp(z, expected)
x_neq_1 = NEExp(x, uconst(1))
y_neq_1 = NEExp(y, uconst(1))

#with open('factoring.dot', 'w') as fd:
#  as_dot_nodes(dict([(eq_expected, 'expected')]), fd)

if BF_UNSAT == BF.pushAssumption(eq_expected.as_bitvec(input_map)[0]):
  raise UnsatError
if BF_UNSAT == BF.pushAssumption(x_neq_1.as_bitvec(input_map)[0]):
  raise UnsatError
if BF_UNSAT == BF.pushAssumption(y_neq_1.as_bitvec(input_map)[0]):
  raise UnsatError

print 'solving...'
res = BF.solve()
print res
print 'x_v = %s' % str(BF.vFromCounterExample(x_v))
print 'y_v = %s' % str(BF.vFromCounterExample(y_v))


