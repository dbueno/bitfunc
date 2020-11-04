
#ifndef bf_types_h
#define bf_types_h

#include <funcsat/system.h>
#include <funcsat/hashtable.h>
#include <funcsat/vec.h>
#include <funcsat.h>
#include <bitfunc/aiger.h>
#if defined(HAVE_SETJMP_LONGJMP)
#include <setjmp.h>
#endif

/** The literal representing true. Never pass this to the SAT solver. */
#define LITERAL_TRUE LITERAL_MAX
/** The literal representing false. Never pass this to the SAT solver. */
#define LITERAL_FALSE (-LITERAL_MAX)
#define bf_litIsTrue(l) ((l) == LITERAL_TRUE)
#define bf_litIsFalse(l) ((l) == LITERAL_FALSE)
/** @return true iff the literal is either ::LITERAL_TRUE or ::LITERAL_FALSE. */
#define bf_litIsConst(l) ((l) == LITERAL_TRUE || (l) == LITERAL_FALSE)
/** @return the literal corresponding to the given bool */
#define bf_bool2lit(b) ((b) ? LITERAL_TRUE : LITERAL_FALSE)
#define bf_lit2idx(l) (((l) << 1) & ((l) < 0))

#if defined(HAVE_SETJMP_LONGJMP) && defined(USE_LONGJMP)

#define BF_SETJMP(bf) setjmp((bf)->jmp_env)
#define BF_ERROR_MSG(bf) (bf)->jmp_error_msg
#define BF_ERROR_MSG_FREE(bf) free(BF_ERROR_MSG(bf))

/* code sent back to setjmp() by longjmp(). MUST NOT BE 0. */
#define BF_INPUTERROR 2

#define INPUTCHECK(bf, cond, msg)                                       \
  if (!(cond)) {                                                        \
    bf->jmp_error_msg = malloc(1024);                                   \
    snprintf(bf->jmp_error_msg, 1024,                                   \
             "*** bitfunc:%s:%d: %s\n", __FILE__, __LINE__, msg);       \
    longjmp(bf->jmp_env, BF_INPUTERROR);                                \
  }

#else

/* essentially does an assert() */
#define INPUTCHECK(bf, cond, msg)                                       \
  if (!(cond)) {                                                        \
    fprintf(stderr, "*** bitfunc:%s:%d: %s\n", __FILE__, __LINE__, msg); \
    fprintf(stderr, "*** condition: " #cond "\n");                      \
    abort();                                                            \
  }
#endif

/**
 * Main result of invoking the SAT solver.
 */
typedef enum
{
/** This may be returned if the SAT solver was invoked in an incomplete mode,
 * e.g., by limiting the number of backtracks the solver is allowed to perform.
 */
BF_UNKNOWN,

BF_SAT,
BF_UNSAT
} bfresult;

/**
 * See ::bfCheckStatus.
 */
typedef enum
{
  STATUS_MUST_BE_TRUE,
  STATUS_MUST_BE_FALSE,
  STATUS_TRUE_OR_FALSE,
  STATUS_NOT_TRUE_NOR_FALSE,
  STATUS_UNKNOWN
} bfstatus;

/**
 * You probably won't need to use this. See ::bfvZextend, ::bfvSextend. Then
 * look at ::bfvExtend.
 */
typedef enum { EXTEND_SIGN, EXTEND_ZERO } extendty;

/**
 * bitfunc supports two modes of constraints under the hood. You have to pick
 * one when you call ::bitfuncInit.
 */
typedef enum
{
/** All constraints are turned into And-Inverter Graph nodes, with structural hashing. */
AIG_MODE
} bfmode;

struct and_key { literal a; literal b; };

/**
 * We standardise on the following representation: the least significant bit
 * of a number is position zero of this type; the most significant bit is at
 * size-1. */

typedef struct bitvec
{
  literal *data;

  /** how many literals */
  uint32_t size;

  /** how much malloc'd space (in literal's) */
  uint32_t capacity;

  /** Many functions will free (consume) their bitvec arguments. To stop a
   *  function from consuming a bitvec, this variable must be greater than
   *  0. To make this bitvec consumable, set this variable to zero. This
   *  variable is initially zero */
  uint8_t numHolds;

  /**
   * Used in printing (sometimes) to help figure out where low-level constraints
   * came from.
   */
  char *name;

} bitvec;

typedef struct bfman
{
  unsigned numVars;
  bitvec *assumps;

  /**
   * All active memories that have been created.
   */
  vector memories;

  /**
   * Adds a constraint to the current AIG. The constraint is expressed as a clause.
   */
  void (*addClause)(struct bfman *, bitvec *);

  /**
   * Adds an AND gate to the AIG.
   */
  void (*addAnd)(struct bfman *, literal lhs, literal rhs0, literal rhs1);

  /**
   * Sets the given literal to be an output of the AIG.
   */
  void (*addAIGOutput)(struct bfman *, literal);

  /**
   * Adds a clause to the current backend solver. Since we support multiple
   * backends, this can change dynamically.
   *
   * Also, the clause is still owned by the caller upon return from this
   * function. So if the caller doesn't want the clause anymore, he should free
   * it.
   */
  void (*addClauseToSolver)(struct bfman *, clause *, void *);

  bfresult (*solve)(struct bfman *);


  funcsat *funcsat;

  aiger *aig;
  /**
   * current satisfying assignment for each AIG node. indexed by aiger variables
   * (aiger_lit2var(aiger_literal))
   */
  mbool *aigAssignment;
  unsigned aigAssignment_size;
  bfmode mode;
  struct hashtable *andhash;
  unsigned hash_hits;
  unsigned memflag;

  /** whether memories are auto-compressed. */
  bool autocompress;

  /** whether to query the solver when loading from memory to try to
   * simplify. currently is really slow for a lot of reads. */
  bool memLoadWithSat;

  /** whether the incremental picosat has been initialized. */
  bool picosatInitd;

  uint64_t numPicosatIncrementalSolves;
  float timePicosatIncrementalSolve;

  /** names of individual literals */
  struct hashtable *lit_names;
  struct hashtable *logSyms;
  posvec logStack;
  /** debug output stream */
  FILE *dout;

  char *jmp_error_msg;
#if !defined(HAVE_SETJMP_LONGJMP)
  void *jmp_env;
#else
  jmp_buf jmp_env;              /* KEEP THIS AT THE END OF THE BFMAN */
#endif
} bfman;

#endif
