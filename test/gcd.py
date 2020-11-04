import bitfunc
from bitfunc import bitvec
import ipdb

bf = bitfunc.bfman()
bf.configurePicosat()

NUM_BITS = 6
a = bf.vInit(NUM_BITS) # create 8 bit unknown value
b = bf.vInit(NUM_BITS)

i = 0
while True:
    """if b == 0 stop
    function gcd(a, b)
    while b != 0
       t := b
       b := a mod b
       a := t
    return a
    """
    r = bf.pushAssumption(b != 0)
    bf.printCNF('/tmp/gcd%02d.cnf' % i)
    if r == bitfunc.BF_UNSAT:
        break
    r = bf.solve()
    if r == bitfunc.BF_UNSAT:
        break
    print 'a',bf.vFromCounterExample(a)
    print 'b',bf.vFromCounterExample(b)
    a, b = b, a % b

    i += 1

