/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */

#include "bitfunc/types.h"
#include "bitfunc/bitvec.h"
#include "bitfunc/array.h"
#include "bitfunc/mem.h"

/**
 * @mainpage bitfunc - modeling registers and memory
 *
 * @section intro_sec Introduction
 *
 * bitfunc is a library for reasoning about CPUs. It provides primitives for
 * representing registers and memory, as well as arbitrary arithmetic and
 * logical operations involving registers and memory. See bitfunc.h for more
 * stuff.
 *
 * Currently most of bitfunc is bit-level based. But we are working on adding
 * word-level and higher abstractions.
 *
 * Registers are represented as pointers to ::bitvec. Each (logical) RAM is
 * represented as a pointer to ::memory.
 *
 * In the test/ directory there is examples.c which contains some examples of
 * using bitfunc.
 *
 * Under the hood, memories and bit vectors are represented as And-Inverter
 * Graphs (AIGs). AIGs have the advantage of structural hashing, meaning no two
 * logic gates share the same pair of inputs. This can reduce the size of
 * circuits that are (eventually) passed to the underlying solver, and can
 * drastically reduce solving time.
 *
 * A typical use of the library is to:
 *
 *   - create a bitfunc handle (::bfInit)
 *   - create bit vector objects and constrain them with cpu operations (::bfvInit, ::bfvAdd0)
 *   - create a memory (::memory) and perform operations on locations (::bfmInit, ::bfmLoad, ::bfmStore)
 *   - provide some assumptions (::bfPushAssumption, ::bfPopAssumptions)
 *   - solve the constraints (::bfSolve)
 *   - and get the counterexample (::bfmFromCounterExample, ::bfvFromCounterExample, ::bflFromCounterExample)
 *
 * @section Logging
 *
 * bitfunc has a logging infrastructure. See debug.h. Basically, it is easy to insert:
 *
 *   - conditional code blocks based on a logging LABEL (string) and LEVEL (int): ::bf_IfDebug
 *   - conditional logging based on the same: ::bf_log
 *   - LABEL-based indentation: ::bf_dopen, ::bf_dclose
 *
 * The latter feature is most useful to nest levels within a label. Call
 * ::bf_dopen to push on an indentation stack. This begins a "parenthesis"
 * around a part of the system.  Any messages now printed are indented by two
 * leading spaced. ::bf_dclose closes the "parenthesis" so that the indentation
 * level is restored. ::bf_dopen and ::bf_dclose can be nested.
 *
 * Currently, the following labels are used (UPDATE THIS WHEN YOU ADD NEW ONES).
 *
 *   - bf: normal bitfunc top-level information
 *
 *
 * @section Program Synthesis
 *
 * bitfunc has an API supporting synthesis of arbitrary Boolean functions. The
 * main header file is program.h
 *
 * @section impl_sec Implementation
 *
 * Important implementation files: bitvec.c, circuits.c, memory.c, program.c.
 */

/**
 * Creates a new bitfunc handle.
 */
bfman *bfInit(bfmode mode);

/**
 * Push an assumption onto the assumptions stack. If the return value is
 * BF_UNSAT, the assumption was not pushed.
 */
bfresult bfPushAssumption(bfman *m, literal p);

/**
 * Push a bit vector of assumptions, from least to most significant bit. If
 * the return value is BF_UNSAT, no assumptions are pushed.
 */
bfresult bfvPushAssumption(bfman *m, bitvec *p);

/**
 * Pops 'num' assumptions from the assumptions stack.
 */
void bfPopAssumptions(bfman *m, unsigned num);

/**
 * Solve the current SAT problem with funcsat. Assumptions are passed using
 * ::bfPushAssumption.
 */
bfresult bfSolve(bfman *m);

/**
 * Configure the bitfunc instance which solver to use
 */
void bfConfigureFuncsat(bfman *);
void bfConfigurePicosat(bfman *);
void bfConfigurePicosatIncremental(bfman *);
void bfConfigureLingeling(bfman *);
void bfConfigurePlingeling(bfman *);
void bfConfigurePrecosat(bfman *);
void bfConfigurePicosatReduce(bfman *);
void bfConfigureLingelingReduce(bfman *);
void bfConfigureExternal(bfman *b);

/**
 * Solves the current AIG (under assumptions) with funcsat.
 */
bfresult bfFuncsatSolve(bfman *);

/**
 * Solves the current SAT problem using picosat via a 'system' call. This solves
 * on the same state as bfSolve.
 */
bfresult bfPicosatSolve(bfman *);
bfresult bfLingelingSolve(bfman *);
bfresult bfPlingelingSolve(bfman *);
bfresult bfPrecosatSolve(bfman *);

/**
 * Solves the current SAT problem using picosat via a 'system' call, after
 * reducing the problem with ABC. The state is first passed through ABC's
 * reduction calls and special CNF printer.
 */
bfresult bfPicosatReduceSolve(bfman *);
bfresult bfLingelingReduceSolve(bfman *);

/**
 * BROKEN DO NOT USE
 */
bfresult bfABCSolve(bfman *m, int64_t lim);

const char *bfResultAsString(bfresult result);

/**
 * Prints the CNF for the SAT problem that would be solved if ::bfSolve were
 * called.
 */
void bfPrintCNF(bfman *m, char *cnf_file);

/**
 * Reset assumptions and assignment. Does not affect the problem
 * constraints. You shouldn't need to call this, but it can be convenient.
 */
void bfReset(bfman *m);

/**
 * Free the solver state. Also frees:
 *
 *   - all bit vectors
 *   - all memory states
 *   - all program synthesis-related stuff
 */
void bfDestroy(bfman *m);

/** @return the number of variables currently in the instance */
unsigned bfNumVars(bfman *m);

/** @return the number of input variables (i.e. not derived for some other
 * constraint) currently in the instance */
unsigned bfNumInputs(bfman *m);

/** Set the number of variables currently in the problem. This is useful when
 * combining existing CNF with extra bitfunc constraints on top.
 */
void bfSetNumVars(bfman *m, unsigned numVars);

/**
 * Sets the literal to true.
 */
bfresult bfSet(bfman *m, literal a);
/**
 * Treats the bit vector as a clause, requiring the disjunction of all the
 * literals in c to hold.
 */
void bfSetClause(bfman *b, bitvec *c);


/// fetch the current satisfying assignment for the given literal. If there is
/// no current satisfying assignment, the result of calling this function is
/// undefined.
bool bfGet(bfman *m, literal a);

///
/// Converts the bit vector (under the current assignment) to an unsigned
/// integer. the bit vector is effectively zero-extended.
uint64_t bfvGet(bfman *m, bitvec *bv);

/**
 * @return the concrete literal from the current satisfying assignment
 */
literal bflFromCounterExample(bfman *b, literal l);

/**
 * @return the concrete bit vector from the current satisfying assignment
 */
bitvec *bfvFromCounterExample(bfman *b, bitvec *bv);

/**
 * @return the concrete memory from the current satisfying assignment
 */
memory *bfmFromCounterExample(bfman *b, memory *m);

/**
 * @return the literal from the variable v which is assigned to true by the
 * current satisfying assignment
 */
literal bfAssignmentVar2Lit(bfman *b, variable v);


/**
 * Ask the SAT solver what it can tell us about the literal, one way or the
 * other. All of the returns are subject to incompleteness if the lim argument
 * is not -1.
 *
 * @return
 *   - ::STATUS_MUST_BE_TRUE if -a is a contradictory assignment
 *   - ::STATUS_MUST_BE_FALSE if a is a contradictory assignment
 *   - ::STATUS_TRUE_OR_FALSE if neither a or -a is contradictory
 *   - ::STATUS_NOT_TRUE_NOR_FALSE if both a and -a are contradictory
 *   - ::STATUS_UNKNOWN otherwise
 */
bfstatus bfCheckStatus(bfman *m, literal a);

/**
 * @return if there is an assignment with the literal set to true
 */
mbool bfMayBeTrue(bfman *m, literal x);
/**
 * @return if there is an assignment with the literal set to false
 */
mbool bfMayBeFalse(bfman *m, literal x);
/**
 * @return if there is no assignment with the literal set to false
 */
mbool bfMustBeTrue(bfman *m, literal x);
/**
 * @return if there is no assignment with the literal set to true
 */
mbool bfMustBeFalse(bfman *m, literal x);

/**
 * @return if there is no assignment with the literal set to false (makes use
 * of picosat instead of funcsat)
 */
mbool bfMustBeTrue_picosat(bfman *man, literal x);
mbool bfMustBeTrue_picosatIncremental(bfman *man, literal x);


void bfPrintAIG(bfman *b, FILE *out);
void bfPrintAIG_dot(bfman *b, const char *path);


/**
 * @return if debugging for the given label was already enabled, returns false;
 * otherwise returns true
 */
bool bfEnableDebug(bfman *, char *label, int maxlevel);

/**
 * @return true iff logging for the given label was enabled
 */
bool bfDisableDebug(bfman *, char *label);

void bfPrintStats(bfman *, FILE *);

void bfFindCore_picomus(bfman *, const char *core_output_file);
void bfPrintNamedCNF(bfman *, const char *cnf_file);

void bfWriteAigerToFile(bfman *b, const char *path);

/**
 * Simulate an entire concrete input.
 *
 * WARNING: this function calls bfReset.
 */
void bfSimulate(bfman *b, bool *inputs);

/**
 * Given an input variable returns the input index of that variable. See bfSimulate.
 *
 * If the given variable is not an input, returns UINTMAX_MAX.
 */
unsigned bfGetInputIndex(bfman *b, variable v);
