#!/usr/bin/env python2.7

# Copyright 2012 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
# on behalf of the U.S. Government. Export of this program may require a license
# from the United States Government.

import unittest
import random
import math
import sys

from bitfunc import *

class IsSymbolicTest(unittest.TestCase):
  def setUp(self):
    self.bf = bfman()

  def testIsSymbolic(self):
    x = self.bf.vUconstant(32, 0)
    self.assertEqual(x.isSymbolic(), False)
    self.assertEqual(x.isConcrete(), True)
    self.assertEqual(self.bf.vUconstant(32, 32).isSymbolic(), False)
    self.assertEqual(self.bf.vUconstant(32, 32).isConcrete(), True)
    self.assertEqual(self.bf.vInit(32).isSymbolic(), True)
    self.assertEqual(self.bf.vInit(32).isConcrete(), False)
    x = self.bf.vUconstant(32, 0)
    x[3] = self.bf.newVar()
    self.assertEqual(x.isSymbolic(), True)
    self.assertEqual(x.isConcrete(), False)
    # self.assertTrue(False)

class MitreTest(unittest.TestCase):
    def _runMitre(self, output):
        if self.m.lIsFalse(-output):
            return True
        if self.m.lIsTrue(-output):
            return False
        self.m.pushAssumption(-output)
        result = self.m.solve()
        if result == BF_SAT:
            return False
        else:
            if result == BF_UNSAT:
                return True
            else:
                print("Error")
                return False

class TestBitfuncFunctions32(MitreTest):
    """Base class for testing bitfunc.  This class can be subclassed, and the
    size changed to specify other tests.  For instance, some tests only are
    done on smaller sizes because of performance issues.
    """
    size = 32
    def setUp(self):
        """Called before every unittest."""
        self.m = bfman()
        self.m.configurePicosat()

    def tearDown(self):
        """Called after every unittest."""
        pass

    def test_iff(self):
        a = self.m.newVar()
        b = self.m.newVar()
        self.assertTrue(self._runMitre(self.m.newEqual(
                            -self.m.newXor(a,b),
                            self.m.newAnd(self.m.newImplies(b,a), self.m.newImplies(a,b)))))
                
    def test_const_prop(self):
        one = self.m.vUconstant(self.size, 1)
        one1 = self.m.vUconstant(self.size, 1)
        zero = self.m.vUconstant(self.size, 0)
        trueLit = self.m.vEqual(one, one1)
        falseLit = self.m.vEqual(one, zero)
        self.assertTrue(self.m.lIsTrue(trueLit) & self.m.lIsFalse(falseLit))

    def test_constants(self):
        k = random.randint(0, 2**self.size-1)
        v = random.randint(0, 2**self.size-1-(2**(self.size-1)))
        #v = 0xfffffffffffffff8
        u = self.m.vUconstant(self.size, k)
        u1 = self.m.vUconstant(self.size, k)
        s = self.m.vSconstant(self.size, v)
        s1 = self.m.vSconstant(self.size, v)

        output = bool2lit(True)
        output = self.m.newAnd(output, bool2lit(self.m.vGet(u) == k))
        output = self.m.newAnd(output, bool2lit(self.m.vGet(s) == v))
        self.assertTrue(litIsConst(output))

        a = self.m.vAlloc(3)
        a.vPush(output)
        a.vPush(self.m.vEqual(u, u1))
        a.vPush(self.m.vEqual(s, s1))
        self.assertTrue(self._runMitre(self.m.bigAnd(a)))

    def test_extensions(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        zero = self.m.vUconstant(self.size, 0)

        output = self.m.newSelect(self.m.vSlt(x, zero),
                                  self.m.vSlt(self.m.vSextend(self.size*2,x),
                                              self.m.vZextend(self.size*2,zero)),
                                  bool2lit(True));
        output = self.m.newAnd(output, self.m.newSelect(self.m.vSlt(zero, x),
                                                        self.m.vSlt(self.m.vZextend(self.size*2, zero),
                                                                    self.m.vSextend(self.size*2, x)),
                                                        bool2lit(True)));
        self.assertTrue(self._runMitre(output))

    def test_add_0_is_identity(self):
        x = self.m.vInit(self.size)
        zero = self.m.vUconstant(self.size, 0)
        xy = self.m.vAdd0(x,zero)
        self.assertTrue(self._runMitre(self.m.vEqual(xy,x)))

    def test_bigAnd(self):
        x = self.m.vInit(self.size)
        naive = bool2lit(True)
        for i in x:
            naive = self.m.newAnd(naive, i)
        actual = self.m.bigAnd(x)
        self.assertTrue(self._runMitre(self.m.newEqual(naive, actual)))

    def test_bigOr(self):
        x = self.m.vInit(self.size)
        naive = bool2lit(False)
        for i in x:
            naive = self.m.newOr(naive, i)
        actual = self.m.bigOr(x)
        self.assertTrue(self._runMitre(self.m.newEqual(naive, actual)))

    def test_check_status_f(self):
        x = self.m.newVar()
        y = self.m.newVar()
        self.m.setEqual(x,y)
        xr = self.m.newXor(x,y)
        status = self.m.checkStatus(xr)
        self.assertTrue(status == STATUS_MUST_BE_FALSE)

    def test_check_status_t(self):
        x = self.m.newVar()
        y = self.m.newVar()
        self.m.setEqual(x,y)
        xr = -self.m.newXor(x,y)
        status = self.m.checkStatus(xr)
        self.assertTrue(status == STATUS_MUST_BE_TRUE)

    def test_check_status_tf(self):
        x = self.m.newVar()
        y = self.m.newVar()
        self.m.setEqual(x,y)
        xr = self.m.newAnd(x,y)
        status = self.m.checkStatus(xr)
        self.assertTrue(status == STATUS_TRUE_OR_FALSE)

    def test_add_commutes(self):
        """Verify that add commutes"""
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        cin = self.m.newVar()

        [xy,c_xy] = self.m.vAdd(x,y,cin)
        [yx,c_yx] = self.m.vAdd(y,x,cin)
        
        eqbv = self.m.vEqual(xy,yx)
        ca = self.m.newEqual(c_xy,c_yx)
        
        output = self.m.newAnd(eqbv,ca)
        
        self.assertTrue(self._runMitre(output))

    def test_x_minus_x(self):
        x = self.m.vInit(self.size)
        diff = self.m.vSubtract(x, x)
        self.assertTrue(self._runMitre(self.m.vEqual(diff, self.m.vUconstant(self.size, 0))))

    def test_add_sub_inverse(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        diff = self.m.vSubtract(x,y)
        s = self.m.vAdd0(x, self.m.vNegate(y))
        self.assertTrue(self._runMitre(self.m.vEqual(diff, s)))

    def test_negate_is_invert_plus_1(self):
        x = self.m.vInit(self.size)
        s = self.m.vAdd0(self.m.vInvert(x), self.m.vUconstant(self.size, 1))
        neg = self.m.vNegate(x)
        self.assertTrue(self._runMitre(self.m.vEqual(s, neg)))

    def test_lt_max(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        for i in y:
            self.m.pushAssumption(i)
        self.assertTrue(self._runMitre(self.m.newXor(self.m.vEqual(x,y), self.m.vUlt(x,y))))

    def test_slt_max(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        for i in range(len(y)-1):
            self.m.pushAssumption(y[i])
        self.m.pushAssumption(-(y[len(y)-1])) #NOTE: y[-1] does not work as expected
        self.assertTrue(self._runMitre(self.m.newXor(self.m.vEqual(x,y), self.m.vSlt(x,y))))

    def test_lt_succ(self):
        x = self.m.vInit(self.size)
        uint_max = self.m.vInit(self.size)
        for i in uint_max:
            self.m.pushAssumption(i)
        one = self.m.vUconstant(self.size, 1)
        succ = self.m.vAdd0(x, one)
        self.assertTrue(self._runMitre(self.m.newImplies(-self.m.vEqual(x, uint_max), self.m.vUlt(x, succ))))

    def test_slt_succ(self):
        x = self.m.vInit(self.size)
        int_max = self.m.vInit(self.size)
        for i in range(len(int_max)-1):
            self.m.pushAssumption(int_max[i])
        self.m.pushAssumption(-(int_max[len(int_max)-1]))
        one = self.m.vUconstant(self.size, 1)
        succ = self.m.vAdd0(x, one)
        self.assertTrue(self._runMitre(self.m.newImplies(-self.m.vEqual(x, int_max), self.m.vSlt(x, succ))))

    def test_lte(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        x_le_y = self.m.vUlte(x,y)
        rhs = self.m.newOr(self.m.vUlt(x,y), self.m.vEqual(x,y))
        self.assertTrue(self._runMitre(self.m.newIff(x_le_y, rhs)))

    def test_lte_19a(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        (s, cout) = self.m.vAdd(y, self.m.vInvert(x), bool2lit(True))
        self.assertTrue(self._runMitre(self.m.newIff(self.m.newOr(self.m.vUlt(x,y), self.m.vEqual(x,y)), cout)))

    def test_less_19a(self):
        """Hacker's Delight, page 24"""
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        ny = self.m.vInvert(y)
        (s, cout) = self.m.vAdd(x, ny, bool2lit(True))
        self.assertTrue(self._runMitre(self.m.newIff(self.m.vUlt(x,y), -cout)))

    def test_sless_19a(self):
        """Hacker's Delight, page 23"""
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        lt = self.m.vSlt(x,y)
        forXor = self.m.vAlloc(self.size)
        forXor.vPush(self.m.vUlt(x,y))
        forXor.vPush(x[len(x)-1])
        forXor.vPush(y[len(y)-1])
        _xor = self.m.bigXor(forXor)
        self.assertTrue(self._runMitre(self.m.newIff(lt,_xor)))

    def test_sless_19b(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        two_31 = self.m.vInit(self.size)
        self.m.pushAssumption(two_31[len(two_31)-1])
        for i in range(len(two_31)-1):
            self.m.pushAssumption(-two_31[i])

        lt = self.m.vSlt(x,y)
        s, cout = self.m.vAdd(self.m.vAdd0(x, two_31), self.m.vInvert(self.m.vAdd0(y, two_31)), bool2lit(True))
        self.assertTrue(self._runMitre(self.m.newIff(lt, -cout)))

    def test_lshift_for_realizes(self):
        base = int(math.log(self.size, 2))
        x = self.m.vInit(self.size)
        dist = self.m.vInit(base)

        bvWidth = self.m.vUconstant(self.size, len(x))
        ub = self.m.vSubtract(bvWidth, self.m.vZextend(self.size, dist))

        lshifted = self.m.vLshift(x,dist)
        rshifted = self.m.vRshift(lshifted, dist, bool2lit(False))

        #test left shifted fill bits
        out = bool2lit(True)
        for i in range(self.size):
            index_i = self.m.vUconstant(self.size, i)
            out = self.m.newAnd(out, self.m.newImplies(self.m.vUlt(index_i, self.m.vZextend(self.size, dist)), self.m.newEqual(lshifted[i], bool2lit(False))))

        #test bits that were shifted back to their orig. positions
        for i in range(self.size):
            index_i = self.m.vUconstant(self.size, i)
            out = self.m.newAnd(out, self.m.newImplies(self.m.vUlt(index_i, ub), self.m.newEqual(x[i], rshifted[i])))

        self.assertTrue(self._runMitre(out))
    
    def test_lshift_const(self):
        x = self.m.vInit(self.size)
        output = bool2lit(True)
        for dist in range(int(2**(math.log(self.size, 2))) + 1):
            shifted = self.m.vLshiftConst(x, dist)
            for i in range(len(x)):
                if i < dist:
                    output = self.m.newAnd(output, self.m.newEqual(bool2lit(False), shifted[i]))
                else:
                    output = self.m.newAnd(output, self.m.newEqual(x[i-dist], shifted[i]))
        self.assertTrue(self._runMitre(output))

    def test_rshift_for_realzies(self):
        base = int(math.log(self.size, 2))
        x = self.m.vInit(self.size)
        dist = self.m.vInit(base)

        bvWidth = self.m.vUconstant(self.size, len(x))
        fill = self.m.newVar()

        rshifted = self.m.vRshift(x, dist, fill)
        lshifted = self.m.vLshift(rshifted,dist)

        #test right shifted fill bits
        out = bool2lit(True)
        for j in range(self.size, 0, -1):
            i = j-1
            index_i = self.m.vUconstant(self.size, i)
            out = self.m.newAnd(out, self.m.newImplies(
                                                self.m.vUlt(self.m.vSubtract(bvWidth, self.m.vZextend(self.size, dist)), index_i),
                                                self.m.newEqual(rshifted[i], fill)))

        #test bits that were shifted back to their orig. positions
        for j in range(self.size, 0, -1):
            i = j-1
            index_i = self.m.vUconstant(self.size, i)
            out = self.m.newAnd(out, self.m.newSelect(
                                                self.m.newOr(
                                                     self.m.vEqual(self.m.vZextend(self.size, dist), index_i),
                                                     self.m.vUlt(self.m.vZextend(self.size, dist), index_i)),
                                                self.m.newEqual(lshifted[i], x[i]),
                                                self.m.newEqual(lshifted[i], bool2lit(False))))

        self.assertTrue(self._runMitre(out))

    def test_rshift_const(self):
        x = self.m.vInit(self.size)
        output = bool2lit(True)
        fill = self.m.newVar()
        for dist in range(int(2**(math.log(self.size, 2))) + 1):
            shifted = self.m.vRshiftConst(x, dist, fill)
            for i in range(len(x)):
                if i >= (self.size-dist):
                    output = self.m.newAnd(output, self.m.newEqual(fill, shifted[i]))
                else:
                    output = self.m.newAnd(output, self.m.newEqual(x[i+dist], shifted[i]))
        self.assertTrue(self._runMitre(output))

    def test_slte(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        two_31 = self.m.vInit(self.size)
        self.m.pushAssumption(two_31[len(two_31)-1])
        for i in range(len(two_31)):
            self.m.pushAssumption(-(two_31[i]))
        output = self.m.newIff(self.m.vSlte(x,y),
                               self.m.vUlte(self.m.vAdd0(x,two_31),
                                            self.m.vAdd0(y,two_31)))
        self.assertTrue(self._runMitre(output))

    def test_sltZero(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        self.assertTrue(self._runMitre(self.m.newIff(self.m.vSlt(x, self.m.vUconstant(self.size, 0)), self.m.vSltZero(x))))

    def test_sgreater_than_zero(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        self.assertTrue(self._runMitre(self.m.newIff(self.m.vSlt(self.m.vUconstant(self.size,0), x), self.m.vSgtZero(x))))

    def test_sgteZero(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        self.assertTrue(self._runMitre(self.m.newIff(self.m.newOr(self.m.vSlt(self.m.vUconstant(self.size,0), x),
                                                   self.m.vEqual(x, self.m.vUconstant(self.size,0))),
                                      self.m.vSgteZero(x))))

    def test_lselect(self):
        tst = self.m.newVar();
        thn = self.m.newVar();
        els = self.m.newVar();
        res = self.m.newSelect(tst, thn, els)
        self.assertTrue(self._runMitre(self.m.newAnd(self.m.newImplies(tst, self.m.newEqual(thn, res)),
                                      self.m.newImplies(-tst, self.m.newEqual(els, res)))))


    def test_concat(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        c = self.m.vConcat(x,y)
        output = bool2lit(True)
        for i in range(len(x)):
            output = self.m.newAnd(output, self.m.newEqual(c[i], x[i]))
        for i in range(len(x), len(c)):
            output = self.m.newAnd(output, self.m.newEqual(c[i], y[i - len(x)]))
        self.assertTrue(self._runMitre(output))

    def test_extract(self):
        x = self.m.vInit(self.size)
        e1 = self.m.vExtract(x, 0, self.size)
        output = bool2lit(len(e1) == self.size)
        for i in range(len(x)):
            output = self.m.newAnd(output, bool2lit(x[i] == e1[i]))
        e2 = self.m.vExtract(x, 1, self.size / 3)
        output = self.m.newAnd(output, bool2lit(len(e2) == self.size / 3))
        for i in range(len(e2)):
            output = self.m.newAnd(output, bool2lit(x[i+1] == e2[i]))
        self.assertTrue(self._runMitre(output))

    def test_incremental(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)

        x_gt_0 = self.m.vUlt(self.m.vUconstant(self.size, 0), x)
        x_lt_100 = self.m.vUlt(x, self.m.vUconstant(self.size, 100))
        y_lt_100 = self.m.vUlt(y, self.m.vUconstant(self.size, 100))

        x1 = self.m.vAdd0(x,y)
        x1_gt_0 = self.m.vUlt(self.m.vUconstant(self.size, 0), x1)

        self.m.pushAssumption(x_gt_0)
        self.m.pushAssumption(-x1_gt_0)
        if not(BF_SAT == self.m.solve()):
            self.fail()

        self.m.pushAssumption(x_lt_100)
        self.m.pushAssumption(y_lt_100)
        self.m.pushAssumption(x_gt_0)
        self.m.pushAssumption(-x1_gt_0)
        if not(BF_UNSAT == self.m.solve()):
            self.fail()

        x2 = self.m.vAdd0(x1, self.m.vUconstant(self.size, 1))
        x2_gt_0 = self.m.vUlt(self.m.vUconstant(self.size, 0), x2)
        self.m.pushAssumption(x_lt_100)
        self.m.pushAssumption(y_lt_100)
        self.m.pushAssumption(x_gt_0)
        self.m.pushAssumption(-x2_gt_0)
        if not(BF_UNSAT == self.m.solve()):
            self.fail()

    def test_xor_x_x_is_0(self):
        x = self.m.vInit(self.size)
        self.assertTrue(self._runMitre(self.m.vEqual(self.m.vXor(x,x), self.m.vUconstant(self.size,0))))

    def test_xor_x_y_x_is_y(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        self.assertTrue(self._runMitre(self.m.vEqual(y, self.m.vXor(x, self.m.vXor(y,x)))))

    def test_xor_assoc(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        z = self.m.vInit(self.size)
        self.assertTrue(self._runMitre(self.m.vEqual(self.m.vXor(x, self.m.vXor(y,z)),
                                      self.m.vXor(self.m.vXor(x,y), z))))

    def test_between(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        z = self.m.vInit(self.size)

        self.m.lSet(self.m.vEqual(x, self.m.vUconstant(self.size, 15)))
        self.m.lSet(self.m.vUlt(y, self.m.vUconstant(self.size, 17)))
        self.m.lSet(self.m.vUlt(x, y))

        answer = self.m.vEqual(y, self.m.vUconstant(self.size, 16))
        ret = 0
        if ret==0:
            ret = not(True == self.m.mayBeTrue(answer))
        if ret==0:
            ret = not(True == self.m.mustBeTrue(answer))
        if ret==0:
            ret = not(BF_SAT == self.m.solve())
        if ret==0:
            ret = not(16 == self.m.vGet(y))
        self.assertFalse(ret)

class TestBitfuncFunctions8(TestBitfuncFunctions32):
    size = 8
    def test_mul_commutes(self):
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        xy = self.m.vMul(x,y)
        yx = self.m.vMul(y,x)
        self.assertTrue(self._runMitre(self.m.vEqual(xy, yx)))

    def test_quotRem_mul(self):
        """q=(x/y), r=(x%y), q*y + r = x
        
        This test is too slow on 32-bit sizes"""
        x = self.m.vInit(self.size)
        y = self.m.vInit(self.size)
        (q,r) = self.m.vQuotRem(x,y)
        
        mul = self.m.vMul(q,y)
        sum = self.m.vAdd0(mul,r)
        
        output = self.m.vEqual(sum, x)
        
        self.assertTrue(self._runMitre(output))

    def test_incremental(self):
        """We don't run this test with 8-bit width"""
        pass

    def test_between(self):
        """We don't run this test with 8-bit width"""
        pass

class TestBitfuncFunctions1(TestBitfuncFunctions8):
    size = 1
    def test_incremental(self):
        """We don't run this test with 1-bit width"""
        pass

    def test_between(self):
        """We don't run this test with 8-bit width"""
        pass

class TestBitfuncFunctions64(TestBitfuncFunctions32):
    size = 64

class TestBitvecInterface(unittest.TestCase):
    def setUp(self):
        """Called before every unittest."""
        self.m = bfman(AIG_MODE)
        self.b = self.m.vInit(32)

    def test_bitvecLen(self):
        self.assertTrue(len(self.b) == 32)

    def test_bitvecGetLimit(self):
        self.assertRaises(IndexError, self.b.__getitem__, len(self.b) + 1)

    def notest_bitvecSet(self):
        self.assertRaises(NotImplementedError, self.b.__setitem__, 0, 0)

class TestMemoryInterface(unittest.TestCase):
    def setUp(self):
        self.m = bfman()
        self.size = 8
    
    def test_mIdxSize(self):
        idxSize = random.randint(0, 32)
        mem = self.m.mInit(idxSize, self.size)
        self.assertTrue(idxSize == mem.idxSize())

    def test_mEltSize(self):
        eltSize = random.randint(0, 32)
        mem = self.m.mInit(self.size, eltSize)
        self.assertTrue(eltSize == mem.eltSize())

    # This doesn't work, should it? 
	'''
	def test_memory_load(self):
        mem = self.m.mInit(self.size, self.size)
        addr = self.m.vUconstant(self.size, 42)
        val1 = self.m.vUconstant(self.size, 11)
        val2 = mem.mLoad(addr)
        fin = self.m.vEqual(val1, val2)
        self.m.pushAssumption(fin)
        self.m.solve()
        print "(%s ==? %s)" % (self.m.vGet(val1),self.m.vGet(val2))
        self.assertTrue(self.m.vGet(val2)==self.m.vGet(val1))
	'''

    def test_memory(self):
        fortytwo = self.m.vUconstant(self.size, 42)
        sym1 = self.m.vInit(self.size)
        sym2 = self.m.vAdd0(sym1, sym1)

        mem = self.m.mInit(self.size, self.size)
        mem.store(fortytwo, fortytwo)
        out = mem.load(fortytwo)
        # print out
        #self.m.vPrintConcrete(sys.stderr, out)

        mem.store(sym1, fortytwo)
        out = mem.load(fortytwo)
        # print out

        #mem.mStore_be(sym1,fortytwo)
        #out = mem.mLoad_be(fortytwo,self.size)
        #print out

        #mem.mStore_le(sym1,fortytwo)
        #out = mem.mLoad_le(fortytwo,self.size)
        #print out

        #self.m.vPrintConcrete(sys.stdout, out)
        #mem2 = mem.mCopy()

        #em2.mStore(sym1, sym2)
        #ut = mem2.mLoad(fortytwo)
        #print out
        #self.m.vPrint(sys.stdout, out)

if __name__ == '__main__':
    unittest.main()

# End:
