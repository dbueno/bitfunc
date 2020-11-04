#ifndef bf_bitvec_h
#define bf_bitvec_h

#include <stdbool.h>

#include "types.h"

/**
 * Creates a bit vector containing fresh, symbolic literals of the given
 * width. The bit vector's reference count is 0. The next time this bit vector
 * is passed to any bfv* function that returns a new bitvec* or literal, if the
 * reference count is 0, the bitvec will be free'd. Therefore, if the client
 * needs this bitvec to persist across multiple such calls, the client should
 * call ::bfvHold (and subsequently ::bfvRelease).
 */
bitvec *bfvInit(bfman *m, uint32_t width);

/**
 * Creates an empty bit vector with the given initial capacity. This is a LOW
 * LEVEL function. Normal users should be using ::bfvInit.
 */
bitvec *bfvAlloc(uint32_t initialCapacity);

/**
 * Push a literal onto the end of a bitvec.
 */
void bfvPush(bitvec *v, literal data);

/**
 * Remove and return the last literal from a bitvec.
 */
literal bfvPop(bitvec *v);

/**
 * Increase the underlying capacity of a bitvec.
 */
void bfvGrowTo(bitvec *v, uint32_t newCapacity);

/**
 * Sets the size of the bit vector to 0.
 */
void bfvClear(bitvec *v);
void bfvDestroy(bitvec *v);

inline bitvec *bfvHold(bitvec *vec)
{
  return vec->numHolds++, vec;
}

inline bitvec *bfvRelease(bitvec *vec)
{
  assert(vec->numHolds!=0 && "releasing a bitvec that has 0 holds.\n");
  return vec->numHolds--, vec;
}

inline void CONSUME_BITVEC(bitvec *vec)
{
  if (vec && vec->numHolds==0) {
    bfvDestroy(vec);
  }
}

inline void bfvReleaseConsume(bitvec *vec)
{
  CONSUME_BITVEC(bfvRelease(vec));
}

/**
 * Iterate over a bitvec.
 *
 * \code
 * bitvec *y = ...
 * literal *lIt;
 * forBv(lIt, y) { ... do something with *lIt ... }
 * \endcode
 *
 */
#define forBv(elt, vec) \
  for (elt = (vec)->data; elt != (vec)->data + (vec)->size; elt++)

/**
 * Iterate over a bitvec in reverse.
 */
#define forBvRev(elt, vec) \
  for (elt = (vec)->data + (vec)->size - 1; elt != (vec)->data - 1; elt--)


typedef enum bfvprint_mode { PRINT_BINARY, PRINT_HEX } bfvprint_mode;

/**
 * Prints the symbolic literals that compose a bit vector.
 */
void bfvPrint(FILE *stream, bitvec *b);

/**
 * Prints the (possibly) symbolic literal.
 */
void bflPrint(FILE *stream, literal l);

/**
 * Prints a concrete bit vector value. The value is printed the way you would
 * write it, with the most significant parts first. In ::PRINT_BINARY, if the
 * assignment of a literal is unknown, a '?' is printed.
 *
 * @param mode whether to print bits or hex
 */
void bfvPrintConcrete(bfman *m, FILE *stream, bitvec *b, bfvprint_mode mode);

/**
 * Tests if the two arguments have the same bits
 * @return
 *   - if true, they are trivially equal
 *   - if false, they are trivially not equal
 *   - unknown otherwise
 */
mbool bfvSame(bitvec *b1, bitvec *b2);

/**
 * Tests if the bitvector is a constant value
 */
bool bfvIsConstant(bitvec *x);

/**
 * Duplicates the given string and stores it in the bit vector. Will be free'd
 * when the bit vector is consumed.
 */
bitvec *bfvSetName(bfman *b, bitvec *x, const char *name);

/**
 * A vector of all zeros of the given size.
 */
bitvec *bfvZero(const uint32_t sz);

/**
 * A vector of all ones of the given size.
 */
bitvec *bfvOnes(const uint32_t sz);

/**
 * A vector of 0x1 of the given size.
 */
bitvec *bfvOne(const uint32_t sz);


// bit vector ops

///
/// elementwise and
bitvec *bfvAnd(bfman *m, bitvec *x, bitvec *y);

///
/// elementwise or
bitvec *bfvOr(bfman *m, bitvec *x, bitvec *y);
///
/// elementwise xor
bitvec *bfvXor(bfman *m, bitvec *x, bitvec *y);

///
/// Twos-complement negation (bfman *m, -x)
bitvec *bfvNegate(bfman *m, bitvec *x);

///
/// Twos-complement absolute value
bitvec *bfvAbsoluteValue(bfman *m, bitvec *x);

///
/// Invert all the bits in x (~x)
bitvec *bfvInvert(bfman *m, bitvec *x);

///
/// returns thn if t, els otherwise
bitvec *bfvSelect(bfman *m, literal t, bitvec *thn, bitvec *els);

/**
 * Extend the bit vector to a larger size. But you probably want either
 * ::bfvSextend or ::bfvZextend.
 */
bitvec *bfvExtend(bfman *m, uint32_t bfNew_width, bitvec *v, extendty ty);

///
/// zero extend the given bit vector to the bfNew width
///
/// @pre bfNewWidth must be at least the current width
inline bitvec *bfvZextend(bfman *man, uint32_t new_width, bitvec *v)
{
  return bfvExtend(man, new_width, v, EXTEND_ZERO);
}

///
/// sign extend the given bit vector to the bfNew width
///
/// @pre bfNewWidth must be at least the current width
inline bitvec *bfvSextend(bfman *man, uint32_t new_width, bitvec *v)
{
  return bfvExtend(man, new_width, v, EXTEND_SIGN);
}


///
/// chop off the top of the bit vector so that it has exactly bfNewWidth
///
/// @pre bfNewWidth must be at most v's current width
bitvec *bfvTruncate(bfman *m, uint32_t bfNewWidth, bitvec *v);

/// Returns the sum and carry bit of the computation x+y, assuming x and y
/// are bit vectors in 2's complement representation.
///
/// @param cin carry in
bitvec *bfvAdd(bfman *m, bitvec *x, bitvec *y, literal cin, literal *cout);
//inline bitvec *bfvAdd0(bfman *m, bitvec *x, bitvec *y)
//{
//  return bfvAdd(m, x, y, LITERAL_FALSE, NULL);
//}
#define bfvAdd0(m,x,y) bfvAdd(m,x,y,LITERAL_FALSE,NULL)

/// Subtraction
bitvec *bfvSubtract(bfman *m, bitvec *x, bitvec *y);

///
/// Unsigned less than
literal bfvUlt(bfman *m, bitvec *x, bitvec *y);

///
/// Unsigned less-than or equal
literal bfvUlte(bfman *m, bitvec *x, bitvec *y);

///
/// Signed less than
literal bfvSlt(bfman *m, bitvec *x, bitvec *y);
literal bfvSlte(bfman *m, bitvec *x, bitvec *y);

///
/// Signed less than 0
/// @return the sign bit of x
literal bfvSltZero(bfman *m, bitvec *x);

///
/// Signed greater than zero
literal bfvSgtZero(bfman *m, bitvec *x);

///
/// Signed greater than or equal to 0
literal bfvSgteZero(bfman *m, bitvec *x);

///
/// Returns a literal that's true iff the input bit vectors are elementwise equal
literal bfvEqual(bfman *m, bitvec *x, bitvec *y);

///
/// For compatibility
#define bfvConstant(m,w,v,s) ((s) == true ? bfvSconstant(m,w,v) : bfvUconstant(m,w,v))

///
/// Creates a signed bit vector constant in two's complement representation
bitvec *bfvSconstant(bfman *m, uint32_t width, int value);
///
/// Creates an unsigned bit vector constant in two's complement representation
bitvec *bfvUconstant(bfman *m, uint32_t width, unsigned value);

///
/// Shift left
///
/// @param val value to shift; width must be pow(2,x)
/// @param dist amount to shift; width must be log2(width(bfman *m, val))
bitvec *bfvLshift(bfman *m, bitvec *val, bitvec *dist);

///
/// Shift left, i.e., add 0s in the least significant bits
///
/// if dist is too large, it is treated as if you shifted as much as possible
/// (i.e. result is all zeros)
bitvec *bfvLshiftConst(bfman *m, bitvec *val, uint32_t dist);

///
/// Shift right, i.e., adding fills in the most significant bits
///
/// A logical right shift can be accomplished by passing const_literal(false) as
/// the fill (this is the default). An arithmetic right shift can be
/// accomplished by passing val.data[val.size()-1] as the fill.
bitvec *bfvRshift(bfman *m, bitvec *val, bitvec *dist, literal fill);
bitvec *bfvRshiftConst(bfman *m, bitvec *val, uint32_t dist, literal fill);

inline bitvec *bfvLRshift(bfman *m, bitvec *val, bitvec *dist) {
  return bfvRshift(m, val, dist, LITERAL_FALSE);
}

inline bitvec *bfvARshift(bfman *m, bitvec *val, bitvec *dist) {
  return bfvRshift(m, val, dist, val->data[val->size-1]);
}

///
/// Rotate bits to the right, i.e., rotating bits down to the LSB
bitvec *bfvRrotate(bfman *bf, bitvec *x, bitvec *shift);
///
/// Rotate bits to the left, i.e., rotating bits up to the MSB
bitvec *bfvLrotate(bfman *bf, bitvec *x, bitvec *shift);

///
/// multiply x and y.  overflow is ignored.
bitvec *bfvMul(bfman *m, bitvec *x, bitvec *y);

///
/// Returns a pair (q,r) containing the quotient q=x/y and the remainder
/// r=x%y.
void bfvQuotRem(bfman *m, bitvec *x, bitvec *y, bitvec **quot, bitvec **rem);

bitvec *bfvSdiv(bfman *m, bitvec *x, bitvec *y);
bitvec *bfvSrem(bfman *b, bitvec *x, bitvec *y);


inline bitvec *bfvQuot(bfman *m, bitvec *x, bitvec *y)
{
  bitvec *q;
  bitvec *r;
  bfvQuotRem(m, x, y, &q, &r);
  CONSUME_BITVEC(r);
  return q;
}

inline bitvec *bfvDiv(bfman *m, bitvec *x, bitvec *y)
{
  return bfvQuot(m, x, y);
}

inline bitvec *bfvRem(bfman *m, bitvec *x, bitvec *y)
{
  bitvec *q;
  bitvec *r;
  bfvQuotRem(m, x, y, &q, &r);
  CONSUME_BITVEC(q);
  return r;
}

///
/// copy the contents of src into dst.  if dst isn't big enough, its size is
/// increased. src is not CONSUME_BITVEC'd.
void bfvCopy(bfman *m, bitvec *dst, bitvec *src);

/**
 * Create and return a duplicate of src. Metadata (e.g. holds) is reset as
 * though one had alloc'd the result.
 */
bitvec *bfvDup(bfman *man, bitvec *src);

///
/// 
bitvec *bfvConcat(bfman *m, bitvec *x, bitvec *y);
bitvec *bfvConcat3(bfman *m, bitvec *x, bitvec *y, bitvec *z);
///
/// @return a bit vector that is the length-len substring of x beginning at begin
bitvec *bfvExtract(bfman *m, bitvec *x, uint32_t begin, uint32_t len);

/**
 * Returns a bit vector with the bits in the opposite order.
 */
bitvec *bfvReverse(bfman *b, bitvec *x);

/**
 * Increment a bit vector by one.
 */
bitvec *bfvIncr(bfman *b, bitvec *x);


unsigned hashBv(void *k);

int bvEqual(void *k1, void *k2);

#endif
