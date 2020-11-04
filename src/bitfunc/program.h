#ifndef program_h_included
#define program_h_included

#include <bitfunc.h>
#include <bitfunc/circuits.h>
#include <bitfunc/mem.h>
#include <bitfunc/machine_state.h>


/** A program. The user should never have to know what's inside this. */
typedef struct program_t program;


/**
 * A synthesized program may only use certain instructions. Each instruction has
 * a state transformation and a description. Once you've modeled all your
 * instructions, you need to put them inside ::specification::instructions.
 */
typedef struct instruction
{
  /**
   * Implementation should side-effect the given machine in order to implement
   * the operation.
   *
   * @param p data for the instruction being executed
   */
  void (*impl)(bfman *b, machine_state *s);

  /**
   * A human-readable description of this instruction.
   */
  char *desc;
} instruction;


/**
 * A specification is a precise description of the program you want to
 * synthesize. Each field in this struct has to be set by the user.
 */
typedef struct specification
{
  /**
   * The synthesized program will be at least this length.
   */
  unsigned minlen;

  /**
   * The synthesized program will be at most this length.
   */
  unsigned maxlen;

  /**
   * The client must provide at least one concrete input vector. The input
   * vector must be valid, and compatible with ::initialState.
   *
   * @pre non-empty
   * @pre each vector is concrete (i.e. involving only ::LITERAL_TRUE and LITERAL_FALSE)
   */
  vector *testVectors;

  /**
   * The state of the machine where everything that is known about the initial
   * state is concrete, and values unknown are symbolic. If nothing is symbolic
   * in the initial state, then the only test vector in ::testVectors in must be
   * this ::initialState.
   *
   * Any vector in ::testVectors must be "compatible" with this initial state,
   * in the sense that any solution to the symbolic parts of ::initialState must
   * be able to take the concrete values of any member of ::testVectors.
   */
  machine_state *initialState;

  /**
   * An oracle is a function that tests an initial and a final machine state
   * against your specification.
   *
   * @param init input state
   *
   * @param final final state
   *
   * @return a literal indicating whether the initial & final states are correct
   * according to the underlying spec
   */
  literal (*oracle)(bfman *b, machine_state *init, machine_state *final);

  /**
   * List of ::instruction. These are the machine instructions that are allowed
   * in the program to be synthesized.
   */
  vector *instructions;


  bool sanityCheckFinalStates;
} specification;

/**
 *
 * Generate a program that fits the given specification.
 *
 * For details on the requirements of the specification, see ::specification.
 *
 * @return a concrete program, or NULL
 */
program *bfSynthesizeProgram(bfman *b, specification *spec);



/**
 * @return a concrete program from the current satisfying assignment
 */
program *bfpFromCounterExample(bfman *b, specification *spec, program *sym);

/**
 * Print the given program in terms of the specification's instructions. The
 * program must be concrete or there must be a current satisfying assignment.
 */
void bfpPrint(bfman *b, FILE *stream, specification *spec, program *p);

/**
 * Print synthesis stats for a concrete program returned from
 * ::bfSynthesizeProgram.
 */
void bfpPrintStats(bfman *b, FILE *stream, specification *spec, program *p);

program *bfpCopy(bfman *b, specification *spec, program *p);


void bfpDestroy(bfman *b, program *p, specification *spec);


#endif
