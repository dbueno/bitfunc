import bitfunc
import cPickle as pickle

b = bitfunc.bfman()

x = b.vInit(8)
y = b.vInit(8)
z = b.vMul(x,y)

b.pushAssumption(b.vEqual(z, b.vUconstant(8, 12)))

b1,x1,y1,z1 = pickle.loads(pickle.dumps((b,x,y,z)))

print b.pushAssumption(b.vEqual(b.vUconstant(8, 2), x))
print b.solve()

print b.vGet(y)
assert b.vGet(y) == 6

print b1.pushAssumption(b1.vEqual(b1.vUconstant(8, 3), x1))
print b1.solve()

print b1.vGet(y1)
assert b1.vGet(y1) == 4
