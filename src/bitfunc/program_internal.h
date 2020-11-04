#ifndef program_internal_h_included
#define program_internal_h_included

#include <bitfunc.h>
#include <bitfunc/circuits.h>
#include <bitfunc/mem.h>

/**
 * A user should never have to look inside a program.
 */
struct program_t
{
  /**
   * instruction[i][j] is true if and only if means program instruction i is
   * implemented by ::instruction j
   */
  literal **instruction;

  /**
   * Number of instructions used by the program.
   */
  unsigned len;

  /** symbolic input */
  machine_state *symIn;
  /** symbolic output */
  machine_state *symOut;

  /** if true, the program actually chose exactly one instruction per allowed
   * instruction */
  literal isValid;

  /** Total time spent in synthesis */
  float totalSynthSecs;
  /** Total time spent in checking against oracle */
  float totalCheckSecs;
};



/**
 * Construct a program with a list of instructions, a length, and a symbolic
 * machine input (at least what the user wants to be symbolic is symbolic).
 */
program *bfpInit(bfman *b, specification *spec);

program *programForTestVector(bfman *b, specification *spec, program *oldp, machine_state *input);

/**
 * Given an existing program, increase its length to len, creating new variables
 * for instruction choices, recompute isValid, and step the program to the new
 * length.
 */
void programResize(bfman *b, specification *spec, program *p, unsigned len);

/**
 * Given an existing program p, increase its size so it matches the source
 * program, copy the instruction choice variables from the source, and step p to
 * the new length.
 */
void programResizeForNewMachine(bfman *b, specification *spec, program *p, program *source);

/**
 * @return step instruction insn of the program given the initial machine state,
 * and return the final state
 */
machine_state *stepProgram(bfman *b, specification *spec, program *program, unsigned insn, machine_state *m);
/**
 * Step all program instructions for the given initial state.
 *
 * @return the final state
 */
machine_state *runProgram(bfman *b, specification *spec, program *program, machine_state *initState);
/**
 * @return a literal to require the program of the given length to be valid
 */
literal programIsValid(bfman *b, specification *spec, program *program);

/** ensures current assignment is valid */
void checkProgramAssignment(bfman *b, specification *spec, program *program);

#endif
