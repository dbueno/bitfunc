#ifndef internal_h_included
#define internal_h_included

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"


/**
 * Undoes the given set of unit assumptions.  Assumes the decision level is 0.
 */
void undoAssumptions(funcsat *func, clause *assumptions);

funcsat_result startSolving(funcsat *f);

/**
 * Do unit propagation from funcsat::propq and funcsat::bpropq.
 *
 * If we discover a conflict, returns False and sets funcsat::conflictClause.
 * Otherwise returns True.
 */
bool bcp(funcsat *func);

/**
 * Chooses a literal to assign, assigns it, and returns it.
 */
literal funcsatMakeDecision(funcsat *func, void *);

/**
 * Adds a clause containing no duplicate literals to the problem.
 *
 * @pre decision level is 0
 */
funcsat_result addClause(funcsat *f, clause *clause);

/**
 * Analyses conflict, produces learned clauses, backtracks, and asserts the
 * learned clauses.  If returns false, this means the SAT problem is unsat; if
 * it returns true, it means the SAT problem is not known to be unsat.
 */
bool analyseConflict(funcsat *func);

/**
 * Undoes the trail and assignments so that the new decision level is
 * ::newDecisionLevel.  The lowest decision level is 0.
 *
 * @param func
 * @param newDecisionLevel
 * @param isRestart
 * @param facts same as in ::trailPop
 */
void backtrack(funcsat *func, variable newDecisionLevel, clause **facts, bool isRestart);


void addBinaryWatch(funcsat *f, clause *clause);

/**
 * Watches a new clause by finding appropriate literals to watch.
 */
void addWatch(funcsat *f, vector *watches, clause *clause);
/**
 * Watches a new clause assuming the first two literals are okay to watch.
 */
void addWatchUnchecked(funcsat *f, vector *watches, clause *clause);
void makeWatchable(funcsat *f, clause *clause);


/* Mutators */
/**
 * The way to tell funcsat about an inference.
 *
 * @param reason must be non-NULL
 */
void trailPush(funcsat *f, literal p, clause *reason);

/**
 * Removes the last set literal from the trail. In order to pop the trail, you
 * usually want to make sure you don't use any facts we have around for the
 * literal you're popping. If that is true, supply a list to merge with. If
 * facts is NULL, any facts are released.
 *
 * @param facts if non-NULL, the clause pointed at will contain any facts
 * associated with the literal popped
 */
literal trailPop(funcsat *f, clause **facts);
/**
 * @return the literal at the top of the trail.
 */
literal trailPeek(funcsat *f);

void printClause(funcsat *f, FILE *out, clause *c);
/**
 * Print a clause in DIMACS format.
 */
void dimacsPrintClause(FILE *f, clause *);
void dimacsPrintClauses(FILE *f, vector *);

void funcsatPrintState(funcsat *, FILE *);
void funcsatPrintConfig(FILE *f, funcsat *);


bool funcsatIsResourceLimitHit(funcsat *, void *);
funcsat_result funcsatPreprocessNewClause(funcsat *, void *, clause *);
funcsat_result funcsatPreprocessBeforeSolve(funcsat *, void *);
variable funcsatLearnClauses(funcsat *, void *);

int varOrderCompare(fibkey *, fibkey *);
double funcsatDefaultStaticActivity(variable *v);
void lbdBumpActivity(funcsat *f, clause *c);
void varBumpActivity(funcsat *f, variable v);
void varsBumpActivity(funcsat *f, clause *c);

void bumpReasonByActivity(funcsat *f, clause *c);
void bumpLearnedByActivity(funcsat *f, clause *c);
void bumpReasonByLbd(funcsat *f, clause *c);
void bumpLearnedByLbd(funcsat *f, clause *c);
void bumpOriginal(funcsat *f, clause *c);
void bumpUnitClauseByActivity(funcsat *f, clause *c);

void singlesPrint(FILE *stream, clause *begin);

/************************************************************************/
/* Helpers */

bool watcherFind(clause *c, clause **watches, uint8_t w);
void watcherPrint(FILE *stream, clause *c, uint8_t w);
void singlesPrint(FILE *stream, clause *begin);
void binWatcherPrint(FILE *stream, funcsat *f);


/**
 * FOR DEBUGGING
 */
bool isUnitClause(funcsat *f, clause *c);

/**
 * Like ::value, but only looks at values that have been propagated by bcp. This
 * can be fewer, but never more, than those that are assigned.
 */
mbool tentativeValue(funcsat *f, literal p);

/**
 * The body of ::funcsatValue.
 */
#define funcsatValueBody(f,p) {                         \
    variable v = fs_lit2var(p);                         \
    if (f->level.data[v] == Unassigned) return unknown; \
    literal value = f->trail.data[f->model.data[v]];    \
    return p == value; }


/**
 * Returns the decision level of the given variable.
 */
literal levelOf(funcsat *f, variable v);

variable fs_lit2var(literal l);
/**
 * Returns the positive literal of the given variable.
 */
literal fs_var2lit(variable v);
/**
 * Converts lit to int suitable for array indexing */
unsigned fs_lit2idx(literal l);

bool isDecision(funcsat *, variable);


/* sorted clause manipulation */

/**
 * Sorts the literals in a clause.  Useful for performing clause operations
 * (::findLiteral, ::findVariable, ::clauseRemove, and
 * resolution) in logarithmic time in the size of the clause.
 */
void sortClause(clause *c);
/**
 * Finds a literal using binary search on the given clause.  Returns the
 * literal's index (0 .. clause->size-1) if found, -1 otherwise.
 */
literal findLiteral(literal l, clause *clause);
/**
 * Same as ::findLiteral but works on variables.
 */
literal findVariable(variable l, clause *clause);

unsigned int fsLitHash(void *);
int fsLitEq(void *, void *);
int litCompare(const void *l1, const void *l2);

typedef struct litpos
{
  clause *c;
  posvec *ix;
} litpos;

static inline struct litpos *buildLitPos(clause *c)
{
  literal *lIt;
  litpos *p;
  CallocX(p, 1, sizeof(*p));
  p->c = c;
  p->ix = posvecAlloc(c->size);
  p->ix->size = c->size;
  unsigned *uIt;
  forPosVec (uIt, p->ix) *uIt = UINT_MAX;
  forClause (lIt, p->c) {
    p->ix->data[fs_lit2idx(*lIt)] = (unsigned)(lIt - p->c->data);
  }
  return p;
}

static inline literal *clauseLitPos(litpos *pos, literal l)
{
  return pos->c->data + pos->ix->data[fs_lit2idx(l)];
}

static inline bool clauseContains(litpos *pos, literal l)
{
  return pos->ix->data[fs_lit2idx(l)] != UINT_MAX;
}



static inline unsigned getLitPos(funcsat *f, literal l)
{
  return f->litPos.data[fs_lit2var(l)];
}

static inline void setLitPos(funcsat *f, literal l, unsigned pos)
{
  f->litPos.data[fs_lit2var(l)] = pos;
}

/* clear lit position in clause */
static inline void resetLitPos(funcsat *f, literal l, unsigned pos)
{
  setLitPos(f, l, UINT_MAX;
}

static inline bool hasLitPos(funcsat *f, literal l)
{
  return getLitPos(f,l) != UINT_MAX;
}

static inline clause *getReason(funcsat *f, literal l)
{
  return f->reason.data[fs_lit2var(l)];
}

static inline void spliceUnitFact(funcsat *f, literal l, clause *c)
{
  clause *fakeHead = &f->unitFacts.data[fs_lit2var(l)];
  clauseSplice1(c, &fakeHead);
}

#endif
