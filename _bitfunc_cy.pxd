# Copyright 2012 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
# on behalf of the U.S. Government. Export of this program may require a license
# from the United States Government.

from libc.stdio cimport FILE, stdout
from libc.stdint cimport *

cdef extern from "stdbool.h":
  ctypedef bint bool

cdef extern from "bitfunc/aiger.h":

  ctypedef unsigned aiger_literal
  ctypedef unsigned aiger_variable
  ctypedef unsigned aiger_size

  ctypedef struct aiger_and:
    aiger_literal lhs
    aiger_literal rhs0
    aiger_literal rhs1

  ctypedef struct aiger_symbol:
    aiger_literal lit
    char *name

  ctypedef struct aiger:
    aiger_variable maxvar
    aiger_size num_inputs
    aiger_size num_latches
    aiger_size num_outputs
    aiger_size num_ands

    aiger_symbol *inputs
    aiger_symbol *latches
    aiger_symbol *outputs
    aiger_and *ands

  void aiger_add_input (aiger *aig, aiger_literal lit, char *name)
  void aiger_add_latch (aiger *aig, aiger_literal lit, aiger_literal next, char *name)
  void aiger_add_output (aiger *aig, aiger_literal lit, char *name)
  void aiger_add_and (aiger *aig, aiger_literal lhs, aiger_literal rhs0, aiger_literal rhs1)

cdef extern from "bitfunc.h":

  # funcsat
  ctypedef uint8_t mbool
  ctypedef int literal
  ctypedef unsigned variable
  cdef enum: LITERAL_MAX
  cdef enum: VARIABLE_MAX
  cdef enum: unknown

  ctypedef struct vector:
    pass

  # bitfunc/types.h
  cdef enum: LITERAL_TRUE
  cdef enum: LITERAL_FALSE

  ctypedef enum bfresult:
    BF_UNKNOWN
    BF_SAT
    BF_UNSAT

  ctypedef enum bfstatus:
    STATUS_MUST_BE_TRUE
    STATUS_MUST_BE_FALSE
    STATUS_TRUE_OR_FALSE
    STATUS_NOT_TRUE_NOR_FALSE
    STATUS_UNKNOWN

  ctypedef enum extendty:
    EXTEND_SIGN
    EXTEND_ZERO

  ctypedef enum bfmode:
    AIG_MODE

  struct and_key:
    pass

  ctypedef struct bitvec:
    uint32_t size
    literal *data

  ctypedef struct bfman:
    unsigned numVars
    bitvec *assumps
    aiger *aig

  # bitfunc/bitvec.h
  bitvec *bfvInit(bfman *m, uint32_t width)
  bitvec *bfvAlloc(uint32_t initialCapacity)
  void bfvPush(bitvec *v, literal data)
  literal bfvPop(bitvec *v)
  void bfvGrowTo(bitvec *v, uint32_t newCapacity)
  void bfvClear(bitvec *v)
  void bfvDestroy(bitvec *v)
  bitvec *bfvHold(bitvec *v)
  bitvec *bfvRelease(bitvec *v)
  void CONSUME_BITVEC(bitvec *v)

  ctypedef enum bfvprint_mode:
    PRINT_BINARY
    PRINT_HEX

  void bfvPrint(FILE *stream, bitvec *b)
  void bflPrint(FILE *stream, literal l)
  void bfvPrintConcrete(bfman *m, FILE *stream, bitvec *b, bfvprint_mode mode)
  mbool bfvSame(bitvec *b1, bitvec *b2)
  bool bfvIsConstant(bitvec *x)
  bitvec *bfvZero(uint32_t sz)
  bitvec *bfvOnes(uint32_t sz)
  bitvec *bfvOne(uint32_t sz)
  bitvec *bfvSetName(bfman *b, bitvec *x, char *name)
  bitvec *bfvAnd(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvOr(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvXor(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvNegate(bfman *m, bitvec *x)
  bitvec *bfvAbsoluteValue(bfman *m, bitvec *x)
  bitvec *bfvInvert(bfman *m, bitvec *x)
  bitvec *bfvSelect(bfman *m, literal t, bitvec *thn, bitvec *els)
  bitvec *bfvZextend(bfman *m, uint32_t bfNew_width, bitvec *v)
  bitvec *bfvSextend(bfman *m, uint32_t bfNew_width, bitvec *v)
  bitvec *bfvExtend(bfman *m, uint32_t bfNew_width, bitvec *v, extendty ty)
  bitvec *bfvTruncate(bfman *m, uint32_t bfNewWidth, bitvec *v)
  bitvec *bfvAdd(bfman *m, bitvec *x, bitvec *y, literal cin, literal *cout)
  bitvec *bfvAdd0(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvSubtract(bfman *m, bitvec *x, bitvec *y)
  literal bfvUlt(bfman *m, bitvec *x, bitvec *y)
  literal bfvUlte(bfman *m, bitvec *x, bitvec *y)
  literal bfvSlt(bfman *m, bitvec *x, bitvec *y)
  literal bfvSlte(bfman *m, bitvec *x, bitvec *y)
  literal bfvSltZero(bfman *m, bitvec *x)
  literal bfvSgtZero(bfman *m, bitvec *x)
  literal bfvSgteZero(bfman *m, bitvec *x)
  literal bfvEqual(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvSconstant(bfman *m, uint32_t width, int value)
  bitvec *bfvUconstant(bfman *m, uint32_t width, unsigned value)
  bitvec *bfvLshift(bfman *m, bitvec *val, bitvec *dist)
  bitvec *bfvLshiftConst(bfman *m, bitvec *val, uint32_t dist)
  bitvec *bfvRshiftConst(bfman *m, bitvec *val, uint32_t dist, literal fill)
  bitvec *bfvRshift(bfman *m, bitvec *val, bitvec *dist, literal fill)
  bitvec *bfvLRshift(bfman *m, bitvec *val, bitvec *dist)
  bitvec *bfvARshift(bfman *m, bitvec *val, bitvec *dist)
  bitvec *bfvMul(bfman *m, bitvec *x, bitvec *y)
  void bfvQuotRem(bfman *m, bitvec *x, bitvec *y, bitvec **quot, bitvec **rem)
  bitvec *bfvSdiv(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvSrem(bfman *b, bitvec *x, bitvec *y)
  bitvec *bfvQuot(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvDiv(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvRem(bfman *m, bitvec *x, bitvec *y)
  void bfvCopy(bfman *m, bitvec *dst, bitvec *src)
  bitvec *bfvDup(bfman *m, bitvec *src)
  bitvec *bfvConcat(bfman *m, bitvec *x, bitvec *y)
  bitvec *bfvConcat3(bfman *m, bitvec *x, bitvec *y, bitvec *z)
  bitvec *bfvExtract(bfman *m, bitvec *x, uint32_t begin, uint32_t len)
  bitvec *bfvReverse(bfman *b, bitvec *x)
  bitvec *bfvIncr(bfman *b, bitvec *x)

  # bitfunc/mem.h
  ctypedef struct memory:
    pass

  unsigned bfmIdxSize(memory *m)
  unsigned bfmEltSize(memory *m)
  memory *bfmInit(bfman *b, uint8_t idxsize, uint8_t eltsize)
  void bfmStore(bfman *b, memory *m, bitvec *addr, bitvec *val)
  void bfmStore_le(bfman *b, memory *m, bitvec *address_start, bitvec *val)
  void bfmStore_be(bfman *b, memory *m, bitvec *address_start, bitvec *val)
  void bfmStoreVector_le(bfman *b, memory *m, bitvec *address_start, vector *values)
  void bfmStoreVector_be(bfman *b, memory *m, bitvec *address_start, vector *values)
  bitvec *bfmLoad(bfman *b, memory *m, bitvec *addr)
  bitvec *bfmLoad_RBW(bfman *b, memory *m, bitvec *addr, literal *rbw)
  bitvec *bfmLoad_le(bfman *b, memory *m, bitvec *address_start, unsigned numElts)
  bitvec *bfmLoad_be(bfman *b, memory *m, bitvec *address_start, unsigned numElts)
  vector *bfmLoadVector_le(bfman *b, memory *m, bitvec *address_start, unsigned numVecs, unsigned vecSize)
  vector *bfmLoadVector_be(bfman *b, memory *m, bitvec *address_start, unsigned numVecs, unsigned vecSize)
  memory *bfmCopy(bfman *b, memory *m)
  memory *bfmSelect(bfman *b, literal c, memory *t, memory *f)
  memory *bfmFromCounterExample(bfman *b, memory *m)
  void bfmPrint(bfman *b, FILE *out, memory *m)
  void bfmDestroy(bfman *b, memory *m)
  void bfmUnsafeFree(memory *m)

  # bitfunc.h
  bfman *bfInit(bfmode mode)
  bfresult bfPushAssumption(bfman *m, literal p)
  bfresult bfvPushAssumption(bfman *m, bitvec *p)
  void bfPopAssumptions(bfman *m, unsigned num)
  bfresult bfSolve(bfman *m)
  void bfConfigureFuncsat(bfman *m)
  void bfConfigurePicosat(bfman *m)
  void bfConfigurePicosatIncremental(bfman *m)
  void bfConfigureLingeling(bfman *m)
  void bfConfigurePlingeling(bfman *m)
  void bfConfigurePrecosat(bfman *m)
  void bfConfigurePicosatReduce(bfman *m)
  void bfConfigureLingelingReduce(bfman *m)
  void bfConfigureExternal(bfman *m)
  void bfReset(bfman *m)
  void bfDestroy(bfman *m)
  void bfResetFuncsat(bfman *m)
  unsigned bfNumVars(bfman *m)
  unsigned bfNumClauses(bfman *m)
  void bfSetNumVars(bfman *m, unsigned numVars)
  void bfSet(bfman *m, literal a)
  void bfSetClause(bfman *b, bitvec *c)
  bool bfGet(bfman *m, literal a)
  uint64_t bfvGet(bfman *m, bitvec *bv)
  literal bflFromCounterExample(bfman *b, literal l)
  bitvec *bfvFromCounterExample(bfman *b, bitvec *bv)
  memory *bfmFromCounterExample(bfman *b, memory *m)
  literal bfAssignmentVar2Lit(bfman *b, variable v)
  bfstatus bfCheckStatus(bfman *m, literal a)
  mbool bfMayBeTrue(bfman *m, literal x)
  mbool bfMayBeFalse(bfman *m, literal x)
  mbool bfMustBeTrue(bfman *m, literal x)
  mbool bfMustBeFalse(bfman *m, literal x)
  mbool bfMustBeTrue_picosat(bfman *man, literal x)
  mbool bfMustBeTrue_picosatIncremental(bfman *man, literal x)
  void bfPrintAIG(bfman *b, FILE *out)
  void bfPrintCNF(bfman *b, char *cnf_file)
  bool bfEnableDebug(bfman *m, char *label, int maxlevel)
  bool bfDisableDebug(bfman *m, char *label)
  void bfPrintStatus(bfman *m, FILE *)
  void bfFindCore_picomus(bfman *m, char *core_output_file)
  void bfWriteAigerToFile(bfman *b, char *path)

cdef extern from "bitfunc/circuits.h":
  literal bfNewVar(bfman *m)
  literal bfSetName(bfman *b, literal l, char *name)
  char *bfGetName(bfman *b, literal l)
  literal bfNewAnd(bfman *m, literal a, literal b)
  literal bfNewOr(bfman *m, literal a, literal b)
  literal bfNewXor(bfman *m, literal a, literal b)
  literal bfNewNand(bfman *m, literal a, literal b)
  literal bfNewNor(bfman *m, literal a, literal b)
  literal bfNewEqual(bfman *m, literal a, literal b)
  literal bfNewImplies(bfman *m, literal a, literal b)
  literal bfNewIff(bfman *m, literal a, literal b)
  literal bfAnd(bfman *m, literal a, literal b, literal o)
  literal bfOr(bfman *m, literal a, literal b, literal o)
  literal bfXor(bfman *m, literal a, literal b, literal o)
  literal bfNand(bfman *m, literal a, literal b, literal o)
  literal bfNor(bfman *m, literal a, literal b, literal o)
  literal bfImplies(bfman *m, literal a, literal b, literal o)
  literal bfEqual(bfman *m, literal a, literal b, literal o)
  literal bfBigAnd(bfman *m, bitvec *bv)
  literal bfBigOr(bfman *m, bitvec *bv)
  literal bfBigXor(bfman *m, bitvec *bv)
  literal bfNewSelect(bfman *m, literal a, literal b, literal c)
  void bfSetEqual(bfman *m, literal a, literal b)
  # unsigned addLitConeOfInfluence(bfman *m, aiger_literal l, FILE *out)
  void printAiger(bfman *, char *, bitvec *)
  void printAiger_allinputs(bfman *m)
  void printFalseAig(bfman *m)

cdef extern from "bitfunc/pcode_definitions.h":
  bitvec *pCOPY(bfman *b, bitvec *input0)
  bitvec *pINT_ADD(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_SUB(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_MULT(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_DIV(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_REM(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_SDIV(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_SREM(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_OR(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_XOR(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_AND(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_LEFT(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_RIGHT(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_SRIGHT(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_EQUAL(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_NOTEQUAL(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_LESS(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_LESSEQUAL(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_CARRY(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_SLESS(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_SLESSEQUAL(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_SCARRY(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pINT_SBORROW(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pBOOL_OR(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pBOOL_XOR(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pBOOL_AND(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pBOOL_NEGATE(bfman *b, bitvec *input0)
  bitvec *pPIECE(bfman *b, bitvec *input0, bitvec *input1)
  bitvec *pSUBPIECE(bfman *b, bitvec *input0, uint32_t input1, uint32_t output_size)
  bitvec *pINT_ZEXT(bfman *b, bitvec *input0, uint32_t output_size)
  bitvec *pINT_SEXT(bfman *b, bitvec *input0, uint32_t output_size)
  bitvec *pINT_2COMP(bfman *b, bitvec *input0)
  bitvec *pINT_NEGATE(bfman *b, bitvec *input0)

cdef extern from "bitfunc/mem.h":
  ctypedef struct memory:
    pass
  unsigned bfmIdxSize(memory *m)
  unsigned bfmEltSize(memory *m)
  memory *bfmInit(bfman *b, uint8_t idxsize, uint8_t eltsize)

  void bfmStore(bfman *b, memory *m, bitvec *addr, bitvec *val)
  void bfmStore_le(bfman *b, memory *m, bitvec *address_start, bitvec *val)
  void bfmStore_be(bfman *b, memory *m, bitvec *address_start, bitvec *val)
  void bfmStoreVector_le(bfman *b, memory *m, bitvec *address_start, vector *values)
  void bfmStoreVector_be(bfman *b, memory *m, bitvec *address_start, vector *values)
  bitvec *bfmLoad(bfman *b, memory *m, bitvec *addr)
  bitvec *bfmLoad_le(bfman *b, memory *m, bitvec *address_start, unsigned numElts)
  bitvec *bfmLoad_be(bfman *b, memory *m, bitvec *address_start, unsigned numElts)
  vector *bfmLoadVector_le(bfman *b, memory *m, bitvec *address_start, unsigned numVecs, unsigned vecSize)
  vector *bfmLoadVector_be(bfman *b, memory *m, bitvec *address_start, unsigned numVecs, unsigned vecSize)
  memory *bfmCopy(bfman *b, memory *m)
  memory *bfmSelect(bfman *b, literal c, memory *t, memory *f)
  memory *bfmFromCounterExample(bfman *b, memory *m)
  void bfmPrint(bfman *b, FILE *out, memory *m)
  void bfmDestroy(bfman *b, memory *m)
  void bfmUnsafeFree(memory *m)
