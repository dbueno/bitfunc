#ifndef machine_state_h_included
#define machine_state_h_included

#include <bitfunc.h>
#include <bitfunc/circuits.h>
#include <bitfunc/mem.h>

/**
 * Iterate over the register bitvectors in a machine state.
 *
 * \code
 * bitvec **it;
 * machine_state *s;
 * forMsReg (it, s) {
 *   ... do something with *it ...
 * }
 * \endcode
 */
#define forMsReg(it, ms) for (it = (ms)->reg; it != (ms)->reg + (ms)->numregs; ++it)

/**
 * A machine state is an abstract model of the state of a machine. Our machine
 * states support modeling any number (and size) of registers and a memory.
 *
 * A typical usage is with P-CODE. In P-CODE, each register is referred to by an
 * offset and a size. This is trivial to map onto the ::machine_state data
 * structure. See ::bfvStore_le, ::bfvLoad_le, ::bfvStore_be, and ::bfvLoad_be.
 */
typedef struct _machine_state
{
  /** State of the registers. */
  bitvec  **reg;

  unsigned numregs;

  /** size of each register, in bits */
  unsigned eltsize : 8;

  /** State of the memory. */
  memory   *ram;

  /**
   * This bit is used by other code in bitfunc (such as program.h). This bit
   * allows ::machine_state to be used to represent potential states that might
   * not actually be feasible. If that is the case, isValid should be equivalent
   * to the feasibility criteria.
   */
  literal isValid;
} machine_state;

/**
 * Construct a Machine State with registers or memory initialized but empty.
 *
 * Each register is eltsize bits; addresses are idxsize bits.
 */
machine_state *bfmsInit(bfman *m, unsigned numregs, uint8_t idxsize, uint8_t eltsize);

void bfmsCheck(bfman *b, machine_state *MS);

/**
 * Print a machine state to the stream.
 */
void bfmsPrint(bfman *b, FILE *stream, machine_state *m);

/**
 * Copies the machine state.
 */
machine_state *bfmsCopy(bfman *b, machine_state *m);

/**
 * Copies the machine state but not the RAM.
 */
machine_state *bfmsCopyNoRam(bfman *b, machine_state *m);

/**
 * Frees the machine state and registers, and frees the memory (if the memory
 * isn't pointed to by any other thing).
 */
void bfmsDestroy(bfman *b, machine_state *s);

/**
 * Muxes two machine states. If the condition is true, the returned state is
 * equivalent to t. Otherwise, it is equivalent to e.
 */
machine_state *bfmsSelect(bfman *b, literal condition, machine_state *t, machine_state *e);

/**
 * Construct concrete input from the current satisfying assignment.
 */
machine_state *bfmsFromCounterExample(bfman *b, machine_state *symIn);


/**
 * Load a native-size bit vector from a concrete address. Assumes the value is
 * stored little-endian. Side-effects the given register space.
 */
bitvec *bfRegLoad_le(bfman *b, machine_state *, unsigned address, unsigned numElts);
bitvec *bfRegLoad_be(bfman *b, machine_state *, unsigned address, unsigned numElts);

/**
 * Store a native-size bit vector at a concrete address with little-endian byte
 * order. Side-effects the given register space.
 *
 * The value val is split up into eltsize chunks, depending on the initial
 * eltsize.
 */
void bfRegStore_le(bfman *b, machine_state *, unsigned address, bitvec *val);
void bfRegStore_be(bfman *b, machine_state *, unsigned address, bitvec *val);

#endif
