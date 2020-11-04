% -*- mode: cweb -*-

% Copyright 2012 Sandia Corporation. Under the terms of Contract
% DE-AC04-94AL85000, there is a non-exclusive license for use of this work by or
% on behalf of the U.S. Government. Export of this program may require a license
% from the United States Government.

\def\todo{TODO COMMAND}
\let\ifpdf=\relax
\def\NULL{{\tt NULL}} %I think the default greek thing is confusing
\input eplain
\beginpackages
  \usepackage{url}
  \usepackage[dvipsnames]{color}
\endpackages
\enablehyperlinks
\hlopts{bwidth=0}
\hlopts[url]{colormodel=named,color=BlueViolet}

\let\footnote=\numberedfootnote
\listleftindent=2\parindent



\input font_palatino

\let\cmntfont=\sl
\let\mainfont=\rm
\mainfont
\def\paragraphit#1{{\it #1}\hskip\parindent}


\def\funcsat{{\caps funcsat}}
\def\minisat{{\caps minisat}}
\def\picosat{{\caps picosat}}



% Typical sections for me:

% External types -- goes into <file>.h
% External declarations -- goes into <file>.h after External ty...
% Internal types -- goes into <file>_internal.h
% Internal declarations -- goes into <file>_internal.h after Internal ty...
% Global definitions -- goes into <file>.c after Internal decl...


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                                   IT BEGINS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%




@* Introduction.  \funcsat\ is a CDCL SAT solver, written by Denis Bueno
\url{denis.bueno@@sandia.gov} at Sandia National Labs. It was developed under
LDRD funding. It is written in ANSI \CEE/99.

funcsat's main goal is {\it flexibility}. It has a bunch of features, blah blah.

@s unsigned int
@s clause int
@s funcsat int
@s literal int
@s variable int
@s uint64_t int
@s uint32_t int
@s uint16_t int
@s uint8_t char
@s int64_t int
@s int32_t int
@s int16_t int
@s int8_t char
@s intvec char
@s fs_ifdbg if
@s forVector for
@s mbool bool
@s funcsat_config int
@s forEachWatchedClause for
@s for_watchlist for
@s for_watchlist_continue for
@s for_clause for
@s for_head_tail for
@s funcsat_result int
@s hashtable int
@s vector int
@s posvec int
@s fibheap int
@s binvec_t int
@s watchlist int
@s watchlist_elt int
@s all_watches int
@s head_tail int
@s new numVars
@s uintptr_t int
@s bh_node int



@* The Client. Information in the {\tt funcsat.h} header file should give you all you
need to begin using \funcsat. We provide methods for the following:

\numberedlist

\li Creating (|funcsatInit|) and destroying (|funcsatDestroy|) a SAT solver

\li Adding instance clauses (|funcsatAddClause|)

\li Solving (|funcsatSolve|) SAT instances.

\endnumberedlist

\funcsat\ is an extensible SAT solver. This means SAT instances can be solved
under unit assumptions.

\numberedlist

\li In order to add a unit assumption, use |funcsatPushAssumption|.

\li {\bf You must check the return value}. If it returns |FS_UNSAT|, the
  assumption is not actually pushed because your problem is trivially
  unsatisfiable.

\li At this point, if you call |funcsatSolve|, the SAT instance is restricted to
  solutions where every push assumption is true.

\li In order to relieve assumptions, call |funcsatPopAssumptions|. Assumptions
are kept on a LIFO stack and {\it push} and {\it pop} have LIFO semantics.

\endnumberedlist

@* The Developer.

The second category of user is going to want to modify funcsat. What follows is
an overview of all data structures in \funcsat.


@ Vocab and data types.

\numberedlist

\li The order of inferences (implied literals) is kept in the |funcsat->trail|,
a chronological list of the current (partial) assignment.

\li The |funcsat->model| tells whether a variable is assigned given the
variable's index in the trail. A variables assignment (either True or False) is
called its {\it phase}.

\li The |funcsat->level| of each variable is the decision level at which the
variable was set: if the variable was set at decision level 0, that means it's
True for all time.

\li The |funcsat->decisions| tells whether a particular variable is a
{\it decision variable} (a choice point, a branch) or not. A variable that is
not a decision is an {\it inference}, or currently unassigned. Each inference
has a {\it reason} (a clause sometimes called an antecedent) stored in
|funcsat->reason|.

\li Each clause can either be original or learned; a learned clause is implied
by the SAT instance.

\endnumberedlist

@ Generic data types. There are some datatypes used everywhere in funcsat:

\unorderedlist

\li |variable|, |literal|: just typedefs for |unsigned| and |int|, resp.

\li |vector|: a growable, sized array of pointers

\li |clause|: a clause; or just a growable, sized array of |int|

\li |posvec|: a growable, sized array of |unsigned|

\li |mbool|: a three-value Boolean, or ``multi-Bool''


\endunorderedlist

Other available data types:
\unorderedlist

\li struct |hashtable|: just a direct chained hash table
\li |fibheap|: fibonacci heap (prio queue impl)

\endunorderedlist

I make extensive use of doubly-linked, circular lists of |clause|. Look at
its definition. The double next and prev pointers are so each clause can be in
at least two 2-watched literals lists. The way it works is described there. If
a clause is in a single doubly-linked list (such as when it is in the unit
fact lists), |clause->next|[1] and |clause->prev|[1] are both NULL.

@ If you want to add some new solver state...

The core solver state is in this file; the data type is |funcsat|. For example,
when I added phase saving I needed a new vector to store the last phase. So,

\unorderedlist
\li I added a |funcsat->phase| into |funcsat|.
\li Next, I allocated the state in |funcsatInit|
\li and free'd the state in |funcsatDestroy|
\li Finally, I added the incremental resizing of the state
in |funcsatResize|.

\endunorderedlist

{\bf Do not allocate during solving.} Allocate only during
|funcsatInit| and |funcsatResize|. Allocating during solving is bad. Of
course we have to allocate when learning a new learned clause. I'm working on
making this more efficient (by re-using space of gc'd clauses and by using a
chunked clause data structure).

@ If you want to add a new user-settable parameter...

All such parameters are part of the |funcsat_config|, which is stored
per-solver in |funcsat->conf|.

@ If you want to change the clause learning...

See the learning on page \refn{pg-clause-learn}. It's a really nasty loop that
calculates all the UIPs of the current conflict graph, or at least all the UIPs
the user wants. It does resolution to figure out which clause to learn. It does
conflict clause minimisation using Van Gelder's DFS-based minimisation
algorithm. It does only-the-fly self-subsuming resolution with each resolvent
produced during learning.

@ If you want to add a new stat...

Put it in |funcsat|. Then you'll probably want to change |funcsatPrintStats|,
too (and possibly even |funcsatPrintColumnStats|).

@ If you want to add a new argument...

Probably you should be exposing some option in |funcsat->conf|. In any case,
you need to look in <main.c> to see how getopt is used to handle the existing
options. Then hack yours in.


Create a new funcsat instance.  Funcsat is designed to be thread-safe so one
process may create as many solvers as it likes.

A good default conf is |funcsatDefaultConfig|.  You can extern it as:

    |extern funcsat_config funcsatDefaultConfig;|

@* Implementation. This file was originally not literate. I'm converting it to
literate style bit by bit. Be patient. First, the public header file is pretty
simple.

@(funcsat.h@>=
#ifndef funcsat_h_included
#define funcsat_h_included
#include <stdio.h>
#include <stdlib.h>

@<External types@>@;

@<External declarations@>@;

#endif

@ The internal header file is where our internal types and declarations go

@(funcsat_internal.h@>=
#ifndef funcsat_internal_h_included
#define funcsat_internal_h_included
#include <funcsat/vec.h>
#include <funcsat/intvec.h>
#include <funcsat/fibheap.h>

  @<Conditional macros@>@;
  @<Internal types@>@;
  @<Main \funcsat\ type@>@;
  @<Internal declarations@>@;

#endif

@  All the external types need the basic \funcsat\ types.

@<External ty...@>=
#include "funcsat/system.h"
#include "funcsat/posvec.h"
#include "funcsat/hashtable.h"


@ Next comes \funcsat! I've put all the includes here, for simplicity. Note that
this file has no {\tt main} function---we only define \funcsat's internals and
API here. See {\tt main.c} for the grungy details of exposing all of \funcsat's
options on the command-line.

It's pretty gross.

@c
#include "funcsat/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <errno.h>
#include <funcsat/hashtable.h>
#include <funcsat/system.h>
#include <zlib.h>
#include <ctype.h>


#include "funcsat.h"
#include "funcsat_internal.h"
#include "funcsat/vec_uintptr.h"

#define UNUSED(x) (void)(x)
@<Global definitions@>@;



@ I use the following macros for memory management, just because.

@<External decl...@>=

#define CallocX(ptr, n, size)                   \
  do {                                          \
    ptr = calloc(n, size);                      \
    if (!ptr) perror("calloc"), abort();        \
  } while (0);

#define MallocX(ptr, n, size)                   \
  do {                                          \
    ptr = malloc((n) * (size));                 \
    if (!ptr) perror("malloc"), abort();        \
  } while (0);

#define MallocTyX(ty, ptr, n, size)             \
  do {                                          \
    ptr = (ty) malloc((n)*(size));              \
    if (!ptr) perror("malloc"), abort();        \
  } while (0);

#define ReallocX(ptr, n, size)                                          \
  do {                                                                  \
    void *tmp_funcsat_ptr__;                                            \
    tmp_funcsat_ptr__ = realloc(ptr, (n)*(size));                       \
    if (!tmp_funcsat_ptr__)  free(ptr), perror("realloc"), exit(1);     \
    ptr = tmp_funcsat_ptr__;                                            \
  } while (0);


@*1 Top-level solver. This is the top-level solving loop. You should read
this. It's really short! it should stay that way. It should fit on a page. It
serves as a pretty easy-to-grasp example that can be referred to for writing a
custom search strategy using \funcsat\ components.

The main parameters of the solver allow configuring a wide range of search
behavior. Note how the loop runs until the solver ``runs out of resources.''
This allows one to use \funcsat\ to do cheap probing for properties, allowing
the solver to return ``unknown'' if it needs to. As another example,
|isTimeToRestart| allows one to insert a custom restart strategy.

@<Global def...@>=
funcsat_result funcsatSolve(funcsat *f)
{
  if (FS_UNSAT == (f->lastResult = startSolving(f))) goto Done;

  if (!bcpAndJail(f)) goto Unsat;

  while (!f->conf->isResourceLimitHit(f, f->conf->user)) {
    if (!bcp(f)) {
      if (0 == f->decisionLevel) goto Unsat;
      if (!analyze_conflict(f)) goto Unsat;
      continue;
    }

#if 0
    if (f->conf->gc) f->conf->sweepClauses(f, f->conf->user);
#endif

    if (f->trail.size != f->numVars &&
        f->conf->isTimeToRestart(f, f->conf->user)) {
      fslog(f, "solve", 1, "restarting\n");
      ++f->numRestarts;
      backtrack(f, 0, NULL, true);
      continue;
    }

    if (!funcsatMakeDecision(f, f->conf->user)) {
      f->lastResult = FS_SAT;
      goto Done;
    }
  }

Unsat:
  f->lastResult = FS_UNSAT;

Done:
  fslog(f, "solve", 1, "instance is %s\n", funcsatResultAsString(f->lastResult));
  assert(f->lastResult != FS_SAT || f->trail.size == f->numVars);
  finishSolving(f);
  return f->lastResult;
}

@ Some notes about the external API.

Funcsat returns results using the following datatype. The codes are the typical
codes used for SAT solvers in the SAT competition.

@<External ty...@>=
typedef enum
{
  FS_UNKNOWN = 0,
  FS_SAT     = 10,
  FS_UNSAT   = 20
} funcsat_result;

@*2 Solver type. \funcsat\ has a bunch of members.

@d Unassigned (-1)

@<Main \funcsat\ type@>=
struct funcsat_config;
struct clause_head;

struct funcsat
{
  struct funcsat_config *conf;

  clause assumptions; /* assumptions as given by the user */

  funcsat_result lastResult; /* result of the last incremental call  */

  unsigned decisionLevel; /* current decision level */

  unsigned propq; /* Unit propagation queue, which is an index into
                      |funcsat->trail|. The element pointed at by |propq| is the
                      first literal to propagate.  {\it Invariant:} If |propq|
                      is |funcsat->trail.size|, then the queue is empty. */

  clause trail; /* Current (partial) assignment stack. */

  posvec model; /* If a variable is assigned (see |funcsat->level|), contains
                   the index of the literal in the trail. \todo\ bitpack */

  clause phase; /* Stores the current, or last, phase of each variable (for
                   phase saving). */

  clause level; /* Records the decision level of each variable.  If a variable
                   is unset, its |level| is |Unassigned|. */

  posvec decisions; /* For each decision variable, this maps to its decision
                       level. A non-decision variable maps to 0. */

  struct vec_uintptr *reason; /* Maps a variable to the (index for the) |struct reason|
                         that became unit to cause a variable assignment (see
                         |funcsat->reason_infos|). The reason is |NO_REASON| if
                         the variable is a decision variable. */

  vector reason_hooks; /* Indexed by types, returns a function pointer that
                        calculates a clause given a |funcsat| and a
                        |literal|. */

  struct vec_reason_info *reason_infos; /* It is not safe to take pointers of
                                         these values because this vector can be
                                         realloc'd. */
  uintptr_t reason_infos_freelist; /* index of first free |reason_info| */

  head_tail *unit_facts;  /* Indexed by variable, each each |head_tail| list
                             points to a linked list of clauses. */
  unsigned unit_facts_size;
  unsigned unit_facts_capacity;

  head_tail *jail; /* Indexed by variable, each mapped to a |head_tail|
                      list. Each list contains only clauses currently satisfied
                      by the associated variable. */

  all_watches watches; /* Indexed from literals (using |fs_lit2idx|) into a
                          |watcherlist|. */

  vector origClauses; /* A list of all the original clauses. */
  vector learnedClauses; /* A list of all the learned clauses. */
  unsigned numVars;

#if 0
  unsigned    freelist;        /* index of first free head in |pool| */
  struct clause_head *pool;
#endif

  clause *conflictClause; /*  When unit propagation discovers a conflicting
                              clause, the clause is stashed here. */


  posvec litPos; /* During conflict analysis, the index in |funcsat->uipClause|
                    of literal |l| is |litPos.data[l]|. */
  clause uipClause; /* Working area for the clause we will eventually learn
                     during conflict analysis. */
  vector subsumed; /* list of subsumed clauses. used by clause learning. */


  
  unsigned lbdCount; /* LBD heuristic stuff -- see computeLbdScore */
  posvec lbdLevels; /* LBD heuristic stuff -- see computeLbdScore */

  
  double   claDecay; /* clause activities */
  double   claInc;
  double   learnedSizeFactor;   /* when doing clause-activity based gc, this is
                                   the initial limit for learned clauses, a
                                   fraction of the original clauses (as in
                                   minisat 2.2.0) */

  double   maxLearned;
  double   learnedSizeAdjustConfl;
  uint64_t learnedSizeAdjustCnt;
  double   learnedSizeAdjustInc;
  double   learnedSizeInc;


  
  posvec seen; /* conflict clause minimisation stuff */
  intvec analyseToClear;
  posvec analyseStack; /* dfs stack */
  posvec allLevels; /* set of the levels-of-vars that occur in the clause */


  
  double   varInc; /* dynamic variable order stuff */
  double   varDecay;

  struct bh_node *binvar_heap;  /* binary heap of variables */
  unsigned       binvar_heap_size; /* number of elements in |binvar_heap| */
  unsigned      *binvar_pos; /* locations of each var in the heap */

  
  int64_t  lrestart; /* luby restart stuff -- cribbed from picosat */
  uint64_t lubycnt;
  uint64_t lubymaxdelta;
  bool   waslubymaxdelta;

  
  double pct; /* conservative estimate of space explored. compatible with restarts. */
  double pctdepth;
  int correction;

  

  struct drand48_data *rand;



  uint64_t numSolves;   /* Number of calls to ::funcsatSolve */


  uint64_t numOrigClauses;   /* How many original clauses */


  uint64_t numLearnedClauses;   /* How many learned clauses */


  uint64_t numSweeps;   /* How many times we deleted learned clauses */


  uint64_t numLearnedDeleted;   /* How many learned clauses we deleted */


  uint64_t numLiteralsDeleted;  /*
  How many literals deleted from learned clauses as we simplify them */


  uint64_t numProps;  /* How many unit propagations */
  uint64_t numUnitFactProps;

  uint64_t numJails;


  uint64_t numConflicts;  /* How many conflicts */


  uint64_t numRestarts;  /* How many restarts */


  uint64_t numDecisions;  /* How many decisions */


  uint64_t numSubsumptions;  /*
  How many times a clause was simplified due to self-subsuming resolution */


  uint64_t numSubsumedOrigClauses;  /*
  How many subsumption simplifications were done for original clauses */

  uint64_t numSubsumptionUips;
};


@*2 Adding clauses. The main solver interface is just a few functions.


@<Global def...@>=
funcsat_result funcsatAddClause(funcsat *f, clause *c)
{
  funcsatReset(f);
  variable k;
  variable maxVar = 0;

  for (k = 0; k < c->size; k++) {
    variable v = fs_lit2var(c->data[k]);
    maxVar = v > maxVar ? v : maxVar;
  }
  funcsatResize(f, maxVar);

  if (c->size > 1) {
    unsigned size = c->size;
    sortClause(c);
    literal *i, *j, *end;
    /* i is current, j is target */
    for (i = j = (literal *) c->data, end = i + c->size; i != end; i++) {
      literal p = *i, q = *j;
      if (i != j) {
        if (p == q) {           /* duplicate literal */
          size--;
          continue;
        } else if (p == -q) {   /* trivial clause */
          clauseDestroy(c);
          goto Done;
        } else *(++j) = p;
      }
    }
    c->size = size;
  }

  fs_ifdbg(f, "solve", 2) {
    fslog(f, "solve", 2, "original clause: ");
    dimacsPrintClause(fs_dbgout(f), c);
    fprintf(fs_dbgout(f), "\n");
  }

  c->lbdScore = LBD_SCORE_MAX;         /* like in glucose */
  c->isLearnt = false;
  c->isReason = false;
  f->conf->bumpOriginal(f, c);
  funcsat_result clauseResult = addClause(f, c);
  if (f->lastResult != FS_UNSAT) {
    f->lastResult = clauseResult;
  }
  vectorPush(&f->origClauses, c);
  ++f->numOrigClauses;
Done:
  return f->lastResult;
}

funcsat_result addClause(funcsat *f, clause *c)
{
  funcsat_result result = FS_UNKNOWN;
  assert(f->decisionLevel == 0);
  c->isLearnt = false;
  if (c->size == 0) {
    f->conflictClause = c;
    result = FS_UNSAT;
  } else if (c->size == 1) {
    mbool val = funcsatValue(f, c->data[0]);
    if (val == false) {
      f->conflictClause = c;
      result = FS_UNSAT;
    } else {
      if (val == unknown) {
	trailPush(f, c->data[0], reason_info_mk(f, c));
        head_tail_add(&f->unit_facts[fs_lit2var(c->data[0])], c);
      }
    }
  } else addWatch(f, c);
  return result;
}


@*2 Unit assumptions. \funcsat\ supports solving under any number of unit
assumptions, like similar solvers such as \minisat\ and \picosat. But our
interface behaves a bit differently.

\numberedlist

\li |funcsatPushAssumption(f,x)| adds a unit assumption |x|. This means that
every time |funcsatSolve| is called from now on (unless you manipulate the
assumptions again), all satisfying assignments must have |x| set to true. If the
solver returns |FS_UNSAT|, that means there exists no satisfying assignment
where |x| holds. It says {\it nothing} about satisfying assignments where |-x|
holds.

{\bf You must check the return value of |funcsatPushAssumption|!} If it ever
returns |FS_UNSAT|, it means the assumption was false and you should take
appropriate action before solving.

\li |funcsatPopAssumption| will remove the last unit assumption pushed (call it
|x|). All subsequent calls (unless you manipulate the assumptions again) will
not be sensitive to any particular assignment to |x|.

\li Therefore, assumptions are ordered in a LIFO stack.

\li Unlike \picosat, the solver {\it never} implicitly removes
assumptions after a call to |funcsatSolve|.

\endnumberedlist

@<Global def...@>=
funcsat_result funcsatPushAssumption(funcsat *f, literal p)
{
  if(p == 0) {
    //This is a hack to get the assumption stacks of bitfunc and funcsat to be the same size;
    clausePush(&f->assumptions, 0);
    return FS_UNKNOWN;
  }
      
  f->conf->minimizeLearnedClauses = false;
  backtrack(f, 0UL, NULL, true);
  /* in a weird case, there are no clauses but some assumptions, so resize */
  funcsatResize(f, fs_lit2var(p));
  if (funcsatValue(f, p) == false) {
    return FS_UNSAT;
  } else if (funcsatValue(f, p) == unknown) {
    /* copy over assumptions that matter */
    clausePush(&f->assumptions, p);
    trailPush(f, p, NO_REASON);
  } else {
    clausePush(&f->assumptions, 0);
  }
  return FS_UNKNOWN;
}

funcsat_result funcsatPushAssumptions(funcsat *f, clause *c) {
  for(unsigned i = 0; i < c->size; i++) {
    if(funcsatPushAssumption(f, c->data[i]) == FS_UNSAT) {
      funcsatPopAssumptions(f, i);
      return FS_UNSAT;
    }
  }
  return FS_UNKNOWN;
}

@

@<Global def...@>=
void funcsatPopAssumptions(funcsat *f, unsigned num) {
 
  head_tail facts;
  head_tail_clear(&facts);

  assert(num <= f->assumptions.size);

  backtrack(f, 0, NULL, true);

  for (unsigned i = 0; i < num; i++) {
    literal p = clausePop(&f->assumptions);
    if (p == 0) return;

    literal t = trailPop(f, &facts);
  
    while (p != t) {
      t = trailPop(f, &facts);
    }
  }

  restore_facts(f, &facts);
}

@ At any point during solving, we can forcibly reset the solver state using this
function. It does {\it not} remove any assumptions.

@<Global def...@>=

void funcsatReset(funcsat *f)
{
  f->conflictClause = NULL;
  backtrack(f, 0UL, NULL, true);
  f->propq = 0;
  f->lastResult = FS_UNKNOWN;
}





@*1 Clauses. Clauses are represented thusly. Well, this is the external clause
type. Soon, it will not be used internally, but right now it is.

@<External ty...@>=
typedef struct clause
{
  literal *data;

  uint32_t size;

  uint32_t capacity;

  uint32_t isLearnt : 1;
  uint32_t isReason : 1;
  uint32_t is_watched : 1; /* in the watched literals for the first 2 literals of the clause */
  uint32_t lbdScore : 29;

  double activity;

  struct clause *nx; /*
  \todo change to clause index, for ad-hoc lists of clauses w/ o allocation */
} clause;

@ Public functions.

@<External decl...@>=
clause *clauseAlloc(uint32_t capacity); /* creates empty clause */

void clauseInit(clause *v, uint32_t capacity); /* initializes manually malloc'd clause */

void clauseDestroy(clause *); /*
  Frees the underlying literal buffer and resets clause metadata. Does not free
  the given pointer. */

void clauseFree(clause *);/* Calls |clauseDestroy| then frees the clause. */

void clauseClear(clause *v); /* 
 Clear the contents of the clause. Does not touch reference count or list
 links. */

void clausePush(clause *v, literal data); /*
 Allocates space (if necessary) for another literal; then appends the given
 literal. */

void clausePushAt(clause *v, literal data, uint32_t i);
void clauseGrowTo(clause *v, uint32_t newCapacity);
literal clausePop(clause *v);
literal clausePopAt(clause *v, uint32_t i);
literal clausePeek(clause *v);

void clauseCopy(clause *dst, clause *src); /*
 Copies all the literals and associated metadata, but the |dst| reference count
 is 1.
 */
void dimacsPrintClause(FILE *f, clause *);

#define forClause(elt, vec) for (elt = (vec)->data; elt != (vec)->data + (vec)->size; elt++)
#define for_clause(elt, cl) for (elt = (cl)->data; elt != (cl)->data + (cl)->size; elt++)



@

@<Internal decl...@>=


void fs_clause_print(funcsat *f, FILE *out, clause *c);
void dimacsPrintClauses(FILE *f, vector *);




@*2 Head-tail lists. Sometimes we use a head-tail representation for lists of
clauses. The |head_tail| structure points at the initial clause and the last
clause. The clauses in between the head and the tail are found by walking each
|clause->nx| pointer. Using these scheme we can append two such lists in
constant time.

@<Internal ty...@>=
typedef struct head_tail
{
  clause *hd;
  clause *tl;
} head_tail;

@
@<Global def...@>=
static inline void head_tail_clear(head_tail *ht) {
  ht->hd = ht->tl = NULL;
}

static inline bool head_tail_is_empty(head_tail *ht) {
  return ht->hd == NULL && ht->tl == NULL;
}

static inline void head_tail_append(head_tail *ht1, head_tail *ht2) { /*
  |ht1| = |ht1| with |ht2| appended */
  if (!head_tail_is_empty(ht2)) {
    if (head_tail_is_empty(ht1)) {
      ht1->hd = ht2->hd;
    } else {
      assert(ht1->hd);
      assert(!ht1->tl->nx);
      ht1->tl->nx = ht2->hd;
    }
    ht1->tl = ht2->tl;
  }
}


static inline void head_tail_add(head_tail *ht1, clause *c) { /*
  |ht1| = |ht1| with clause |c| appended */
  (c)->nx = ht1->hd;
  ht1->hd = (c);
  if (!ht1->tl) {
    ht1->tl = (c); }
}

@
@<Internal decl...@>=
static inline void head_tail_clear(head_tail *ht);
static inline bool head_tail_is_empty(head_tail *ht);
static inline void head_tail_append(head_tail *ht1, head_tail *ht2);
static inline void head_tail_add(head_tail *ht1, clause *c);


@ Iterating over a |head_tail| list is slightly tricky. The common case is
iterating and doing something to each element, without removing it. That's
pretty easy.

But often we want to preserve some elements in the list and delete others, while
we iterate. That's what |head_tail_iter_rm| is for. It deletes the |curr|
element from the list and sets |curr->nx| to |NULL|. The iterator can therefore
detect that |curr| has been deleted (by testing |curr->nx|) and not update the
|prev| pointer when that happens.

@d for_head_tail(ht, prev, curr, next) /* iterate over all the clauses in the list */
    for (prev = NULL, curr = (ht)->hd, ((curr) ? next = (curr)->nx : 0);@/
         (curr);@/
         ((curr)->nx ? prev = (curr) : 0), curr = (next), ((curr) ? next = (curr)->nx : 0))

@d head_tail_iter_rm(ht, prev, curr, next) /* delete |curr| from iteration */
    if ((ht)->hd == (curr)) { /* fix up |hd| and |tl| fields */
      (ht)->hd = (next);
    }
    if ((ht)->tl == (curr)) {
      (ht)->tl = (prev);
    }
    if (prev) {
      (prev)->nx = (next);
    }
    (curr)->nx = NULL;

@c


@

@<Global def...@>=
inline void head_tail_print(funcsat *f, FILE *out, head_tail *l)
{
  if (!out) out = stderr;
  if (l->hd) {
    clause *p, *c, *nx;
    for_head_tail (l, p, c, nx) {
      if (!c->nx && l->tl != c) {
        fprintf(out, "warning: tl is not last clause\n");
      }
      fs_clause_print(f, out, c);
      fprintf(out, "\n");
    }
  } else if (l->tl) {
    fprintf(out, "warning: hd unset but tl set!\n");
  }
}

@

@<Internal decl...@>=
extern void head_tail_print(funcsat *f, FILE *out, head_tail *l);


@*1 BCP. How does \funcsat's BCP work? Here's the signature.

@<Internal decl...@>=
bool bcp(funcsat *f);

@ First, there's this notion of a {\it propagation queue} (|funcsat->propq|). It
is an index into the trail. It is the index of the earliest literal on the trail
that should be examined by BCP. BCP examines each literal, beginning with the
first in the propagation queue, until there are no more inferences or it finds a
conflict.

If we discover a conflict, |bcp| returns |false| and sets
|funcsat->conflictClause| to the clause that is false. Otherwise it returns
|true|.

@<Global def...@>=
bool bcp(funcsat *f)
{
  bool isConsistent = true;

  @<BCP clauses@>@;

bcp_conflict:
  return isConsistent;
}

@ We put learned clauses in our normal watchlists. Each clause knows whether
it's learned or not.

\numberedlist

\li Con: when garbage collection is done, the learned watchlists are entirely
recreated. This is easiest because the GC can sort all the clauses by score,
delete the worst half, and recreate the watchlists. How could this be resolved?

  \numberedlist

  \li Can I add some information to the clauses themselves and GC them during
  BCP? (This might be bad because it might make BCP slow. But let's have some
  faith in branch prediction on processors, shall we?)

  \li At first, I'll ignore cleaning learned clauses. Next, I'll do a special
  walk over the whole watcher list and clean out certain of the learned
  clauses. Next, I'll think of some queue-of-craptastry in order to drop clauses
  more dynamically and in a fine-grained way.

  \endnumberedlist

\li Pro: every single watched literal operation does not have to be duplicated
for the original and learned lists, in the other representation. This is
annoying.

\endnumberedlist

Here we are using a representation inspired by the ``Cache-conscious SAT'' paper.

The watched literal structure is indexed by literals. At each index is stored a
|watchlist|, which points to a number of |watchlist_elt|s. A |watchlist_elt| has
two different forms, depending on the clause:

\numberedlist

\li For large clauses: a literal from the clause (|lit|), clause index
(|clauseidx|). (This is different from the paper. I can't figure out how they
managed to keep the {\it first} literal in the |watchlist_elt| at all times!)

\li For binary clauses: the other literal (|lit|), clause index
(|clauseidx|). (This situation is different from the paper; there the clause
index was unused. We need it because we always store a clause index to each
clauses that is a {\it reason} for an inference.)

\endnumberedlist

@<Internal ty...@>=
struct watchlist_elt
{
  literal   lit;
  clause    *cls;
};


@ Here's the |watchlist|, which represents the list of clauses associated with a
single watched literal. Each |watchlist| stores a small number of clauses in its
head, because often the watch lists are small. If the watch list is small, we
don't need to fetch anything else from memory in order to propagate this watch
list.

I chose a size of 12 for |WATCHLIST_HEAD_SIZE_MAX| because I felt like it. And
because it was in the paper.

@<Internal ty...@>=
#ifndef WATCHLIST_HEAD_SIZE_MAX
#  define WATCHLIST_HEAD_SIZE_MAX 4
#endif

struct watchlist
{
  uint32_t size;               /* of |watchlist->elts + watchlist->rest| */
  uint32_t capacity;           /* of |watchlist->rest| */

  struct watchlist_elt elts[WATCHLIST_HEAD_SIZE_MAX];
  struct watchlist_elt *rest;
};

@ Traversing the watchlists. Using these macros makes the |bcp| code a bit
cleaner.

@d watchlist_next_elt(elt, wl)
  ((elt) + 1 == (wl)->elts + WATCHLIST_HEAD_SIZE_MAX ? elt = (wl)->rest : (elt)++)
@d for_watchlist(elt, dump, wl) /* |elt| ptr has to transition from head list to rest */
  for (elt = dump = wl->elts;
       (watchlist_is_elt_in_head(elt, wl)
          ? elt < watchlist_head_size_ptr(wl)
          : elt < watchlist_rest_size_ptr(wl));
       watchlist_next_elt(elt, wl))

@ The |all_watches| is the vector of all the watcher lists for every literal.

@<Internal ty...@>=
typedef struct all_watches
{
  struct watchlist *wlist; /* indexed by |fs_lit2idx(i)| */
  unsigned size;
  unsigned capacity;
} all_watches;

@ Initializing.

@<Global def...@>=
static inline void all_watches_init(funcsat *f)
{
  CallocX(f->watches.wlist, 1<<7, sizeof(*f->watches.wlist));
  f->watches.size = 2;
  f->watches.capacity = 1<<7;
}

@ 

@<Global def...@>=
static inline void all_watches_destroy(funcsat *f)
{
  free(f->watches.wlist);
}

@ These used to be macros, but then GDB on OS X wouldn't let me call them no
matter how many funny {\tt -g} options I passed to gcc. So I made them
functions.

@<Internal decl...@>=
static inline bool watchlist_is_elt_in_head(struct watchlist_elt *elt, struct watchlist *wl)
{
  return (elt >= wl->elts && elt < (wl)->elts + WATCHLIST_HEAD_SIZE_MAX);
}
static inline uint32_t watchlist_head_size(struct watchlist *wl)
{
  return ((wl)->size > WATCHLIST_HEAD_SIZE_MAX ? WATCHLIST_HEAD_SIZE_MAX : (wl)->size);
}
static inline struct watchlist_elt *watchlist_head_size_ptr(struct watchlist *wl)
{
  return ((wl)->elts + watchlist_head_size(wl));
}
static inline uint32_t watchlist_rest_size(struct watchlist *wl)
{
  return ((wl)->size > WATCHLIST_HEAD_SIZE_MAX ? (wl)->size - WATCHLIST_HEAD_SIZE_MAX : 0);
}
static inline struct watchlist_elt *watchlist_rest_size_ptr(struct watchlist *wl)
{
  return ((wl)->rest + watchlist_rest_size(wl));
}


@

@<Internal decl...@>=
static inline void all_watches_init(funcsat *f);
static inline void all_watches_destroy(funcsat *f);

@

@<Incrementally resize internal...@>=
  if (f->watches.capacity <= f->watches.size+2) {
    while (f->watches.capacity <= f->watches.size) {
      f->watches.capacity = f->watches.capacity*2+2;
    }
    ReallocX(f->watches.wlist, f->watches.capacity, sizeof(*f->watches.wlist));
  }

  @<Initialize wlist bucket at |f->watches.size|@>@;
  f->watches.size++;
  @<Initialize wlist bucket at |f->watches.size|@>@;
  f->watches.size++;
  assert(f->watches.size <= f->watches.capacity);

@

@<Initialize wlist bucket at |f->watches.size|@>=
f->watches.wlist[f->watches.size].size = 0;
f->watches.wlist[f->watches.size].capacity = 0;
for (uint32_t i = 0; i < WATCHLIST_HEAD_SIZE_MAX; i++) {
  f->watches.wlist[f->watches.size].elts[i].lit = 0;
  f->watches.wlist[f->watches.size].elts[i].cls = NULL;
}
f->watches.wlist[f->watches.size].rest = NULL;
assert(0 == watchlist_head_size(&f->watches.wlist[f->watches.size]));
assert(0 == watchlist_rest_size(&f->watches.wlist[f->watches.size]));


@ Destroy the watcher lists.

@<Destroy internal data...@>=

for (variable i = 1; i <= f->numVars; i++) {
  free(f->watches.wlist[fs_lit2idx((literal)i)].rest);
  free(f->watches.wlist[fs_lit2idx(-(literal)i)].rest);
}
free(f->watches.wlist);


@ 
@<Global def...@>=
static int compare_pointer(const void *x, const void *y)
{
  uintptr_t xp = (uintptr_t)*(clause **)x;
  uintptr_t yp = (uintptr_t)*(clause **)y;
  if (xp < yp) return -1;
  else if (xp > yp) return 1;
  else return 0;
}


@ 
@<Internal decl...@>=
static int compare_pointer(const void *x, const void *y);


@ Our invariants should be explicit.
@<Global def...@>=
static inline void watchlist_check(funcsat *f, literal l)
{
  literal false_lit = -l;

  struct watchlist *wl = &f->watches.wlist[fs_lit2idx(l)];
  struct watchlist_elt *elt, *dump;

  vector *clauses = vectorAlloc(wl->size);
  for_watchlist (elt, dump, wl) {
    clause *c = elt->cls;
    vectorPush(clauses, c);
    literal *chkl; bool chk_in_cls = false;
    for_clause (chkl, c) { /* ensure |elt->lit| is in |c| */
      if (*chkl == elt->lit) {
        chk_in_cls = true;
        break;
      }
    }
    assert(chk_in_cls && "watched lit not in clause");

    assert((c->data[0] == false_lit || c->data[1] == false_lit) &&
           "watched lit not in first 2");

    uint32_t num_not_false = 0; /* not-false literals are ``safe'' to watch */
    for (unsigned i = 0; i < c->size; i++) {
      if (tentativeValue(f, c->data[i]) != false) {
        num_not_false++;
        if (funcsatValue(f, c->data[i]) == true) {
          num_not_false = 0;
          break;
        }
      }
    }
    if (num_not_false >= 1) { /* ensure we're watching ``safe'' literals */
      assert((tentativeValue(f, c->data[0]) != false ||
              tentativeValue(f, c->data[1]) != false) &&
               "watching bad literals");
      if (num_not_false >= 2) {
        assert(tentativeValue(f, c->data[0]) != false &&
               tentativeValue(f, c->data[1]) != false &&
               "watching bad literals");
      }
    }
  }
  
  qsort(clauses->data, clauses->size, sizeof(clause*), compare_pointer);
  
  for (unsigned i = 0, j = 1; j < clauses->size; i++, j++) {
    /* find duplicate clauses */
    assert(clauses->data[i] != clauses->data[j] && "duplicate clause");
  }
  vectorDestroy(clauses), free(clauses);
}



@ We can check all the watchlists at once.
@<Global def...@>=

static inline void watches_check(funcsat *f)
{
  for (variable v = 1; v <= f->numVars; v++) {
    literal pos = (literal)v;
    literal neg = -(literal)v;
    watchlist_check(f, pos);
    watchlist_check(f, neg);
  }
}

@ 
@<Internal decl...@>=
static inline void watchlist_check(funcsat *f, literal l);
static inline void watches_check(funcsat *f);


@ With all this insane machinery we can how handle discovering unit inferences
and propagating them.

@<BCP cl...@>=
while (f->propq < f->trail.size) {
  literal p = f->trail.data[f->propq];
#ifndef NDEBUG
  watchlist_check(f, p);
#endif
  fs_ifdbg (f, "bcp", 3) { fslog(f, "bcp", 3, "bcp on %i\n", p); }
  const literal false_lit = -p;

  struct watchlist *wl = &f->watches.wlist[fs_lit2idx(p)];
  struct watchlist_elt *elt, *dump;
  uint32_t new_size = 0;

  ++f->numProps;
  dopen(f, "bcp");

  for_watchlist (elt, dump, wl) {
    clause *c = elt->cls;
    literal otherlit; /* the other watched lit in |c| */
    mbool litval;
    fs_ifdbg (f, "bcp", 9) {
      fslog(f, "bcp", 9, "visit ");
      fs_clause_print(f, fs_dbgout(f), c);
      fprintf(fs_dbgout(f), " %p\n", c);
    }
    assert(false_lit == c->data[0] || false_lit == c->data[1]);
    assert(!c->nx);
    @<Check |elt->lit| for quick SAT clause@>@;
    @<Ensure false lit...@>@;
    @<Look for new, unfalsified literal to watch, removing |elt| if found@>@;

    otherlit = c->data[0];
    litval = funcsatValue(f, otherlit);
    if (litval == true) goto watch_continue;
    if (litval == false) {
      @<Record conflict and return@>@;
    } else {
      @<Record new unit inference@>@;
    }
  watch_continue:
    *dump = *elt, watchlist_next_elt(dump, wl); /* keep |elt| in this |wl| */
    new_size++;
  skip_watchelt_copy:;
  }
  f->propq++;
  wl->size = new_size;
  dclose(f, "bcp");
}

@ The |watchlist_elt| is in the cache line. We have stored a literal from the
clause in there so that we can quickly test whether it's true; if it is, we can
ignore the current clause and go on to the next one.

@<Check |elt->lit| for quick SAT...@>=
    if (funcsatValue(f, elt->lit) == true) goto watch_continue; /*
    If |elt->lit| is |true|, clause already satisfied */


@ Just for organization's sake, we put the watched literal (which is false) into
|c->data[1]|.

@<Ensure false literal is in |c->data[1]|@>=
if (c->data[0] == false_lit) {
  literal tmp = c->data[0];
  elt->lit = c->data[0] = c->data[1], c->data[1] = tmp;
  assert(c->data[1] == false_lit);
  fs_ifdbg (f, "bcp", 10) {
    fslog(f, "bcp", 9, "swapped ");
    fs_clause_print(f, fs_dbgout(f), c), fprintf(fs_dbgout(f), "\n"); }
}
assert(c->data[1] == false_lit);


@ Our goal is to discover whether the clause |c| is {\it unit} or not. To do
that we check the clause (excepting the two first literals) for any literal that
is not false. If we fail to find one, since we have ensured that the first
literal is in |c->data[1]| (see |@<BCP cl...@>|), the clause is unit or false.

@<Look for new, unfalsified...@>=
for (literal *l = c->data + 2; l != c->data + c->size; l++) {
  mbool v = funcsatValue(f, *l);
  if (v != false) {
    c->data[1] = *l, *l = false_lit;
    watch_l1(f, c);
    fs_ifdbg (f, "bcp", 9) {
      fslog(f, "bcp", 9, "moved ");
      fs_clause_print(f, fs_dbgout(f), c), NEWLINE(fs_dbgout(f));
    }
    elt->cls = NULL; elt->lit = 0;
    goto skip_watchelt_copy;
  }
}

@ Recording the conflict should be straightforward, but we have to be careful
about the clauses after the conflict clause in this |watchlist|.  The loop below
ensures they're copied back into the current |watchlist| before we stop BCP.

|for_watchlist_continue| simply omits the initialization part of |for_watchlist|,
but is otherwise identical. At this point we have found a false clause (a
conflict) and need to make sure not to drop the other watched clauses down the
disposal.

@d for_watchlist_continue(elt, dump, wl)
  for (;
       (watchlist_is_elt_in_head(elt, wl)
          ? elt < watchlist_head_size_ptr(wl)
          : elt < watchlist_rest_size_ptr(wl));
       watchlist_next_elt(elt, wl))

@<Record conflict...@>=
isConsistent = false;
f->conflictClause = c;
for_watchlist_continue (elt, dump, wl) { /* save rest of watched clauses */
  *dump = *elt, watchlist_next_elt(dump, wl);
  new_size++;
}
wl->size = new_size;
dclose(f, "bcp");
goto bcp_conflict;

@ It's simple, really.

@<Record new unit...@>=
fslog(f, "bcp", 2, " => %i (%s:%d)\n", otherlit, __FILE__, __LINE__);
trailPush(f, otherlit, reason_info_mk(f, c));
f->conf->bumpUnitClause(f, c);

@ We have a few helpers for watching literals 0 and 1 of a clause.

@<Global def...@>=
static inline struct watchlist_elt *watch_lit(funcsat *f, struct watchlist *wl, clause *c)
{
  UNUSED(f);
  struct watchlist_elt *ret;
  if (watchlist_head_size(wl) < WATCHLIST_HEAD_SIZE_MAX) {
    assert(watchlist_rest_size(wl) == 0);
    ret = &wl->elts[watchlist_head_size(wl)];
    wl->elts[watchlist_head_size(wl)].cls = c;
  } else {
    assert(watchlist_head_size(wl) >= WATCHLIST_HEAD_SIZE_MAX);
    @q/*@>@<Allocate and/or grow |wl|'s watchlist element capacity@>@;@q*/@>
    ret = &wl->rest[watchlist_rest_size(wl)];
    wl->rest[watchlist_rest_size(wl)].cls = c;
  }
  wl->size++;
  return ret;
}
static inline void watch_l0(funcsat *f, clause *c)
{
  struct watchlist *wl = &f->watches.wlist[fs_lit2idx(-c->data[0])];
  struct watchlist_elt *elt = watch_lit(f, wl, c);
  elt->lit = (c->size == 2 ? c->data[1] : c->data[0]);
}
static inline void watch_l1(funcsat *f, clause *c)
{
  struct watchlist *wl = &f->watches.wlist[fs_lit2idx(-c->data[1])];
  struct watchlist_elt *elt = watch_lit(f, wl, c);
  elt->lit = (c->size == 2 ? c->data[1] : c->data[0]);
}

@

@<Allocate and/or grow |wl|'s watchlist element capacity@>=
if (wl->capacity > 0) {
    if (watchlist_rest_size(wl) >= wl->capacity) {
      ReallocX(wl->rest, wl->capacity*2, sizeof(*wl->rest));
      wl->capacity *= 2;
    }
} else {
  CallocX(wl->rest, 8, sizeof(*wl->rest));
  wl->capacity = 8;
}

@

@<Internal decl...@>=
static inline void watch_l0(funcsat *f, clause *c);
static inline void watch_l1(funcsat *f, clause *c);

@ When a clause is initially added, we watch it with |addWatch|. This ensures an
invariant holds (see |makeWatchable|) and then adds the clause into the watcher
lists.

@d NEWLINE(s) fprintf((!(s) ? stderr : (s)), "\n")

@<Global def...@>=
static inline void addWatch(funcsat *f, clause *c)
{
  makeWatchable(f, c);
  addWatchUnchecked(f, c);
}

static inline void addWatchUnchecked(funcsat *f, clause *c)
{
  fslog(f, "bcp", 1, "watching %li and %li in ", c->data[0], c->data[1]);
  fs_ifdbg (f, "bcp", 1) {fs_clause_print(f, fs_dbgout(f), c), NEWLINE(fs_dbgout(f));}
  assert(c->size > 1);
  watch_l0(f, c);
  watch_l1(f, c);
  c->is_watched = true;
}

@ A clause which is about we be watched must obey an invariant: that the first
(up to) two literals in the clause must be not false, if there are any such
literals.

@<Global def...@>=
static inline void makeWatchable(funcsat *f, clause *c)
{
  for (variable i = 0, j = 0; i < c->size && j < 2; i++) {
    mbool v = funcsatValue(f, c->data[i]);
    if (v != false && i != j) {
      literal tmp = c->data[j];
      c->data[j] = c->data[i], c->data[i] = tmp;
      j++;
    }
  }
}

@ 

@<Internal decl...@>=
static inline void addWatch(funcsat *f, clause *c);
static inline void addWatchUnchecked(funcsat *f, clause *c);
static inline void makeWatchable(funcsat *f, clause *c);

@

@<Global def...@>=
static void fs_watches_print(funcsat *f, FILE *out, literal p)
{
  if (!out) out = stderr;
  fprintf(out, "watcher list for %i:\n", p);
  struct watchlist *wl = &f->watches.wlist[fs_lit2idx(p)];
  struct watchlist_elt *elt, *dump;
  for_watchlist (elt, dump, wl) {
    struct clause *c = elt->cls;
    if (c) {
      fprintf(out, "[%i", elt->lit);
      bool in_clause = false;
      literal *q;
      for_clause (q, c) {
        if (*q == elt->lit) {
          in_clause = true; break;
        }
      }
      if (!in_clause) {
        fprintf(out, " oh noes!");
      }
      fprintf(out, "] ");
      fs_clause_print(f, out, c), NEWLINE(out);
    } else {
      fprintf(out, "[EMPTY SLOT]\n");
    }
  }
}


@*2 Reasons. We allow dynamic calculation of reason clauses.

@d NO_REASON UINTPTR_MAX

@<External ty...@>=
enum reason_ty
{
  REASON_CLS_TY
};

@ Reasons are represented internally with this |struct|. When the |reason_info|
is free, the |ty| is the index of the next |reason_info| in the
freelist. Otherwise it indicates the type of the |reason_info| (see |enum
reason_ty|).

@<Internal ty...@>=
struct reason_info
{
  uintptr_t ty;                       /* |enum reason_ty|, but can be others too */
  clause *cls;
};
#include "funcsat/vec_reason_info.h"


@ Getting a reason can now invoke whatever frontend is driving \funcsat, if that
frontend will dynamically calculate a reason clause.

@<Global def...@>=
static inline clause *getReason(funcsat *f, literal l)
{
  uintptr_t reason_idx = f->reason->data[fs_lit2var(l)];
  if (reason_idx != NO_REASON) {
    struct reason_info *r = reason_info_ptr(f, reason_idx);
    if (r->ty == REASON_CLS_TY || r->cls) return r->cls;
    clause *(*get_reason_clause)(funcsat *, literal) = f->reason_hooks.data[r->ty];
    return r->cls = get_reason_clause(f, l);
  } else
    return NULL;
}


@ The |reason| and |reason_hooks| need to be initialized.

@<Initialize func...@>=
f->reason = vec_uintptr_init(2);
vec_uintptr_push(f->reason, NO_REASON);
vectorInit(&f->reason_hooks, 2);

@ Clients should use this to add a hook for generating reasons.
@<Global def...@>=
void funcsatAddReasonHook(funcsat *f, uintptr_t ty, clause *(*hook)(funcsat *f, literal l))
{
  vectorPushAt(&f->reason_hooks, hook, ty);
}

@ Declaration.
@<External decl...@>=
void funcsatAddReasonHook(funcsat *f, uintptr_t ty, clause *(*hook)(funcsat *f, literal l));

@ When an inference is discovered, we need to set its reason using a fresh
|reason_info|. But of course we shouldn't allocate it. That would slow down
solving. We have a freelist of |reason_info|s and we pick the next free one.

@d reason_info_ptr(f,i) (&((f)->reason_infos->data[i]))

@<Global def...@>=
static inline uintptr_t reason_info_mk(funcsat *f, clause *cls)
{
  assert(f->reason_infos_freelist < f->reason_infos->size);
  uintptr_t ret = f->reason_infos_freelist;
  struct reason_info *r = reason_info_ptr(f, ret);
  f->reason_infos_freelist = r->ty;

  r->ty = REASON_CLS_TY;
  r->cls = cls;
  return ret;
}

@ To reclaim the |reason_info|, we assign it to the head of the freelist and its
next to the current freelist head.

@d reason_info_idx(f,r) ((r) - (f)->reason_infos->data)

@<Global def...@>=
static inline void reason_info_release(funcsat *f, uintptr_t ri)
{
  struct reason_info *r = reason_info_ptr(f,ri);
  r->ty = f->reason_infos_freelist; /* |ty| becomes the next index */
  f->reason_infos_freelist = ri;
}

@ Allocate and initialize the freelist.

@<Initialize func...@>=
f->reason_infos_freelist = UINT_MAX; /* empty */
f->reason_infos = vec_reason_info_init(2);
f->reason_infos->size = 0;


@ Just grow and add the new elements to the freelist.
@<Resize internal...@>=
vec_reason_info_grow_to(f->reason_infos, numVars);
for (uintptr_t i = f->reason_infos->size; i < numVars; i++) {
  reason_info_release(f,i);
}
f->reason_infos->size = numVars;

@ For each new variable, push that it has no reason.
@<Incrementally resize...@>=
vec_uintptr_push(f->reason, NO_REASON);

@ Destroy.
@<Destroy internal...@>=
vec_uintptr_destroy(f->reason);
vec_reason_info_destroy(f->reason_infos);
vectorDestroy(&f->reason_hooks);

@*2 Trail. The trail is managed pretty simply with |trailPush| and |trailPop|.

|trailPush| is for adding a new literal to the trail (inferred or decided).

@<Global def...@>=
void trailPush(funcsat *f, literal p, uintptr_t reason_info_idx)
{
  variable v = fs_lit2var(p);
#ifndef NDEBUG
  if (f->model.data[v] < f->trail.size) {
    assert(f->trail.data[f->model.data[v]] != p);
  }
#endif
  clausePush(&f->trail, p);
  f->model.data[v]  = f->trail.size-(unsigned)1;
  f->phase.data[v] = p;
  f->level.data[v]  = (literal)f->decisionLevel;
  
  if (reason_info_idx != NO_REASON) {
    struct reason_info *r = reason_info_ptr(f, reason_info_idx);
    if (r->ty == REASON_CLS_TY) {
      r->cls->isReason = true;
    }
    f->reason->data[v] = reason_info_idx;
  }
}

@ |trailPop| is for removing the last pushed literal from the trail. The
|literal| is returned.

In order to pop the trail, you usually want to make sure you don't lose any
facts we have around for the literal you're popping. If that is true, supply a
list to merge with. If facts is NULL, any facts are released.

@<Global def...@>=
literal trailPop(funcsat *f, head_tail *facts)
{
  literal p = clausePop(&f->trail);
  variable v = fs_lit2var(p);
  uintptr_t reason_ix;
  if (f->unit_facts[v].hd && facts) {
    head_tail_append(facts, &f->unit_facts[v]);
    head_tail_clear(&f->unit_facts[v]);
  }

#if 0
  exonerateClauses(f, v);
#endif

  if (f->decisions.data[v] != 0) {
    f->decisionLevel--;
    f->decisions.data[v] = 0;
  }
  /* no need to clear model */
  /* |f->model.data[v] = 0;| */
  f->level.data[v] = Unassigned;
  reason_ix = f->reason->data[v];
  if (reason_ix != NO_REASON) {
    struct reason_info *r = reason_info_ptr(f, reason_ix);
    if (r->ty == REASON_CLS_TY) {
      r->cls->isReason = false;
    }
    reason_info_release(f, reason_ix);
    f->reason->data[v] = NO_REASON;
  }
  if (f->propq >= f->trail.size) {
    f->propq = f->trail.size;
  }
  if (!bh_is_in_heap(f,v)) {
#ifndef NDEBUG
    bh_check(f);
#endif
    bh_insert(f,v);
  }
  return p;
}

@

@<Global def...@>=
static inline literal trailPeek(funcsat *f)
{
  literal p = clausePeek(&f->trail);
  return p;
}

@ The prototypes.
@<Internal decl...@>=

void trailPush(funcsat *f, literal p, uintptr_t reason_info_idx);
literal trailPop(funcsat *f, head_tail *facts);
static inline literal trailPeek(funcsat *f);
static inline uintptr_t reason_info_mk(funcsat *f, clause *cls);

@*2 Jailing and exoneration. Clause jailing is simply stashing clauses that we
can easily prove BCP won't ever need to consider (in a particular part of the
search tree).

Right now it's turned off.

Preconditions:
\unorderedlist
\li the clause is alone through the 1st literal
\li |trueLit| is true
\li the clause is (2-watch) linked through the 0th literal
\endunorderedlist

@<Global def...@>=
static void jailClause(funcsat *f, literal trueLit, clause *c)
{
UNUSED(f), UNUSED(trueLit), UNUSED(c);
#if 0
  ++f->numJails;
  dopen(f, "jail");
  assert(funcsatValue(f, trueLit) == true);
  const variable trueVar = fs_lit2var(trueLit);
  dmsg(f, "jail", 7, false, "jailed for %u: ", trueVar);
  clauseUnSpliceWatch((clause **) &watches->data[fs_lit2idx(-c->data[0])], c, 0);
  clause *cell = &f->jail.data[trueVar];
  clauseSplice1(c, &cell);
  dclose(f, "jail");
  assert(!c->is_watched);
#endif
}

static void exonerateClauses(funcsat *f, variable v)
{
  clause *p, *c, *nx;
  for_head_tail (&f->jail[v], p, c, nx) {
    c->nx = NULL;
    addWatchUnchecked(f, c);
  }
  head_tail tmp;
  head_tail_clear(&tmp);
  memcpy(&f->jail[v], &tmp, sizeof(tmp));
}

@*1 Decisions. We use a heap to organize our unassigned variables. Then a
decision is made based on their priority. The heap needs to have in it at least
all unassigned variables, though it may have assigned variables in it.

\paragraphit{\it Digression.}
%
I originally wrote a fibonacci heap because I believe it would be the fastest
for this application. (Asymptotically, it is very fast.) But there are a few
problems with it:

\numberedlist

\li It is big. Each node needs something like four pointers, and even though I
allocate all the nodes contiguously, that's big. It's going to affect the cache
behavior.

An array-backed binary heap one pointer for each heap element: the pointer
points to the node associated with its position in the heap. The ``pointers'' to
the child elements are just array index expressions.

\li I've since learned about pairing heaps, which are alleged to be just as fast
(or faster) than fibheaps, and much cheaper to implement. From what I've read,
it looks like you can get a pairing heap with only two pointers per node
(instead of four).

I wonder, though, if people use pairing heaps chiefly because they're afraid of
trying to implement a fibonacci heap.

\endnumberedlist \hfill{\it End Digression.}

Anyway, back to this code. This is how we decide on the next branching variable:

\numberedlist

\li We extract the highest priority variable. It might be assigned or not, so we
have to check for that. If it is assigned, we keep picking variables until
(1)~we can't anymore or (2)~we find an unassigned one.

\li We check which phase the variable should get. (This should be made more
general. Probably we should have a heap of literals, not variables.)

\li Once we have a literal to set to true, we put it on the trail and adjust a
few other data structures.

\endnumberedlist

@<Global def...@>=
literal funcsatMakeDecision(funcsat *f, void *p)
{
  UNUSED(p);
  literal l = 0;
  while (bh_size(f) > 0) {
    fslog(f, "decide", 5, "extracting\n");
    variable v = bh_pop(f);
#ifndef NDEBUG
    bh_check(f);
#endif
    fslog(f, "decide", 5, "extracted %u\n", v);
    literal p = -fs_var2lit(v);
    if (funcsatValue(f,p) == unknown) {
      if (f->conf->usePhaseSaving) l = f->phase.data[v];
      else l = p;
      ++f->numDecisions, f->pctdepth /= 2.0, f->correction = 1;
      trailPush(f, l, NO_REASON);
      f->level.data[v] = (int)++f->decisionLevel;
      f->decisions.data[v] = f->decisionLevel;
      fslog(f, "solve", 2, "branched on %i\n", l);
      break;
    }
  }
  assert(l != 0 || bh_size(f) == 0);
  return l;
}

@ 
@<Internal decl...@>=

literal funcsatMakeDecision(funcsat *, void *);


@ Because the variable ordering is dynamic, we can choose strategic points at
which to ``bump'' the priority of a variable. So if at some point during solving
we decide that a given variable is a but more important, we call
|varBumpScore|.
@<Global def...@>=
void varBumpScore(funcsat *f, variable v)
{
  double *activity_v = bh_var2act(f,v);
  double origActivity, activity;
  origActivity = activity = *activity_v;
  if ((activity += f->varInc) > 1e100) {     /* rescale */
    for (variable j = 1; j <= f->numVars; j++) {
      double *m = bh_var2act(f,j);
      fslog(f, "decide", 5, "old activity %f, rescaling\n", *m);
      *m *= 1e-100;
    }
    double oldVarInc = f->varInc;
    f->varInc *= 1e-100;
    fslog(f, "decide", 1, "setting varInc from %f to %f\n", oldVarInc, f->varInc);
    activity *= 1e-100;
  }
  if (bh_is_in_heap(f, v)) {
    bh_increase_activity(f, v, activity);
  } else {
    *activity_v = activity;
  }
  fslog(f, "decide", 5, "bumped %u from %.30f to %.30f\n", v, origActivity, *activity_v);
}

@ Activities are stored as |double|s. Comparing them always returns 1 if the
first activity is {\it more active} than the second (and therefore should be
branched on first).

@<Global def...@>=
static inline int activity_compare(double x, double y)
{
  if (x > y) return 1;
  else if (x < y) return -1;
  else return 0;
}

@
@<Internal decl...@>=
static inline int activity_compare(double x, double y);


@*2 Binary decision variable heap. This basic data structure is what Minisat
uses. This implementation, however, is original. The idea is pretty simple.

Each heap node is stored at some index of an array, {\it whether it's in the
heap or not}. If the node is not in the heap, it is stored after the
|f->binvar_heap_size| index. If the node is in the heap, its children are stored
at known offsets of the parent. We know how big this array must be
(|f->numVars|). The heap invariant is that a heap node is no smaller than each
of its children.

Each heap node is 16 bytes:
\numberedlist
\li a |double| priority (can we make this a |float|?)
\li a |variable| variable (needed to find out which var to return on a |bh_pop|)
\endnumberedlist

The auxiliary array of {\it positions} (|f->heap_pos|) maps a |variable| to its
current position in the heap. This way when we adjust a |variable|'s priority we
can easily find the variable in the heap.  The |f->heap_pos| invariant is that
|f->heap_pos[v]| is the index of the heap node corresponding to variable |v|.

@<Internal ty...@>=
struct bh_node
{
  variable var;
  double   act;
};

@ Returns the index into |f->binvar_heap| that gives the current variable's
priority.
@<Global def...@>=
static inline unsigned bh_var2pos(funcsat *f, variable v)
{
  return f->binvar_pos[v];
}
static inline bool bh_is_in_heap(funcsat *f, variable v)
{
  assert(bh_var2pos(f,v) > 0);
  return bh_var2pos(f,v) <= f->binvar_heap_size;
}
static inline bool bh_node_is_in_heap(funcsat *f, struct bh_node *n)
{
  assert(n >= f->binvar_heap);
  return (unsigned)(n - f->binvar_heap) <= f->binvar_heap_size;
}

@ |bh_increase_activity| needs to adjust the priority---the following function
is used for this.

@<Global def...@>=

static inline double *bh_var2act(funcsat *f, variable v)
{
  return &f->binvar_heap[bh_var2pos(f,v)].act;
}

@ The following functions allow one to traverse heap: to obtain the children and
the parent of a given node. Note, as with other |variable| data structures, the
first node of the heap is at index 1, leaving index 0 unused.

@<Global def...@>=
static inline struct bh_node *bh_top(funcsat *f)
{
  return f->binvar_heap + 1;
}
static inline struct bh_node *bh_bottom(funcsat *f)
{
  return f->binvar_heap + f->binvar_heap_size;
}
static inline bool bh_is_top(funcsat *f, struct bh_node *v)
{
  return bh_top(f) == v;
}
static inline struct bh_node *bh_left(funcsat *f, struct bh_node *v)
{
  return f->binvar_heap + (2 * (v - f->binvar_heap));
}
static inline struct bh_node *bh_right(funcsat *f, struct bh_node *v)
{
  return f->binvar_heap + (2 * (v - f->binvar_heap) + 1);
}
static inline struct bh_node *bh_parent(funcsat *f, struct bh_node *v)
{
  return f->binvar_heap + ((v - f->binvar_heap) / 2);
}
static inline unsigned bh_size(funcsat *f)
{
  return f->binvar_heap_size;
}

@ Getting the node location in the heap for an arbitrary |variable| is
accomplished with the help of |f->binvar_pos|, which records exactly that
information.

@<Global def...@>=
static inline struct bh_node *bh_node_get(funcsat *f, variable v)
{
  return f->binvar_heap + f->binvar_pos[v];
}


@ decls
@<Internal decl...@>=

static inline unsigned bh_var2pos(funcsat *f, variable v);
static inline bool bh_is_in_heap(funcsat *f, variable v);
static inline bool bh_node_is_in_heap(funcsat *f, struct bh_node *);
static inline double *bh_var2act(funcsat *f, variable v);
static inline struct bh_node *bh_top(funcsat *f);
static inline struct bh_node *bh_bottom(funcsat *f);
static inline bool bh_is_top(funcsat *f, struct bh_node *v);
static inline struct bh_node *bh_left(funcsat *f, struct bh_node *v);
static inline struct bh_node *bh_right(funcsat *f, struct bh_node *v);
static inline struct bh_node *bh_parent(funcsat *f, struct bh_node *v);
static inline unsigned bh_size(funcsat *f);
static inline variable bh_pop(funcsat *f);
static inline void bh_insert(funcsat *f, variable v);


@ Insert. After incrementing the heap size, the to insert is placed at the last
free location of the heap. Then it gets bubbled up to the location where the
heap invariant holds again.

Insertion and deletion need often to swap two elements of the heap. |bh_swap|
helps with this.

@<Global def...@>=
static inline void bh_swap(funcsat *f, struct bh_node **x, struct bh_node **y)
{
  struct bh_node tmp = **x, *tmpp = *x;
  **x = **y, **y = tmp;
  *x = *y, *y = tmpp;           /* swap the pointers, too */
  f->binvar_pos[(*x)->var] = *x - f->binvar_heap;
  f->binvar_pos[(*y)->var] = *y - f->binvar_heap;
}

static inline void bh_bubble_up(funcsat *f, struct bh_node *e)
{
  while (!bh_is_top(f, e)) {
    struct bh_node *p = bh_parent(f, e);
    if (activity_compare(p->act, e->act) < 0) {
      bh_swap(f, &p, &e);
    } else
      break;
  }
}

static inline void bh_insert(funcsat *f, variable v)
{
  assert(bh_size(f)+1 <= f->numVars);
  assert(bh_var2pos(f,v) > f->binvar_heap_size);
  struct bh_node *node = &f->binvar_heap[bh_var2pos(f,v)];
  assert(node->var == v);
  f->binvar_heap_size++;
  struct bh_node *last = &f->binvar_heap[f->binvar_heap_size];
  bh_swap(f, &node, &last);
  bh_bubble_up(f, node);
  assert(f->binvar_heap[bh_var2pos(f,v)].var == v); /* invariant */
}


@ Extract Max. We take the bottom element of the heap and replace the top with
it. Then we bubble it down until the heap property holds again.

@<Global def...@>=
static inline void bh_bubble_down(funcsat *f, struct bh_node *e)
{
  struct bh_node *l, *r;
  goto bh_bd_begin;
  while (bh_node_is_in_heap(f,l)) {
    if (bh_node_is_in_heap(f,r)) {
      if (activity_compare(l->act, r->act) < 0)
        l = r; /* put max child in |l| */
    }
    if (activity_compare(e->act, l->act) < 0) {
      bh_swap(f, &e, &l);
    } else
      break;
bh_bd_begin:
    l = bh_left(f,e), r = bh_right(f,e);
  }
}

static inline variable bh_pop(funcsat *f)
{
  assert(f->binvar_heap_size > 0);
  struct bh_node *top = bh_top(f);
  struct bh_node *bot = bh_bottom(f);
  bh_swap(f, &top, &bot);
  f->binvar_heap_size--;
  bh_bubble_down(f, bh_top(f));
  return top->var;
}

@ decls
@<Global def...@>=
static inline void bh_increase_activity(funcsat *f, variable v, double act_new)
{
  double *act_curr = bh_var2act(f,v);
  struct bh_node *n = bh_node_get(f, v);
  assert(n->var == v);
  assert(*act_curr <= act_new);
  *act_curr = act_new;
  bh_bubble_up(f, n);
}

@ decl
@<Internal decl...@>=
static inline void bh_increase_activity(funcsat *f, unsigned node_pos, double new_act);

@ we should check the heap property!
@<Global def...@>=
static void bh_check_node(funcsat *f, struct bh_node *x)
{
  struct bh_node *l = bh_left(f,x), *r = bh_right(f,x);
  if (bh_node_is_in_heap(f,l)) {
    assert(activity_compare(l->act, x->act) <= 0);
    bh_check_node(f,l);
  }
  if (bh_node_is_in_heap(f,r)) {
    assert(activity_compare(r->act, x->act) <= 0);
    bh_check_node(f,r);
  }
}

static void bh_check(funcsat *f)
{
  struct bh_node *root = bh_top(f);
  if (bh_node_is_in_heap(f, root)) {
    bh_check_node(f, root);
  }
  for (unsigned i = 1; i < f->numVars; i++) {
    assert(bh_node_get(f, i)->var == i);
  }
}

@
@<Internal decl...@>=
static void bh_check(funcsat *f);

@ Initializing is easy.
@<Initialize funcsat...@>=
CallocX(f->binvar_heap, 2, sizeof(*f->binvar_heap));
CallocX(f->binvar_pos, 2, sizeof(*f->binvar_pos));
f->binvar_heap_size = 0;


@ Destroying.
@<Destroy internal data...@>=
free(f->binvar_heap);
free(f->binvar_pos);
f->binvar_heap_size = 0;


@ Allocating is easy.
@<Resize internal data...@>=
ReallocX(f->binvar_heap, numVars+1, sizeof(*f->binvar_heap));
ReallocX(f->binvar_pos, numVars+1, sizeof(*f->binvar_pos));


@ Initializing each activity is also easy.
@<Incrementally resize internal data...@>=
f->binvar_heap[v].var = v;
f->binvar_heap[v].act = f->conf->getInitialActivity(&v);
f->binvar_pos[v] = v;
bh_insert(f, v);
assert(f->binvar_pos[v] != 0);


@ Print the heap.
@<Global def...@>=
static void bh_padding(funcsat *f, const char *s, int x)
{
  while (x-- > 0) {
    fprintf(fs_dbgout(f), "%s", s);
  }
}

static bool bh_print_levels(funcsat *f, FILE *dotfile, struct bh_node *r, int level)
{
  assert(r);
  if (bh_node_is_in_heap(f, r)) {
    bool lf, ri;
    lf = bh_print_levels(f, dotfile, bh_left(f, r), level+1);
    ri = bh_print_levels(f, dotfile, bh_right(f, r), level+1);
    if (lf) fprintf(dotfile, "%u -> %u [label=\"L\"];\n", r->var, bh_left(f,r)->var);
    if (ri) fprintf(dotfile, "%u -> %u [label=\"R\"];\n", r->var, bh_right(f,r)->var);
    fprintf(dotfile, "%u [label=\"%u%s, %.1f\"];\n", r->var, r->var,
            (funcsatValue(f, r->var) == true ? "T" :
             (funcsatValue(f, r->var) == false ? "F" : "?")), r->act);
    return true;
  } else {
    return false;
  }
}

static void bh_print(funcsat *f, const char *path, struct bh_node *r)
{
  if (!r) r = bh_top(f);
  FILE *dotfile;
  if (NULL == (dotfile = fopen(path, "w"))) perror("fopen"), exit(1);
  fprintf(dotfile, "digraph G {\n");
  bh_print_levels(f, dotfile, r, 0);
  fprintf(dotfile, "}\n");
  if (0 != fclose(dotfile)) perror("fclose");
  fprintf(fs_dbgout(f), "\n");
}

@ 
@<Internal decl...@>=
static void bh_print(funcsat *f, const char *path, struct bh_node *r);
@*1 Clause learning.\definexref{pg-clause-learn}{\folio}{page} \funcsat\ learns
unique implication point (UIP) clauses. It can learn any number of them,
starting with the first. The top-level procedure is |analyze_conflict|, which
assumes:

\numberedlist

\li the solver is in conflict (|f->conflictClause| is set)

\li the literals in |f->conflictClause| are all |false| (under |funcsatValue|)

\endnumberedlist

@<Global def...@>=
bool analyze_conflict(funcsat *f)
{
  ++f->numConflicts;
#ifndef NDEBUG
#if 0
  print_dot_impl_graph(f, f->conflictClause);
#endif
#endif

  f->uipClause.size = 0;
  clauseCopy(&f->uipClause, f->conflictClause);

  head_tail facts;
  head_tail_clear(&facts);
  literal uipLit = 0;
  variable i, c = 0;
  literal *it;
  /* c - count current-decision-level literals in the learned clause and
   * initialise litPos for fast querying of what's in the conflict clause and
   * all intermediate resolvents. */
  for (i = 0, c = 0; i < f->uipClause.size; i++) {
    variable v = fs_lit2var(f->uipClause.data[i]);
    f->litPos.data[v] = i;
    if ((unsigned)levelOf(f,v) == f->decisionLevel) c++;
  }
  int64_t entrydl = (int64_t)f->decisionLevel;
  /* find all the uip clauses and put them in facts, along with any UIP clauses
   * we generated on previous conflicts */
#ifndef NDEBUG
  watches_check(f);
#endif
  bool isUnsat = findUips(f, c, &facts, &uipLit);
  /* calculate pct covered */
  if (entrydl <= (int64_t) f->decisionLevel + (int64_t) f->correction) f->pct += f->pctdepth;
  for (int64_t i = (int64_t) entrydl;
       i > (int64_t) f->decisionLevel + (int64_t) f->correction;
       i--) {
    f->pct += f->pctdepth;
    f->pctdepth *= 2.0;
  }
  f->correction = 0;
  for (i = 0; i < f->uipClause.size; i++) {
    variable v = fs_lit2var(f->uipClause.data[i]);
    f->litPos.data[v] = UINT_MAX;
  }
  if (isUnsat) {
    fslog(f, "solve", 1, "findUips returned isUnsat\n");
    f->conflictClause = NULL;
    clause *prev, *curr, *next;
    for_head_tail (&facts, prev, curr, next) {
      clauseDestroy(curr);
    }
    return false;
  }

  f->conf->decayAfterConflict(f);
  f->conflictClause = NULL;
#ifndef NDEBUG
  watches_check(f);
#endif
  if (!propagateFacts(f, &facts, uipLit)) {
    return analyze_conflict(f);
  } else {
    return true;
  }
}

@ |findUips| does the heavy lifting of forming the UIP clauses. Returns |true|
when the SAT instance is UNSAT.

@<Global def...@>=
bool findUips(funcsat *f, unsigned c, head_tail *facts, literal *uipLit)
{
  bool more = true;
  uint32_t numUipsLearned = 0;
  literal p;
  fs_ifdbg (f, "findUips", 1) {
    fslog(f, "findUips", 1, "conflict@@%u (#%" PRIu64 ") with ",
          f->decisionLevel, f->numConflicts);
    fs_clause_print(f, fs_dbgout(f), &f->uipClause);
    fprintf(fs_dbgout(f), "\n");
  }
  dopen(f, "findUips");
  do { /* loop until we have learned enough UIPs */
    clause *reason = NULL;
    do {                        /* fills |f->uipClause| with the next UIP */

      @q/*@>@<Find the most recent literal on the trail that's in |f->uipClause|@>@;@q*/@>
      reason = getReason(f, p);
      @<Resolve |reason| with |f->uipClause|@>@;
      @<Update count |c| of current-level clause literals@>@;
      if (f->conf->useSelfSubsumingResolution) {
      /* we might learn a clause that subsumes the reason. check. */

        checkSubsumption(f, p, &f->uipClause, reason, !(c > 1));
      }
    } while (c > 1);
    ++numUipsLearned;    /* WE HAVE LEARNED A UIP. YAY! */
    assert(c == 1 || f->decisionLevel == 0);

    @<Check for UNSAT from the learned clause@>@;

    /* undo enough assignments so that the top of the trail is the uip
     * literal */
    p = trailPeek(f);
    while (!hasLitPos(f, p)) {
      trailPop(f, facts);
      p = trailPeek(f);
    }
    more = getReason(f, p) != NULL;
    assert(f->decisionLevel != VARIABLE_MAX);


    @<Add the learned clause to the unit facts for |fs_lit2var(p)|@>@;
  } while (more && numUipsLearned < f->conf->numUipsToLearn);
  bool foundDecision = isDecision(f, fs_lit2var(p));
  *uipLit = -trailPop(f, facts); /* pop the UIP literal from the trail. */
  /* if the UIP literal is not the decision variable, be sure to back up over
   * the last decision.  this should only happen if we learned fewer uips than
   * we could (|f->conf->numUipsToLearn|) */
  while (!foundDecision) {
    foundDecision = isDecision(f, fs_lit2var(trailPeek(f)));
    trailPop(f, facts);
  }
  fslog(f, "findUips", 5, "learned %" PRIu32 " UIPs\n", numUipsLearned);
  dclose(f, "findUips");
  fslog(f, "findUips", 5, "done\n");

  @<Deal with subsumed clauses@>@;

  return f->uipClause.size == 0 || c == 0;
}

@
@<Find the most recent...@>=
      while (!hasLitPos(f, p = trailPeek(f))) {
        trailPop(f, facts);
      }

@ We perform actual clause resolution. Because of the litPos data structure, it
can happen in linear time.

@<Resolve |reason|...@>=
      fs_ifdbg (f, "findUips", 6) {
        fslog(f, "findUips", 6, "resolving ");
        fs_clause_print(f, fs_dbgout(f), &f->uipClause);
        fprintf(fs_dbgout(f), " with ");
        fs_clause_print(f, fs_dbgout(f), reason);
        fprintf(fs_dbgout(f), "\n");
      }
      trailPop(f, facts);
      assert(reason);
      f->conf->bumpReason(f, reason);
      variable num = resolveWithPos(f, p, &f->uipClause, reason);
      if (f->uipClause.size == 0) return true; /* check for UNSAT */

@ We need an accurate count of the number of literals in |f->uipClause| from the
current decision level. After resolving a literal from the clause with its
|reason|, we need to update this count.

@<Update count |c|...@>=
      c = c - 1 + num;
      c = resetLevelCount(f, c, facts);


@ After we have learned a UIP, we may have discovered that the SAT instance is
unsatisfiable. This can happen when we backtrack all the way to decision
level 0.

@<Check for UNSAT...@>=
    if (f->decisionLevel == 0) return true;

@ Since we might produce more than one learned clause per conflict, we need a
list to store them. We use the unit fact list at the current UIP literal |p| for
our storage.

@<Add the learned clause...@>=
    /* allocate new storage for the learned clause */
    clause *newUip = clauseAlloc((&f->uipClause)->size);
    /** todo use memcpy not clausePush */
    clauseCopy(newUip, &f->uipClause);
    fs_ifdbg (f, "findUips", 5) {
      fslog(f, "findUips", 5, "found raw UIP: ");
      fs_clause_print(f, fs_dbgout(f), newUip);
      fprintf(fs_dbgout(f), "\n");
    }
    newUip->isLearnt = true;
    /* add learned clause to the unit facts for uip p */
    spliceUnitFact(f, p, newUip);
    vectorPush(&f->learnedClauses, newUip);
    ++f->numLearnedClauses;

    @<Order unit fact literals@>@;
    f->conf->bumpLearned(f, newUip);
    fs_ifdbg (f, "findUips", 5) {
      fslog(f, "findUips", 5, "found min UIP: ");
      fs_clause_print(f, fs_dbgout(f), newUip);
      fprintf(fs_dbgout(f), "\n");
    }

@ While munging the unit facts, we need to ensure that the invariant on
|f->unitFacts| for |propagateFacts| holds for this clause: the UIP literal needs
to be the first literal in the clause, and the second literal is the one that
was most recently inferred.

We minimize the learned clause here, too, for some reason.

@<Order unit fact literals@>=
    literal watch2 = 0, watch2_level = -1;
    unsigned pPos = getLitPos(f, p);
    literal tmp = newUip->data[0];
    newUip->data[0] = -p;
    newUip->data[pPos] = tmp;
    if (f->conf->minimizeLearnedClauses) minimizeUip(f, newUip);
    for (variable i = 1; i < newUip->size; i++) { /* find max level literal */
      literal lev = levelOf(f, fs_lit2var(newUip->data[i]));
      if (watch2_level < lev) {
        watch2_level = lev;
        watch2 = (literal) i;
      }
      if (lev == (literal) f->decisionLevel) break;
    }
    if (watch2_level != -1) {
      tmp = newUip->data[1];
      newUip->data[1] = newUip->data[watch2];
      newUip->data[watch2] = tmp;
    }


@ We need some auxiliary functions. The first re-counts literal levels in the
clause after a resolution step.

@<Global def...@>=
static unsigned resetLevelCount(funcsat *f, unsigned c, head_tail *facts)
{
  /* after the resolution step, it may be that the last decision didn't
   * directly contribute to this conflict, so the count is 0. if it is 0 at
   * this point, backtrack until it is at least 1 */
  while (c == 0 && f->decisionLevel != 0) {
    literal p = trailPeek(f);
    if (hasLitPos(f, p)) {
      /* at least one var at this level, so reinitialise count and
       * continue learning. */
      variable i;
      for (i = 0, c = 0; i < f->uipClause.size; i++) {
        if ((unsigned)levelOf(f, fs_lit2var(f->uipClause.data[i])) == f->decisionLevel) {
          c++;
        }
      }
    } else trailPop(f, facts);
  }
  return c;
}



@ After learning, we may have discovered learned (intermediate or UIP) clauses
that subsume existing clauses. If so, we should take that information and
strengthen our clause database.

@<Deal with subsumed clauses@>=
#if 0
  if (f->conf->useSelfSubsumingResolution) {
    clause **it;
    forVector (clause **, it, &f->subsumed) {
      bool subsumedByUip = (bool)*it;
      ++it;

      fs_ifdbg (f, "subsumption", 1) {
        fprintf(dbgout(f), "removing clause due to subsumption: ");
        dimacsPrintClause(dbgout(f), *it);
        fprintf(dbgout(f), "\n");
      }

      if ((*it)->isLearnt) {
        clause *removedClause = funcsatRemoveClause(f, *it);
        assert(removedClause);
        if (subsumedByUip) clauseFree(removedClause);
        else {
          removedClause->data[0] = removedClause->data[--removedClause->size];
          vectorPush(&f->learnedClauses, removedClause);
          fs_ifdbg (f, "subsumption", 1) {
            fprintf(dbgout(f), "new adjusted clause: ");
            dimacsPrintClause(dbgout(f), removedClause);
            fprintf(dbgout(f), "\n");
          }
          if (removedClause->is_watched) {
            addWatch(f, removedClause);
          }
        }
      }
    }
    f->subsumed.size = 0;
  }
#endif

@ Learned clause minimization.
@<Global def...@>=
static void cleanSeen(funcsat *f, variable top)
{
  while (f->analyseToClear.size > top) {
    variable v = fs_lit2var(intvecPop(&f->analyseToClear));
    f->seen.data[v] = false;
  }
}

static bool isAssumption(funcsat *f, variable v)
{
  literal *it;
  forClause (it, &f->assumptions) {
    if (fs_lit2var(*it) == v) {
      return true;
    }
  }
  return false;
}

/**
 * Performs a depth-first search of the conflict graph beginning at q0.
 */
static bool litRedundant(funcsat *f, literal q0)
{
  if (levelOf(f, fs_lit2var(q0)) == 0) return false;
  if (!getReason(f, q0)) return false;
  assert(!isAssumption(f, fs_lit2var(q0)));

  posvecClear(&f->analyseStack);
  posvecPush(&f->analyseStack, fs_lit2var(q0));
  variable top = f->analyseToClear.size;

  while (f->analyseStack.size > 0) {
    variable p = posvecPop(&f->analyseStack);
    clause *c = getReason(f, (literal)p);
    variable i;
    /* begins at 1 because |fs_lit2var(c->data[0])==p| */
    for (i = 1; i < c->size; i++) {
      literal qi = c->data[i];
      variable v = fs_lit2var(qi);
      literal lev = levelOf(f, v);
      if (!f->seen.data[v] && lev > 0) {
        if (getReason(f, qi) && f->allLevels.data[lev]) {
          posvecPush(&f->analyseStack, v);
          intvecPush(&f->analyseToClear, qi);
          f->seen.data[v] = true;
        } else {
          cleanSeen(f, top);
          return false;
        }
      }
    }
  }
  return true;
}

void minimizeUip(funcsat *f, clause *learned)
{
  /** todo use integer to denote the levels in the set, then just increment */
  variable i, j;
  for (i = 0; i < learned->size; i++) {
    literal l = levelOf(f, fs_lit2var(learned->data[i]));
    assert(l != Unassigned);
    f->allLevels.data[l] = true;
  }

  intvecClear(&f->analyseToClear);
  literal *it;
  forClause (it, learned) {
    intvecPush(&f->analyseToClear, *it);
  }

  /* memset(f->seen.data, 0, f->seen.capacity * sizeof(*f->seen.data)); */
  for (i = 0; i < learned->size; i++) {
    f->seen.data[fs_lit2var(learned->data[i])] = true;
  }

  /* uip literal is assumed to be in learned->data[0], so skip it by starting at
   * i=1 */
  for (i = 1, j = 1; i < learned->size; i++) {
    literal p = learned->data[i];
    if (!litRedundant(f, p)) learned->data[j++] = p;
    else {
      assert(!isAssumption(f, fs_lit2var(p)));
      fslog(f, "minimizeUip", 5, "deleted %i\n", p);
      ++f->numLiteralsDeleted;
    }
  }
  learned->size -= i - j;

  /* specialised cleanSeen(f, 0) to include clearing of allLevels */
  while (f->analyseToClear.size > 0) {
    literal l = intvecPop(&f->analyseToClear);
    variable v = fs_lit2var(l);
    f->seen.data[v] = false;
    f->allLevels.data[levelOf(f,v)] = false;
  }
}


@
@<Internal decl...@>=
static void print_dot_impl_graph(funcsat *f, clause *cc);

@ It is convenient to be able to dump the implication graph to a dot file so we can
render it in graphviz.

@<Global def...@>=
static char *dot_lit2label(literal p)
{
  static char buf[64];
  sprintf(buf, "lit%i", fs_lit2var(p));
  return buf;
}

static void print_dot_impl_graph_rec(funcsat *f, FILE *dotfile, bool *seen, literal p)
{
  if (seen[fs_lit2var(p)] == false) {
    fprintf(dotfile, "%s ", dot_lit2label(p));
    fprintf(dotfile, "[label=\"%i @@ %u%s\"];\n",
      (funcsatValue(f,p) == false ? -p : p),
      levelOf(f, fs_lit2var(p)),
      (funcsatValue(f,p)==unknown ? "*" : ""));
    seen[fs_lit2var(p)] = true;
    clause *r = getReason(f, p);
    if (r) {
      literal *q;
      fprintf(dotfile, "/* reason for %i: ", p);
      fs_clause_print(f, dotfile, r);
      fprintf(dotfile, "*/\n");
      for_clause (q, r) {
        print_dot_impl_graph_rec(f, dotfile, seen, *q);
        if (*q != -p) {
          fprintf(dotfile, "%s", dot_lit2label(*q));
          fprintf(dotfile, " -> ");
          fprintf(dotfile, "%s;\n", dot_lit2label(p));
        }
      }
    } else {
      fprintf(dotfile, "/* no reason for %i */\n", p);
    }
  }
}

static void print_dot_impl_graph(funcsat *f, clause *cc)
{
  literal *p, *q;
  bool *seen;
  CallocX(seen, f->numVars+1, sizeof(*seen));
  FILE *dotfile = fopen("g.dot", "w");

  fprintf(dotfile, "digraph G {\n");
  fprintf(dotfile, "/* conflict clause: ");
  fs_clause_print(f, dotfile, cc);
  fprintf(dotfile, "*/\n");
  for_clause (p, cc) {
    print_dot_impl_graph_rec(f, dotfile, seen, *p);
  }
#ifdef EDGES_FOR_CLAUSE
  q = NULL; /* edges in conflict clause */
  for_clause (p, cc) {
    if (q) {
      fprintf(dotfile, "%s", dot_lit2label(*p));
      fprintf(dotfile, " -> ");
      fprintf(dotfile, "%s [color=\"red\"];\n", dot_lit2label(*q));
    }
    q = p;
  }
#endif
  fprintf(dotfile, "\n}");
  free(seen);
  fclose(dotfile);
}


@*2 Unit facts. At the end of learning we have a list of new clauses. We need to
propagate them, and more importantly shunt them into {\it unit fact lists}.

@<Internal decl...@>=
bool findUips(funcsat *f, unsigned c, head_tail *facts, literal *uipLit);
bool propagateFacts(funcsat *f, head_tail *facts, literal uipLit);

static unsigned resetLevelCount(funcsat *f, unsigned c, head_tail *facts);

static void checkSubsumption(
  funcsat *f,
  literal p, clause *learn, clause *reason,
  bool learnIsUip);

@ The following function propagates the given unit facts.  Each clause is
either:
%
\numberedlist
\li unit
\li or has at least two unassigned literals.
\endnumberedlist In the first case, we immediately make an assignment, but this function will
ignore that assignment for the purposes of detecting a conflict.  In the latter
case, we add the clause to the watched literals lists.  The uips not added to
the watched literal lists are attached as unit facts to the variable dec.

It is possible that propagateFacts may discover a conflict.  If so, it returns
|false|.


@ The following code is wrapped around the crucial node, |@<Propagate single unit...@>|.

@<Global def...@>=
bool propagateFacts(funcsat *f, head_tail *facts, literal uipLit)
{
  bool isConsistent = true;
  variable uipVar = fs_lit2var(uipLit);
  unsigned cnt = 0;
  dopen(f, "bcp");
  /* Assign each unit fact */
  clause *prev, *curr, *next;
  for_head_tail (facts, prev, curr, next) {
    ++f->numUnitFactProps, ++cnt;
    @<Propagate single unit fact@>@;
    if (!isConsistent) break;
  }

  dclose(f, "bcp");
  fslog(f, "bcp", 1, "propagated %u facts\n", cnt);
  if (f->conflictClause) {
    if (funcsatValue(f, uipLit) == unknown) {
      if (f->decisionLevel > 0) {
        uipVar = fs_lit2var(f->trail.data[f->trail.size-1]);
      } else goto ReallyDone;
    }
  }
#if 0
  assert(!head || funcsatValue(f, fs_var2lit(uipVar)) != unknown);
  assert(clauseIsAlone(&f->unitFacts.data[uipVar], 0));
#endif
  /* if var@@0, then all learns are SAT@@0 */
  if (levelOf(f, uipVar) != 0) {
    head_tail_append(&f->unit_facts[uipVar], facts);
  }

ReallyDone:
  return isConsistent;
}

@ Using the list of |facts|, |curr| and |prev|, we can propagate a single unit
fact. If we detect an inconsistency we set |isConsistent| to |false|.

@<Propagate single unit...@>=
fs_ifdbg (f, "bcp", 5) {
  fslog(f, "bcp", 5, "");
  fs_clause_print(f, fs_dbgout(f), curr);
}

if (curr->size == 0) goto Conflict;
else if (curr->size == 1) {
  literal p = curr->data[0];
  mbool val = funcsatValue(f,p);
  if (val == false) goto Conflict;
  else if (val == unknown) {
    trailPush(f, p, reason_info_mk(f, curr));
    fslog(f, "bcp", 1, " => %i\n", p);
  }
  continue;
}

literal p = curr->data[0], q = curr->data[1];
assert(p != 0 && q != 0 && "unset literals");
assert(p != q && "did not find distinct literals");
mbool vp = tentativeValue(f,p), vq = tentativeValue(f,q);

/* if |(vp==true && vq == true)| (if jailing, put in jailed list for data[1],
 * else put in watcher list) */
if (vp == true && vq == true) {
  head_tail_iter_rm(facts, prev, curr, next);
  addWatchUnchecked(f, curr);
  fslog(f, "bcp", 5, " => watched\n");
} else if (vp == true || vq == true) {        /* clause is SAT */
  ;                           /* leave in unit facts */
  fslog(f, "bcp", 5, " => unmolested\n");
} else if (vp == unknown && vq == unknown) {
  head_tail_iter_rm(facts, prev, curr, next);
  addWatchUnchecked(f, curr);
  fslog(f, "bcp", 5, " => watched\n");
} else if (vp == unknown) {
  if (funcsatValue(f,p) == false) goto Conflict;
  assert(curr->data[0] == p);
  trailPush(f, p, reason_info_mk(f, curr)); /* clause is unit, implying p */
  f->conf->bumpUnitClause(f, curr);
  fslog(f, "bcp", 1, " => %i\n", p);
} else if (vq == unknown) {
  if (funcsatValue(f,q) == false) goto Conflict;
  assert(curr->data[1] == q);
  curr->data[0] = q, curr->data[1] = p;
  trailPush(f, q, reason_info_mk(f, curr)); /* clause is unit, implying q */
  fslog(f, "bcp", 1, " => %i\n", q);
  f->conf->bumpUnitClause(f, curr);
} else {
Conflict: /* clause is UNSAT */
  isConsistent = false;
  f->conflictClause = curr;
  fslog(f, "bcp", 1, " => X\n");
}

@ A slightly thorny case, that I can't explain yet, is why we use
|tentativeValue| here instead of |funcsatValue|. |tentativeValue| works just
like its counterpart, only looks at values that have been {\it already
propagated} by BCP. This can be fewer, but never more, than those that are
assigned.


@<Global def...@>=
static inline mbool tentativeValue(funcsat *f, literal p)
{
  variable v = fs_lit2var(p);
  literal *valLoc = &f->trail.data[f->model.data[v]];
  bool isTentative = valLoc >= f->trail.data + f->propq;
  if (levelOf(f,v) != Unassigned && !isTentative) return p == *valLoc;
  else return unknown;
}

@

@<Internal decl...@>=
static inline mbool tentativeValue(funcsat *f, literal p);


@ We need to initialize the unit facts.

@<Initialize funcsat type@>=
  CallocX(f->unit_facts, 2, sizeof(*f->unit_facts));
  f->unit_facts_size = 1;
  f->unit_facts_capacity = 2;


@ And grow them.
@<Incrementally resize internal data structures...@>=
if (f->unit_facts_size >= f->unit_facts_capacity) {
  ReallocX(f->unit_facts, f->unit_facts_capacity*2, sizeof(*f->unit_facts));
  f->unit_facts_capacity *= 2;
}
head_tail_clear(&f->unit_facts[f->unit_facts_size]);
f->unit_facts_size++;


@ And free them.
@<Destroy internal data structures@>=
free(f->unit_facts);

@

@<Internal decl...@>=
void minimizeUip(funcsat *f, clause *learned);



/**
 * Performs a resolution on the literal p.
 * @@pre p is in pReason
 * @@pre -p is in learn
 * @@pre learn and pReason are sorted
 * @@param p a literal from pReason; -p is in learn
 * @@param learn sorted clause, also the resolvent after returning
 * @@param pReason sorted clause
 */
variable resolve(funcsat *f, literal p, clause *learn, clause *pReason);

/**
 * Performs a resolution on the literal p using the position list, which is
 * stored in |funcsat->litPos|.
 *
 * @@pre p is in pReason
 * @@pre -p is in learn
 * @@param p a literal from pReason; -p is in learn
 * @@param learn clause, also the resolvent after returning
 * @@param pReason clause
 */
variable resolveWithPos(funcsat *f, literal p, clause *learn, clause *pReason);


@*1 Restart functions.

@<Global def...@>=
void incLubyRestart(funcsat *f, bool skip);

bool funcsatNoRestart(funcsat *f, void *p) {
  UNUSED(f), UNUSED(p);
  return false;
}

bool funcsatLubyRestart(funcsat *f, void *p)
{
  UNUSED(p);
  if ((int)f->numConflicts >= f->lrestart && f->decisionLevel > 2) {
    incLubyRestart(f, false);
    return true;
  }
  return false;
}

bool funcsatInnerOuter(funcsat *f, void *p)
{
  UNUSED(p);
  static uint64_t inner = 100UL, outer = 100UL, conflicts = 1000UL;
  if (f->numConflicts >= conflicts) {
    conflicts += inner;
    if (inner >= outer) {
      outer *= 1.1;
      inner = 100UL;
    } else {
      inner *= 1.1;
    }
    return true;
  }
  return false;
}

bool funcsatMinisatRestart(funcsat *f, void *p)
{
  UNUSED(p);
  static uint64_t cutoff = 100UL;
  if (f->numConflicts >= cutoff) {
    cutoff *= 1.5;
    return true;
  }
  return false;
}



/* This stuff was cribbed from picosat and changed a smidge just to get bigger
 * integers. */


int64_t luby(int64_t i)
{
  int64_t k;
  for (k = 1; k < (int64_t)sizeof(k); k++)
    if (i == (1 << k) - 1)
      return 1 << (k - 1);

  for (k = 1;; k++)
    if ((1 << (k - 1)) <= i && i < (1 << k) - 1)
      return luby (i - (1 << (k-1)) + 1);
}

void incLubyRestart(funcsat *f, bool skip)
{
  UNUSED(skip);
  uint64_t delta;

  /* Luby calculation takes a really long time around 255? */
  if (f->lubycnt > 250) {
    f->lubycnt = 0;
    f->lubymaxdelta = 0;
    f->waslubymaxdelta = false;
  }
  delta = 100 * (uint64_t)luby((int64_t)++f->lubycnt);
  f->lrestart = (int64_t)(f->numConflicts + delta);

  /* if (waslubymaxdelta) */
  /*   report (1, skip ? 'N' : 'R'); */
  /* else */
  /*   report (2, skip ? 'n' : 'r'); */

  if (delta > f->lubymaxdelta) {
    f->lubymaxdelta = delta;
    f->waslubymaxdelta = 1;
  } else {
    f->waslubymaxdelta = 0;
  }
}


@*1 Configuration. \funcsat\ is configurable in many ways. The |funcsat_config|
type is responsible for \funcsat's configuration.

@<Internal decl...@>=
static void noBumpOriginal(funcsat *f, clause *c);
void myDecayAfterConflict(funcsat *f);
void lbdDecayAfterConflict(funcsat *f);

@
@c

funcsat_config funcsatDefaultConfig = {
  .user = NULL,
  .name = NULL,
  .usePhaseSaving = false,
  .useSelfSubsumingResolution = false,
  .minimizeLearnedClauses = true,
  .numUipsToLearn = UINT_MAX,
  .gc = true,
  .maxJailDecisionLevel = 0,
  .logSyms = NULL,
  .printLogLabel = true,
  .debugStream = NULL,
  .isTimeToRestart = funcsatLubyRestart,
  .isResourceLimitHit = funcsatIsResourceLimitHit,
  .preprocessNewClause = funcsatPreprocessNewClause,
  .preprocessBeforeSolve = funcsatPreprocessBeforeSolve,
  .getInitialActivity = funcsatDefaultStaticActivity,
  .sweepClauses = claActivitySweep,
  .bumpOriginal = bumpOriginal,
  .bumpReason = bumpReasonByActivity,
  .bumpLearned = bumpLearnedByActivity,
  .bumpUnitClause = bumpUnitClauseByActivity,
  .decayAfterConflict = myDecayAfterConflict,
};

@*1 Initialization. Requires an initialized configuration.

@<Initialize parameters@>=
  f->varInc = 1.f;
  f->varDecay = 0.95f; /* or 0.999f; */
  f->claInc = 1.f;
  f->claDecay = 0.9999f; /* or 0.95f; */
  f->learnedSizeFactor = 1.f / 3.f;
  f->learnedSizeAdjustConfl = 25000;  /* see |startSolving| */ 
  f->learnedSizeAdjustCnt = 25000;
  f->maxLearned = 20000.f;
  f->learnedSizeAdjustInc = 1.5f;
  f->learnedSizeInc = 1.1f;


@ Initially allocates a |funcsat| object.
@c

funcsat *funcsatInit(funcsat_config *conf)
{
  funcsat *f;
  CallocX(f, 1, sizeof(*f));
  f->conf = conf;
  f->conflictClause = NULL;
  f->propq = 0;
  @<Initialize parameters@>@;
  fslog(f, "sweep", 1, "set maxLearned to %f\n", f->maxLearned);
  fslog(f, "sweep", 1, "set 1/f->claDecoy to %f\n", 1.f/f->claDecay);
  f->lrestart = 1;
  f->lubycnt = 0;
  f->lubymaxdelta = 0;
  f->waslubymaxdelta = false;

  f->numVars = 0;
  @<Initialize funcsat type@>@;
  clauseInit(&f->assumptions, 0);
  posvecInit(&f->model, 2);
  posvecPush(&f->model, 0);
  clauseInit(&f->phase, 2);
  clausePush(&f->phase, 0);
  clauseInit(&f->level, 2);
  clausePush(&f->level, Unassigned);
  posvecInit(&f->decisions, 2);
  posvecPush(&f->decisions, 0);
  clauseInit(&f->trail, 2);
  all_watches_init(f);
  vectorInit(&f->origClauses, 2);
  vectorInit(&f->learnedClauses, 2);
  posvecInit(&f->litPos, 2);
  posvecPush(&f->litPos, 0);
  clauseInit(&f->uipClause, 100);
  posvecInit(&f->lbdLevels, 100);
  posvecPush(&f->lbdLevels, 0);
  posvecInit(&f->seen, 2);
  posvecPush(&f->seen, false);
  intvecInit(&f->analyseToClear, 2);
  intvecPush(&f->analyseToClear, 0);
  posvecInit(&f->analyseStack, 2);
  posvecPush(&f->analyseStack, 0);
  posvecInit(&f->allLevels, 2);
  posvecPush(&f->allLevels, false);
  vectorInit(&f->subsumed, 10);

  return f;
}



@ Initializes a |funcsat| configuration.
@c
funcsat_config *funcsatConfigInit(void *userData)
{
  UNUSED(userData);
  funcsat_config *conf = malloc(sizeof(*conf));
  memcpy(conf, &funcsatDefaultConfig, sizeof(*conf));
#ifdef FUNCSAT_LOG
  conf->logSyms = create_hashtable(16, hashString, stringEqual);
  posvecInit(&conf->logStack, 2);
  posvecPush(&conf->logStack, 0);
  conf->debugStream = stderr;
#endif
  return conf;
}

void funcsatConfigDestroy(funcsat_config *conf)
{
#ifdef FUNCSAT_LOG
  hashtable_destroy(conf->logSyms, true, true);
  posvecDestroy(&conf->logStack);
#endif
  free(conf);
}

@ For incrementality, \funcsat\ resizes for a given predicted or known number of
variables. It just goes through and grows every data structure to fit |numVars|.

@c

void funcsatResize(funcsat *f, variable numVars)
{
  assert(f->decisionLevel == 0); /* so we can fix up unit facts */
  if (numVars > f->numVars) {
    const variable old = f->numVars, new = numVars;
    f->numVars = new;
    variable i;

    clauseGrowTo(&f->uipClause, numVars);
    intvecGrowTo(&f->analyseToClear, numVars);
    posvecGrowTo(&f->analyseStack, numVars);
    @<Resize internal data structures up to new |numVars|@>@;
    for (i = old; i < new; i++) {
      variable v = i+1;
      @<Incrementally resize internal data structures up to new |numVars|@>@;
#if 0
      literal l = fs_var2lit(v);
#endif
      posvecPush(&f->model, UINT_MAX);
      clausePush(&f->phase, -fs_var2lit(v));
      clausePush(&f->level, Unassigned);
      posvecPush(&f->decisions, 0);
      posvecPush(&f->litPos, UINT_MAX);
      posvecPush(&f->lbdLevels, 0);
      posvecPush(&f->seen, false);
      posvecPush(&f->allLevels, false);
    }
    unsigned highestIdx = fs_lit2idx(-(literal)numVars)+1;
    assert(f->model.size     == numVars+1);
    assert(!f->conf->usePhaseSaving || f->phase.size == numVars+1);
    assert(f->level.size     == numVars+1);
    assert(f->decisions.size == numVars+1);
    assert(f->reason->size    == numVars+1);
    assert(f->unit_facts_size == numVars+1);
    assert(f->uipClause.capacity >= numVars);
    assert(f->allLevels.size == numVars+1);
    assert(f->watches.size   == highestIdx);

    if (numVars > f->trail.capacity) {
      ReallocX(f->trail.data, numVars, sizeof(*f->trail.data));
      f->trail.capacity = numVars;
    }
  }
}

@ Destroys a |funcsat|.

@c
void funcsatDestroy(funcsat *f)
{
  literal i;
  while (f->trail.size > 0) trailPop(f, NULL);
  @<Destroy internal data structures@>@;
  clauseDestroy(&f->assumptions);
  posvecDestroy(&f->model);
  clauseDestroy(&f->phase);
  clauseDestroy(&f->level);
  posvecDestroy(&f->decisions);
  clauseDestroy(&f->trail);
  posvecDestroy(&f->litPos);
  clauseDestroy(&f->uipClause);
  posvecDestroy(&f->analyseStack);
  intvecDestroy(&f->analyseToClear);
  posvecDestroy(&f->lbdLevels);
  posvecDestroy(&f->seen);
  posvecDestroy(&f->allLevels);
  for (unsigned i = 0; i < f->origClauses.size; i++) {
    clause *c = f->origClauses.data[i];
    clauseFree(c);
  }
  vectorDestroy(&f->origClauses);
  for (unsigned i = 0; i < f->learnedClauses.size; i++) {
    clause *c = f->learnedClauses.data[i];
    clauseFree(c);
  }
  vectorDestroy(&f->learnedClauses);
  vectorDestroy(&f->subsumed);
  free(f);
}


@

@c
void funcsatSetupActivityGc(funcsat_config *conf)
{
  conf->gc = true;
  conf->sweepClauses = claActivitySweep;
  conf->bumpReason = bumpReasonByActivity;
  conf->bumpLearned = bumpLearnedByActivity;
  conf->decayAfterConflict = myDecayAfterConflict;
}

void funcsatSetupLbdGc(funcsat_config *conf)
{
  conf->gc = true;
  conf->sweepClauses = lbdSweep;
  conf->bumpReason = bumpReasonByLbd;
  conf->bumpLearned = bumpLearnedByLbd;
  conf->decayAfterConflict = lbdDecayAfterConflict; /** \todo is this right? */
}

funcsat_result funcsatResult(funcsat *f)
{
  return f->lastResult;
}







@ Backtracking.
@c
void backtrack(funcsat *f, variable newLevel, head_tail *facts, bool isRestart)
{
  head_tail restartFacts;
  head_tail_clear(&restartFacts);
  if (isRestart) {
    assert(newLevel == 0UL);
    assert(!facts);
    facts = &restartFacts;
  }
  while (f->decisionLevel != newLevel) {
    trailPop(f, facts);
  }
  if (isRestart) {
    f->pct = 0., f->pctdepth = 100., f->correction = 1;
    f->propq = 0;
  }
}

static void restore_facts(funcsat *f, head_tail *facts)
{
  clause *prev, *curr, *next;
  for_head_tail (facts, prev, curr, next) {
    /* if the clause was subsumed, it could be here but not learned */
    assert(curr->size >= 1);

    if (curr->size == 1) {
      literal p = curr->data[0];
      mbool val = funcsatValue(f,p);
      assert(val != false);
      if (val == unknown) {
        trailPush(f, p, reason_info_mk(f, curr));
        fslog(f, "bcp", 5, " => %i\n", p);
      }
    } else {
      head_tail_iter_rm(facts, prev, curr, next);
      addWatchUnchecked(f, curr);
#ifndef NDEBUG
      watches_check(f);
#endif
    }
  }
}

@
@<Internal decl...@>=
static void restore_facts(funcsat *f, head_tail *facts);

@ These two functions are for sanity's sake.

@c

funcsat_result startSolving(funcsat *f)
{
  f->numSolves++;
  if (f->conflictClause) {
    if(f->conflictClause->size == 0) {
      return FS_UNSAT;
    } else {
      f->conflictClause = NULL;
    }
  }

  backtrack(f, 0UL, NULL, true);
  f->lastResult = FS_UNKNOWN;

  assert(f->decisionLevel == 0);
  return FS_UNKNOWN;
}

/**
 * Call me whenever (incremental) solving is done, whether because of a timeout
 * or whatever.
 */
static void finishSolving(funcsat *f)
{
  UNUSED(f);
}




@ Another break.

@c


static bool bcpAndJail(funcsat *f)
{
  if (!bcp(f)) {
    fslog(f, "solve", 2, "returning false at toplevel\n");
    return false;
  }

  fslog(f, "solve", 1, "bcpAndJail trailsize is %u\n", f->trail.size);

  clause **cIt;
  vector *watches;
  uint64_t numJails = 0;

#if 0
  forVector (clause **, cIt, &f->origClauses) {
    if ((*cIt)->is_watched) {
      literal *lIt;
      bool allFalse = true;
      forClause (lIt, *cIt) {
        mbool value = funcsatValue(f, *lIt);
        if (value == false) continue;
        allFalse = false;
        if (value == true) {
          /* jail the clause, it is satisfied */
          clause **w0 = (clause **) &watches->data[fs_lit2idx(-(*cIt)->data[0])];
          /* assert(watcherFind(*cIt, w0, 0)); */
          clause **w1 = (clause **) &watches->data[fs_lit2idx(-(*cIt)->data[1])];
          /* assert(watcherFind(*cIt, w1, 0)); */
          /* clauseUnSpliceWatch(w0, *cIt, 0); */
          clauseUnSpliceWatch(w1, *cIt, 1);
          jailClause(f, *lIt, *cIt);
          numJails++;
          break;
        }
      }
      if (allFalse) {
        fslog(f, "solve", 2, "returning false at toplevel\n");
        return false;
      }
    }
  }
#endif

  fslog(f, "solve", 2, "jailed %" PRIu64 " clauses at toplevel\n", numJails);
  return true;
}


bool funcsatIsResourceLimitHit(funcsat *f, void *p)
{
  UNUSED(f), UNUSED(p);
  return false;
}
funcsat_result funcsatPreprocessNewClause(funcsat *f, void *p, clause *c)
{
  UNUSED(p), UNUSED(c);
  return (f->lastResult = FS_UNKNOWN);
}
funcsat_result funcsatPreprocessBeforeSolve(funcsat *f, void *p)
{
  UNUSED(p);
  return (f->lastResult = FS_UNKNOWN);
}


@ Rest of stuff.

@c
void funcsatPrintStats(FILE *stream, funcsat *f)
{
  fprintf(stream, "c %" PRIu64 " decisions\n", f->numDecisions);
  fprintf(stream, "c %" PRIu64 " propagations (%" PRIu64 " unit)\n",
          f->numProps + f->numUnitFactProps,
          f->numUnitFactProps);
  fprintf(stream, "c %" PRIu64 " jailed clauses\n", f->numJails);
  fprintf(stream, "c %" PRIu64 " conflicts\n", f->numConflicts);
  fprintf(stream, "c %" PRIu64 " learned clauses\n", f->numLearnedClauses);
  fprintf(stream, "c %" PRIu64 " learned clauses removed\n", f->numLearnedDeleted);
  fprintf(stream, "c %" PRIu64 " learned clause deletion sweeps\n", f->numSweeps);
  if (!f->conf->minimizeLearnedClauses) {
    fprintf(stream, "c (learned clause minimisation off)\n");
  } else {
    fprintf(stream, "c %" PRIu64 " redundant learned clause literals\n", f->numLiteralsDeleted);
  }
  if (!f->conf->useSelfSubsumingResolution) {
    fprintf(stream, "c (on-the-fly self-subsuming resolution off)\n");
  } else {
    fprintf(stream, "c %" PRIu64 " subsumptions\n", f->numSubsumptions);
    fprintf(stream, "c - %" PRIu64 " original clauses\n", f->numSubsumedOrigClauses);
    fprintf(stream, "c - %" PRIu64 " UIPs (%2.2lf%%)\n", f->numSubsumptionUips,
            (double)f->numSubsumptionUips*100./(double)f->numSubsumptions);
  }
  fprintf(stream, "c %" PRIu64 " restarts\n", f->numRestarts);
  fprintf(stream, "c %d assumptions\n", f->assumptions.size);
  fprintf(stream, "c %u original clauses\n", f->origClauses.size);
}

void funcsatPrintColumnStats(FILE *stream, funcsat *f)
{
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  double uTime = ((double)usage.ru_utime.tv_sec) +
    ((double)usage.ru_utime.tv_usec)/1000000;
  double sTime = ((double)usage.ru_stime.tv_sec) +
    ((double)usage.ru_stime.tv_usec)/1000000;
  fprintf(stream, "Name,NumDecisions,NumPropagations,NumUfPropagations,"
                  "NumLearnedClauses,NumLearnedClausesRemoved,"
                  "NumLearnedClauseSweeps,NumConflicts,NumSubsumptions,"
                  "NumSubsumedOrigClauses,NumSubsumedUips,NumRestarts,"
                  "UserTimeSeconds,SysTimeSeconds\n");
  fprintf(stream, "%s,", f->conf->name);
  fprintf(stream, "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
          ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
          ",%" PRIu64 ",%.2lf,%.2lf\n",
          f->numDecisions,
          f->numProps + f->numUnitFactProps,
          f->numUnitFactProps,
          f->numLearnedClauses,
          f->numLearnedDeleted,
          f->numSweeps,
          f->numConflicts,
          f->numSubsumptions,
          f->numSubsumedOrigClauses,
          f->numSubsumptionUips,
          f->numRestarts,
          uTime,
          sTime);
}

void funcsatBumpLitPriority(funcsat *f, literal p)
{
  varBumpScore(f, fs_lit2var(p));
}







/* FORMERLY INTERNAL.H */


/* defaults */




literal levelOf(funcsat *f, variable v)
{
  return f->level.data[v];
}


literal fs_var2lit(variable v)
{
  literal result = (literal) v;
  assert((variable) result == v);
  return result;
}

variable fs_lit2var(literal l)
{
  if (l < 0) {
    l = -l;
  }
  return (variable) l;
}

unsigned fs_lit2idx(literal l)
{
  /* just move sign bit into the lowest bit instead of the highest */
  variable v = fs_lit2var(l);
  v <<= 1;
  v |= (l < 0);
  return v;
}

bool isDecision(funcsat *f, variable v)
{
  return 0 != f->decisions.data[v];
}




#if 0
void singlesPrint(FILE *stream, clause *begin)
{
  clause *c = begin;
  if (c) {
    do {
      clause *next = c->next[0];
      clause *prev = c->prev[0];
      if (!next) {
        fprintf(stream, "next is NULL");
        return;
      }
      if (!prev) {
        fprintf(stream, "prev is NULL");
        return;
      }
      if (next->prev[0] != c) fprintf(stream, "n*");
      if (prev->next[0] != c) fprintf(stream, "p*");
      dimacsPrintClause(stream, c);
      c = next;
      if (c != begin) fprintf(stream, ", ");
    } while (c != begin);
  } else {
    fprintf(stream, "NULL");
  }
}
#endif

#if 0
void watcherPrint(FILE *stream, clause *c, uint8_t w)
{
  if (!c) {
    fprintf(stream, "EMPTY\n");
    return;
  }

  clause *begin = c;
  literal data = c->data[w];
  fprintf(stream, "watcher list containing lit %i\n", c->data[w]);
  do {
    uint8_t i = c->data[0]==data ? 0 : 1;
    clause *next = c->next[i];
    if (!next) {
      fprintf(stream, "next is NULL\n");
      return;
    }
    if (!(next->prev[ next->data[0]==c->data[i] ? 0 : 1 ] == c)) {
      fprintf(stream, "*");
    }
    dimacsPrintClause(stream, c);
    fprintf(stream, "\n");
    c = next;
  } while (c != begin);
}



bool watcherFind(clause *c, clause **watches, uint8_t w)
{
  clause *curr = *watches, *nx, *end;
  uint8_t wi;
  bool foundEnd;
  forEachWatchedClause(curr, c->data[w], wi, nx, end, foundEnd) {
    if (curr == c) return true;
  }
  return false;
}

void binWatcherPrint(FILE *stream, funcsat *f)
{
  variable v;
  unsigned i;
  for (v = 1; v <= f->numVars; v++) {
    binvec_t *bv = f->watchesBin.data[fs_lit2idx(fs_var2lit(v))];
    if (bv->size > 0) {
      fprintf(stream, "%5ji -> ", fs_var2lit(v));
      for (i = 0; i < bv->size; i++) {
        literal imp = bv->data[i].implied;
        fprintf(stream, "%i", imp);
        if (i+1 != bv->size) fprintf(stream, ", ");
      }
      fprintf(stream, "\n");
    }

    bv = f->watchesBin.data[fs_lit2idx(-fs_var2lit(v))];
    if (bv->size > 0) {
      fprintf(stream, "%5ji -> ", -fs_var2lit(v));
      for (i = 0; i < bv->size; i++) {
        literal imp = bv->data[i].implied;
        fprintf(stream, "%i", imp);
        if (i+1 != bv->size) fprintf(stream, ", ");
      }
      fprintf(stream, "\n");
    }
  }
}

#endif

unsigned funcsatNumClauses(funcsat *f)
{
  return f->numOrigClauses;
}

unsigned funcsatNumVars(funcsat *f)
{
  return f->numVars;
}

@

@c
void funcsatPrintHeuristicValues(FILE *p, funcsat *f)
{
  for (variable i = 1; i <= f->numVars; i++) {
    double *value = bh_var2act(f,i);
    fprintf(p, "%u = %4.2lf\n", i, *value);
  }
  fprintf(p, "\n");
}

static void fs_print_state(funcsat *f, FILE *p)
{
  variable i;
  literal *it;
  if (!p) p = stderr;

  fprintf(p, "assumptions: ");
  forClause (it, &f->assumptions) {
    fprintf(p, "%i ", *it);
  }
  fprintf(p, "\n");

  fprintf(p, "dl %u (%" PRIu64 " X, %" PRIu64 " d)\n",
          f->decisionLevel, f->numConflicts, f->numDecisions);

  fprintf(p, "trail: ");
  for (i = 0; i < f->trail.size; i++) {
    fprintf(p, "%2i", f->trail.data[i]);
    if (f->decisions.data[fs_lit2var(f->trail.data[i])] != 0) {
      fprintf(p, "@@%u", f->decisions.data[fs_lit2var(f->trail.data[i])]);
    }
    if (!head_tail_is_empty(&f->unit_facts[fs_lit2var(f->trail.data[i])])) {
      fprintf(p, "*");
    }
#if 0
    if (!clauseIsAlone(&f->jail.data[fs_lit2var(f->trail.data[i])], 0)) {
      fprintf(p, "!");
    }
#endif
    fprintf(p, " ");
  }
  fprintf(p, "\n");

  fprintf(p, "model: ");
  for (i = 1; i <= f->numVars; i++) {
    if (levelOf(f, i) != Unassigned) {
      fprintf(p, "%3i@@%i ", f->trail.data[f->model.data[i]], levelOf(f, i));
    }
  }
  fprintf(p, "\n");
  fprintf(p, "pq: %u (-> %i)\n", f->propq, f->trail.data[f->propq]);
}

void funcsatPrintConfig(FILE *stream, funcsat *f)
{
  funcsat_config *conf = f->conf;
  if (NULL != conf->user)
    fprintf(stream, "Has user data\n");
  if (NULL != conf->name)
    fprintf(stream, "Name: %s\n", conf->name);

  conf->usePhaseSaving ? 
    fprintf(stream, "phsv\t") : 
    fprintf(stream, "NO phsv\t");

  conf->useSelfSubsumingResolution ? 
    fprintf(stream, "ssr\t") : 
    fprintf(stream, "NO ssr\t");

  conf->minimizeLearnedClauses ?
    fprintf(stream, "min\t") : 
    fprintf(stream, "NO min\t");

  // TODO MAL check
  if (true == conf->gc) {
    if (lbdSweep == conf->sweepClauses) {
      fprintf(stream, "gc glucose\t");
    } else if (claActivitySweep == conf->sweepClauses) {
      fprintf(stream, "gc minisat\t");
    } else {
      abort(); /* impossible */
    }
  } else {
    fprintf(stream, "NO gc\t\t");
    assert(NULL == conf->sweepClauses);
  }

  if (funcsatLubyRestart == conf->isTimeToRestart) {
    fprintf(stream, "restart luby\t");
  } else if (funcsatNoRestart == conf->isTimeToRestart) {
    fprintf(stream, "restart none\t");
  } else if (funcsatInnerOuter == conf->isTimeToRestart) {
    fprintf(stream, "restart inout\t");
  } else if (funcsatMinisatRestart == conf->isTimeToRestart) {
    fprintf(stream, "restart minisat\t");
  } else {
    abort(); /* impossible */
  }

  fprintf(stream, "learn %" PRIu32 " uips\t\t", conf->numUipsToLearn);

  // TODO MAL this is how to print?
  fprintf(stream, "Jail up to %u uips\n", 
          conf->maxJailDecisionLevel);
#if 0
  if (funcsatIsResourceLimitHit == conf->isResourceLimitHit) {
    fprintf(stream, "  resource hit default\n");
  } else {
    abort();
  }

  if (funcsatPreprocessNewClause == conf->preprocessNewClause) {
    fprintf(stream, "  UNUSED preprocess new clause default\n");
  } else {
    abort();
  }

  if (funcsatPreprocessBeforeSolve == conf->preprocessBeforeSolve) {
    fprintf(stream, "  UNUSED preprocess before solve default\n");
  } else {
    abort();
  }

  if (funcsatDefaultStaticActivity == conf->getInitialActivity) {
    fprintf(stream, "  initial activity static (default)\n");
  } else {
    abort();
  }
#endif
}

void funcsatPrintCnf(FILE *stream, funcsat *f, bool learned)
{
  fprintf(stream, "c clauses: %u original", funcsatNumClauses(f));
  if (learned) {
    fprintf(stream, ", %u learned", f->learnedClauses.size);
  }
  fprintf(stream, "\n");

  unsigned num_assumptions = 0;
  for (unsigned i = 0; i < f->assumptions.size; i++) {
    if(f->assumptions.data[i] != 0)
      num_assumptions++;
  }

  fprintf(stream, "c %u assumptions\n", num_assumptions);
  unsigned numClauses = funcsatNumClauses(f) +
    (learned ? f->learnedClauses.size : 0) + num_assumptions;
  fprintf(stream, "p cnf %u %u\n", funcsatNumVars(f), numClauses);
  for (unsigned i = 0; i < f->assumptions.size; i++) {
    if(f->assumptions.data[i] != 0)
      fprintf(stream, "%i 0\n", f->assumptions.data[i]);
  }
  dimacsPrintClauses(stream, &f->origClauses);
  if (learned) {
    fprintf(stream, "c learned\n");
    dimacsPrintClauses(stream, &f->learnedClauses);
  }
}





void dimacsPrintClauses(FILE *f, vector *clauses)
{
  unsigned i;
  for (i = 0; i < clauses->size; i++) {
    clause *c = clauses->data[i];
    dimacsPrintClause(f, c);
    fprintf(f, "\n");
  }
}

void fs_clause_print(funcsat *f, FILE *out, clause *c)
{
  if (!out) out = stderr;
  literal *it;
  forClause (it, c) {
    fprintf(out, "%i@@%i%s ", *it, levelOf(f, fs_lit2var(*it)),
      (funcsatValue(f, *it) == true ? "T" :
       (funcsatValue(f, *it) == false ? "F" : "U")));
  }
}

void dimacsPrintClause(FILE *f, clause *c)
{
  if (!f) f = stderr;
  if (!c) {
    fprintf(f, "(NULL CLAUSE)");
    return;
  }
  variable j;
  for (j = 0; j < c->size; j++) {
    literal lit = c->data[j];
    fprintf(f, "%i ", lit);
  }
  fprintf(f, "0");
}

void funcsatClearStats(funcsat *f)
{
  f->numSweeps              = 0;
  f->numLearnedDeleted      = 0;
  f->numLiteralsDeleted     = 0;
  f->numProps               = 0;
  f->numUnitFactProps       = 0;
  f->numConflicts           = 0;
  f->numRestarts            = 0;
  f->numDecisions           = 0;
  f->numSubsumptions        = 0;
  f->numSubsumedOrigClauses = 0;
  f->numSubsumptionUips     = 0;
}

char *funcsatResultAsString(funcsat_result result)
{
  switch (result) {
  case FS_UNKNOWN: return "UNKNOWN";
  case FS_SAT: return "SATISFIABLE";
  case FS_UNSAT: return "UNSATISFIABLE";
  default: abort();             /* impossible */
  }
}


bool isUnitClause(funcsat *f, clause *c)
{
  variable i, numUnknowns = 0, numFalse = 0;
  for (i = 0; i < c->size; i++) {
    if (funcsatValue(f, c->data[i]) == unknown) {
      numUnknowns++;
    } else if (funcsatValue(f, c->data[i]) == false) {
      numFalse++;
    }
  }
  return numUnknowns==1 && numFalse==c->size-(unsigned)1;
}


int varOrderCompare(fibkey *a, fibkey *b)
{
  fibkey k = *a, l = *b;
  if (k > l) {
    return -1;
  } else if (k < l) {
    return 1;
  } else {
    return 0;
  }
}


static void noBumpOriginal(funcsat *f, clause *c) {
  UNUSED(f), UNUSED(c);
}
void bumpOriginal(funcsat *f, clause *c)
{
  literal *it;
  double orig_varInc = f->varInc;
  f->varInc +=  2.*(1./(double)c->size);
  forClause (it, c) {
    varBumpScore(f, fs_lit2var(*it));
  }
  f->varInc = orig_varInc;
}



clause *funcsatRemoveClause(funcsat *f, clause *c) {
 UNUSED(f), UNUSED(c); return NULL; }
#if 0
{
  assert(c->isLearnt);
  if (c->isReason) return NULL;

  if (c->is_watched) {
    /* remove \& release from the two watchers we had */
    clause **w0 = (clause **) &f->watches.data[fs_lit2idx(-c->data[0])];
    clause **w1 = (clause **) &f->watches.data[fs_lit2idx(-c->data[1])];
    clauseUnSpliceWatch(w0, c, 0);
    clauseUnSpliceWatch(w1, c, 1);
  } else {
    /* clause is in unit facts or jailed */
    clause *copy = c;
    clauseUnSplice(&copy, 0);
  }

  vector *clauses;
  if (c->isLearnt) {
    clauses = &f->learnedClauses;
  } else {
    clauses = &f->origClauses;
  }
  clauses->size--;
  return c;
}
#endif


extern int DebugSolverLoop;

static char *dups(const char *str)
{
  char *res;
  CallocX(res, strlen(str)+1, sizeof(*str));
  strcpy(res, str);
  return res;
}

bool funcsatDebug(funcsat *f, char *label, int level)
{
#ifdef FUNCSAT_LOG
  int *levelp;
  MallocX(levelp, 1, sizeof(*levelp));
  *levelp = level;
  hashtable_insert(f->conf->logSyms, dups(label), levelp);
  return true;
#else
  UNUSED(f), UNUSED(label), UNUSED(level);
  return false;
#endif
}



@
@<Internal decl...@>=
extern mbool funcsatValue(funcsat *f, literal p);

@*1 New clauses.

My first representation was like picosat's, but much worse. This means clauses
were ... big. 544 bits each! I need to cut this down. I'm going to make a few
changes.

@<Unused clause type@>=
typedef struct clause
{
  literal *data;

  uint32_t size;


  uint32_t capacity;


  bool isLearnt : 1;

  bool isReason : 1;

  bool inBinaryWatches : 1;
  uint32_t lbdScore : 29;

  struct clause *next[2];

  struct clause *prev[2];

  double activity;

  unsigned id;
} clause;

@ The (new) clause representation.

\numberedlist

\li Clause heads will be separate from the ``rest'' of the clause. In fact, all
clauses will be chunked into head and body parts. There is one head and various
body parts.

\li The activity and the score need to be unified. First of all, the LBD score
has way too many bits. After about 5 bits of information all the LBDs blend
together. The activity is slightly murkier because it's a float. Either I need
to (1) create custom float operations packed into 28 bits or (2) using the int
and make the activity recording work anyway. Not sure at the moment how to do
either but I know picosat does (1). (Can I get away with fixed-point floats?)

\endnumberedlist

So here's the new |clause_head| data structure. The |headlits| is either
|CLAUSE_HEAD_SIZE| literals, or the last one is an index (if |is_bigger| is
set). If it's an index it's an index into |f->clausebodies|.

The |clause_block| is a fixed size where the last |literal| is an index to the
rest of the clause, if necessary. If the clause terminates in this block, then
the literal after the last literal in the clause is a 0.

Each clause block will be allocated from the huge array, |f->clausebodies|. When
due to simplification or garbage collection a clause block is free'd, it simply
becomes unused and then |next_block| refers to the next block of the freelist
instead of the allocated block list. |f->freebodies| points to the first
free clause block.

@s clause_head int
@s clause_block int
@s clause_state int

@<Internal ty...@>=
#ifndef CLAUSE_HEAD_SIZE
#  define CLAUSE_HEAD_SIZE 4    /* or 8, or 16 */
#endif

#ifndef CLAUSE_BLOCK_SIZE
#  define CLAUSE_BLOCK_SIZE 8
#endif

enum clause_state {
     CLAUSE_WATCHED, /* in watchlist */
     CLAUSE_JAILED, /* in jail */
     CLAUSE_UNIT, /* in unit facts, don't GC */
};

typedef struct clause_head
{
  literal headlits[CLAUSE_HEAD_SIZE]; /* first few literals */

  uint32_t is_learned : 1;
  uint32_t is_reason : 1; /* pinned, don't GC */
  uint32_t is_bigger : 1; /* is |headlits[CLAUSE_HEAD_SIZE-1]| an index? */
  uint32_t where : 2; /* see |clause_state| */
  uint32_t lbd_score_or_activity : 27;

} clause_head;

typedef struct clause_block
{
  literal blocklits[CLAUSE_BLOCK_SIZE];
  unsigned next_block;
} clause_block;

@ What is the {\it lifecycle of a clause}? And why should you care? Because the
data structure has to support everything in its lifecycle. If the lifecycle is
in one place, it's easy to figure out whether later clause modifications will
efficiently support the needed operations.

\unorderedlist

\li While the client is constructing the CNF instance (or it's being read from
disk), the clause is being built and eventually is given to |funcsatAddClause|.

  \orderedlist

  \li If the clause is empty, the solver becomes immediately conflicted.

  \li If the clause is singleton, we put it on the trail and attach it as a {\it
  unit fact} to the newly-added trail literal

  \li If it's binary, it's put in the binary watcher list and it will never be
  garbage collected.

  \li Otherwise, it's put in the normal watchlist for the clause.

  \endorderedlist

\li Once all the clauses are added the search begins. At this point a clause can
become the {\it reason} for an inference. This means the clause was unit at the
time it implied a literal.

\li During backtracking a clause (especially learned) may transition to a unit
fact list which is eventually associated with a particular literal on the
trail. It may stay on a unit fact list across other decisions, inferences, and
backtracks. (But not across restarts unless it's a singleton.)

\li During BCP a clause can be {\it jailed}, meaning it's removed from its
watchlists and put into a shadow watchlist while it's satisfied. It is restored
to the watchlists once it becomes unsatisfied. (It does {\it not} go through any
unit fact list.)

\li A literal could be completely deleted from a clause if a literal in the
clause becomes true at decision level 0.


\endunorderedlist

When a garbage collection is triggered, the clause could be in any of these
states. Assuming the clause is not pinned, it is returned to the clause pool. A
clause is {\it pinned} when any of the following hold:

  \unorderedlist

  \li The clause is in a unit fact list.

  \li The clause is currently the {\it reason} for an inference.
  \li The clause is binary (we decided to keep them all around).

  \endunorderedlist

@ We need a clause pool.

@<foobar@>=
size_t sz = 1024;
if (numClauses > funcsatNumClauses(f)) {
  sz = numClauses;
}
f->pool = malloc(sz * sizeof(*f->pool));




@

@<External declarations@>=
struct funcsat_config;
funcsat *funcsatInit(funcsat_config *conf);

funcsat_config *funcsatConfigInit(void *userData);

void funcsatConfigDestroy(funcsat_config *);

void funcsatResize(funcsat *f, variable numVars);

void funcsatDestroy(funcsat *);

funcsat_result funcsatAddClause(funcsat *func, clause *clause);

funcsat_result funcsatPushAssumption(funcsat *f, literal p);

funcsat_result funcsatPushAssumptions(funcsat *f, clause *c);

void funcsatPopAssumptions(funcsat *f, unsigned num);

funcsat_result funcsatSolve(funcsat *func);

unsigned funcsatNumClauses(funcsat *func);

unsigned funcsatNumVars(funcsat *func);

void funcsatPrintStats(FILE *stream, funcsat *f);

void funcsatPrintColumnStats(FILE *stream, funcsat *f);

void funcsatClearStats(funcsat *f);

void funcsatBumpLitPriority(funcsat *f, literal p);

void funcsatPrintCnf(FILE *stream, funcsat *f, bool learned);

funcsat_result funcsatResult(funcsat *f);

clause *funcsatSolToClause(funcsat *f);

int funcsatSolCount(funcsat *f, clause subset, clause *lastSolution);

void funcsatReset(funcsat *f);

void funcsatSetupActivityGc(funcsat_config *);
void funcsatSetupLbdGc(funcsat_config *);

bool funcsatDebug(funcsat *f, char *label, int level);

clause *funcsatRemoveClause(funcsat *f, clause *c);


@* Debugging support.

@<External decl...@>=
char *funcsatResultAsString(funcsat_result result);


@ We want to print the variable interaction graph. I think that's what it's
called.

@<External decl...@>=
void fs_vig_print(funcsat *f, const char *path);

@

@<Global def...@>=
void fs_vig_print(funcsat *f, const char *path)
{
  FILE *dot;
  char buf[256];
  if (NULL == (dot = fopen(path, "w"))) perror("fopen"), exit(1);
  fprintf(dot, "graph G {\n");

  fprintf(dot, "// variables\n");
  for (variable i = 1; i <= f->numVars; i++) {
    fprintf(dot, "%u;\n", i);
  }

  fprintf(dot, "// clauses\n");
  unsigned cnt = 0;
  for (unsigned i = 0; i < f->origClauses.size; i++) {
    char *bufptr = buf;
    clause *c = f->origClauses.data[i];
    literal *p;
    for_clause (p, c) {
      bufptr += sprintf(bufptr, "%i ", *p);
    }
    fprintf(dot, "clause%u [shape=box,label=\"%s\",fillcolor=black,shape=box];\n",
            cnt, buf);
    for_clause (p, c) {
      fprintf(dot, "clause%u -- %u [%s];\n",
              cnt,
              fs_lit2var(*p),
              (*p < 0 ? "color=red" : "color=green"));
    }
    cnt++;
  }
  
  fprintf(dot, "}\n");
  if (0 != fclose(dot)) perror("fclose");

}


@ The \funcsat\ configuration.
@<External ty...@>=
typedef struct funcsat funcsat;
typedef struct funcsat_config
{
  void *user; /* clients can use this */

  char *name; /* Name of the currently running sat problem.  (May be null) */


  struct hashtable *logSyms;   /*  for logging */
  posvec logStack;
  bool printLogLabel;
  FILE *debugStream;

  bool usePhaseSaving; /* whether to save variable phases */

  bool useSelfSubsumingResolution;

  bool minimizeLearnedClauses;

  uint32_t numUipsToLearn;

  
  bool gc; /* If false, all learned clauses are kept for all time. */

  unsigned maxJailDecisionLevel;  /*
   Only jail clauses that occur at or below this decision level.
   */

  bool (*isTimeToRestart)(funcsat *, void *);  /*
   This function is called after a conflict is discovered and analyzed.  If it
   function returns |true|, \funcsat\ backtracks to decision level 0 and
   recommences.
   */


  bool (*isResourceLimitHit)(funcsat *, void *);   /*
   This function is called after any learned clause is added but before
   beginning the next step of unit propagation.  If it returns |true|, \funcsat\
   solver terminates whether the problem has been solved or not.
   */


  funcsat_result (*preprocessNewClause)(funcsat *, void *, clause *);  /*
   UNUSED
   */


  funcsat_result (*preprocessBeforeSolve)(funcsat *, void *);  /*
   UNUSED
   */


  double (*getInitialActivity)(variable *);  /*
   Gets the initial activity for the given variable.
   */


  void (*bumpOriginal)(funcsat *, clause *orig);  /*
   Called on original clauses.
   */



  void (*bumpReason)(funcsat *, clause *rsn);  /*
   Called on clauses that are resolved on during learning.
   */


  void (*bumpLearned)(funcsat *, clause *learned);  /*
   Called on the new UIP clause after the conflict is analyzed.
   */


  void (*bumpUnitClause)(funcsat *, clause *uc);  /*
   Called when a clause implies a unit literal.
   */


  void (*decayAfterConflict)(funcsat *f);  /*
   Called after a conflict is analyzed.
   */


  void (*sweepClauses)(funcsat *, void *);  /*
   Implements a policy for deleting learned clauses immediately after a
   restart.
   */

} funcsat_config;

/* |uint64_t funcsatDebugSet;| in main.c*/

/* Descriptions for these options can be found in |debugDescriptions[Option]| */

@ When debugging \funcsat, it is convenient to be able to minimize a test
case. Armin Biere has been kind enough to write {\sc cnfdd}, which is a
delta-debugging algorithm for CNF files. The macro |assertExit| lets one quickly
change a failing |assert| into one with exits with a particular code. So when
\funcsat\ fails a particular |assert(x)|, change it to |assertExit(2, x)| and
run {\sc cnfdd} to produce a minimal test case.

@<Conditional mac...@>=

#ifndef NDEBUG
#define assertExit(code, cond) \
  if (!(cond)) exit(code);
#else
#define assertExit(code, cond)
#endif

@ The following are functions used for logging messages to the console (if
|FUNCSAT_LOG| is enabled).

@d fs_dbgout(f) (f)->conf->debugStream

@<Conditional mac...@>=
#ifdef FUNCSAT_LOG
#ifndef SWIG
DECLARE_HASHTABLE(fsLogMapInsert, fsLogMapSearch, fsLogMapRemove, char, int)
#endif

#define fs_ifdbg(f, label, level)                                        \
  if ((f)->conf->logSyms &&                                             \
      fsLogMapSearch(f->conf->logSyms, (void *) (label)) &&             \
      (level) <= *fsLogMapSearch((f)->conf->logSyms, (void *) (label)))

#else

#define fs_ifdbg(f, label, level) if (false)

#endif

static inline int fslog(const struct funcsat *, const char *label,
                        int level, const char *format, ...);
static inline int dopen(const struct funcsat *, const char *label);
static inline int dclose(const struct funcsat *, const char *label);

@

@<Global def...@>=
DEFINE_HASHTABLE(fsLogMapInsert, fsLogMapSearch, fsLogMapRemove, char, int)

int fslog(const funcsat *f, const char *label, int msgLevel, const char *format, ...)
{
  int pr = 0;
  int *logLevel;
  va_list ap;

  if (f->conf->logSyms && (logLevel = hashtable_search(f->conf->logSyms, (void *) label))) {
    if (msgLevel <= *logLevel) {
      unsigned indent = posvecPeek(&f->conf->logStack), i;
      for (i = 0; i < indent; i++) fprintf(f->conf->debugStream, " ");
      if (f->conf->printLogLabel) pr += fprintf(f->conf->debugStream, "%s %d: ", label, msgLevel);
      va_start(ap, format);
      pr += vfprintf(f->conf->debugStream, format, ap);
      va_end(ap);
    }
  }
  return pr;
}

int dopen(const funcsat *f, const char *label)
{
  if (f->conf->logSyms && hashtable_search(f->conf->logSyms, (void *) label)) {
    unsigned indent = posvecPeek(&f->conf->logStack)+2;
    posvecPush(&f->conf->logStack, indent);
  }
  return 0;
}
int dclose(const funcsat *f, const char *label)
{
  if (f->conf->logSyms && hashtable_search(f->conf->logSyms, (void *) label)) {
    posvecPop(&f->conf->logStack);
  }
  return 0;
}


@

@<Global def...@>=
int64_t readHeader(int (*getChar)(void *), void *, funcsat *func);
void readClauses(
  int (*getChar)(void *), void *, funcsat *func, uint64_t numClauses);

#define VecGenTy(name, eltTy)                                           \
  typedef struct                                                        \
  {                                                                     \
    eltTy   *data;                                                      \
    size_t size;                /* num. items stored */                    \
    size_t capacity;            /* num. items malloc'd */                  \
  } name ## _t;                                                         \
  static void name ## Destroy(name ## _t *v);                           \
  static void name ## Init(name ## _t *v, size_t capacity);             \
  static void name ## Clear(name ## _t *v);                             \
  static void name ## Push(name ## _t *v, eltTy data);                  



#define VecGen(name, eltTy)                                     \
  static void name ## Init(name ## _t *v, size_t capacity)      \
  {                                                             \
    size_t c = capacity > 0 ? capacity : 4;                     \
    CallocX(v->data, c, sizeof(*v->data));                      \
    v->size = 0;                                                \
    v->capacity = c;                                            \
  }                                                             \
                                                                \
  static void name ## Destroy(name ## _t *v)                    \
  {                                                             \
    free(v->data);                                              \
    v->data = NULL;                                             \
    v->size = 0;                                                \
    v->capacity = 0;                                            \
  }                                                             \
                                                                \
  static void name ## Clear(name ## _t *v)                      \
  {                                                             \
    v->size = 0;                                                \
  }                                                             \
                                                                \
  static void name ## Push(name ## _t *v, eltTy data)           \
  {                                                             \
    if (v->capacity == v->size) {                               \
      ReallocX(v->data, v->capacity*2+1, sizeof(eltTy));        \
      v->capacity = v->capacity*2+1;                            \
    }                                                           \
    v->data[v->size++] = data;                                  \
  }


VecGenTy(vecc, char);
VecGen(vecc, char);

int fgetcWrapper(void *stream)
{
  return fgetc((FILE *) stream);
}

void parseDimacsCnf(const char *path, funcsat *func)
{
  struct stat buf;
  if (-1 == stat(path, &buf)) perror("stat"), exit(1);
  if (!S_ISREG(buf.st_mode)) {
    fprintf(stderr, "Error: '%s' not a regular file\n", path);
    exit(1);
  }
  int (*getChar)(void *);
  void *stream;
  const char *opener;
  bool isGz = 0 == strcmp(".gz", path + (strlen(path)-strlen(".gz")));
  if (isGz) {
#ifdef HAVE_LIBZ
#  if 0
  fprintf(stderr, "c found .gz file\n");
#  endif
    getChar = (int(*)(void *))gzgetc;
    stream = gzopen(path, "r");
    opener = "gzopen";
#else
    fprintf(stderr, "cannot read gzipp'd file\n");
    exit(1);
#endif
  } else {
    getChar = fgetcWrapper;
    stream = fopen(path, "r");
    opener = "fopen";
  }
  if (!stream) {
    perror(opener);
    exit(1);
  }

  uint64_t numClauses = (uint64_t) readHeader(getChar, stream, func);
  readClauses(getChar, stream, func, numClauses);

  if (isGz) {
#ifdef HAVE_LIBZ
    if (Z_OK != gzclose(stream)) perror("gzclose"), exit(1);
#else
    assert(0 && "no libz and this shouldn't happen");
#endif
  } else {
    if (0 != fclose(stream)) perror("fclose"), exit(1);
  }
}


static literal readLiteral(
  int (*getChar)(void *stream),
  void *stream,
  vecc_t *tmp,
  uint64_t numClauses);
int64_t readHeader(int (*getChar)(void *stream), void *stream, funcsat *func)
{
  UNUSED(func);
  char c;
Comments:
  while (isspace(c = getChar(stream))); /* skip leading spaces */
  if ('c' == c) {
    while ('\n' != (c = getChar(stream)));
  }

  if ('p' != c) {
    goto Comments;
  }
  while ('c' != (c = getChar(stream)));
  assert(c == 'c');
  c = getChar(stream);
  assert(c == 'n');
  c = getChar(stream);
  assert(c == 'f');

  vecc_t tmp;
  veccInit(&tmp, 4);
  readLiteral(getChar, stream, &tmp, 0);
  unsigned numClauses = (unsigned) readLiteral(getChar, stream, &tmp, 0);
  veccDestroy(&tmp);
#if 0
  fprintf(stderr, "c read header 'p cnf %u %u'\n", numVariables, numClauses);
#endif
  return (int) numClauses;
}


void readClauses(
  int (*getChar)(void *stream),
  void *stream,
  funcsat *func,
  uint64_t numClauses)
{
  clause *clause;
  vecc_t tmp;
  veccInit(&tmp, 4);
  if (numClauses > 0) {
    do {
      clause = clauseAlloc(5);

#if 0
      literal literal = readLiteral(getChar, stream);
#endif
      literal literal = readLiteral(getChar, stream, &tmp, numClauses);

      while (literal != 0) {
        clausePush(clause, literal);
        literal = readLiteral(getChar, stream, &tmp, numClauses);
      }
#if 0
      dimacsPrintClause(stderr, clause);
      fprintf(stderr, "\n");
#endif
      if (FS_UNSAT == funcsatAddClause(func, clause)) {
        fprintf(stdout, "c trivially unsat\n");
        exit(EXIT_SUCCESS);
      }
    } while (--numClauses > 0);
  }
  veccDestroy(&tmp);
}

static literal readLiteral(
  int (*getChar)(void *stream),
  void *stream,
  vecc_t *tmp,
  uint64_t numClauses)
{
  char c;
  bool begun;
  veccClear(tmp);
  literal literal;
  begun = false;
  while (1) {
    c = getChar(stream);
    if (isspace(c) || EOF == c) {
      if (begun) {
        break;
      } else if (EOF == c) {
        fprintf(stderr, "readLiteral error: too few clauses (after %"
                PRIu64 " clauses)\n", numClauses);
        exit(1);
      } else {
        continue;
      }
    }
    begun = true;
    veccPush(tmp, c);
  }
  veccPush(tmp, '\0');
  literal = strtol((char *) tmp->data, NULL, 10);
  return literal;
}


/**
 * Uses the lbd score to delete learned clauses.  It's pretty slow right now.
 */
void lbdSweep(funcsat *f, void *p);
void claActivitySweep(funcsat *f, void *p);


void varBumpScore(funcsat *f, variable v);


static variable computeLbdScore(funcsat *f, clause *c);


@
@<Global def...@>=


/**
 * Searches in the clause for the literal with the maximum decision level,
 * beginning at startIdx. If found, that literal is swapped with
 * reason[swapIdx]. This preserves the necessary invariant for the unit facts
 * list.
 */
static void swapInMaxLevelLit(funcsat *f, clause *reason, unsigned swapIdx, unsigned startIdx)
{
  literal secondWatch = 0, swLevel = -1;
  /* find max level lit, swap with lit[1] */
  for (variable i = startIdx; i < reason->size; i++) {
    literal lev = levelOf(f, fs_lit2var(reason->data[i]));
    fslog(f, "subsumption", 9, "level of %i = %u\n",
          reason->data[i], levelOf(f, fs_lit2var(reason->data[i])));
    if (swLevel < lev) swLevel = lev, secondWatch = (literal) i;
    /* TODO Does this speed stuff up? */
    if (lev == fs_var2lit(f->decisionLevel)) break;
  }
  if (swLevel != -1) {
    literal tmp = reason->data[swapIdx];
    reason->data[swapIdx] = reason->data[secondWatch], reason->data[secondWatch] = tmp;
  }
}

/* this function must be called after the resolution operation */
static void checkSubsumption(
  funcsat *f, literal p, clause *learn, clause *reason, bool learnIsUip)
{
  UNUSED(p);
  /* This is the test suggested in the paper "On-the-fly clause improvement" in
   * SAT09. */
  if (learn->size == reason->size-1 &&
      reason->size > 1 &&       /* learn subsumes reason */
      learn->size > 2) {

    ++f->numSubsumptions;

    fs_ifdbg (f, "subsumption", 1) {
      FILE *out = f->conf->debugStream;
      dimacsPrintClause(out, learn);
      fprintf(out, " subsumes ");
      dimacsPrintClause(out, reason);
      fprintf(out, "\n");
    }

    assert(!reason->isReason);

    if (learnIsUip) vectorPush(&f->subsumed, (void *)1);
    else vectorPush(&f->subsumed, (void *)0);
    vectorPush(&f->subsumed, reason);
  }
}



/* lbd score from the glucose paper: "Predicting Learnt Clauses Quality in
 * Modern SAT Solvers" by Audemard and Simon */
static variable computeLbdScore(funcsat *f, clause *c)
{
  /* assert(f->lbdLevelsStack.size==0); */
  /* f->lbdLevels[d] == true iff we've seen a literal l in c whose level is d */
  uint16_t score = 0;
  f->lbdCount++;

  for (variable i = 0; i < c->size; i++) {
    literal level = levelOf(f, fs_lit2var(c->data[i]));
    assert(level != Unassigned);
    if (f->lbdLevels.data[level] != f->lbdCount) {
      f->lbdLevels.data[level] = f->lbdCount;
      score++;
    }
    /* who cares if the score is big. */
    if (score > 16) break;
  }
  assert(c->size == 0 || score != 0);
  return score;
}

static int compareByLbdRev(const void *cl1, const void *cl2);


/**
 * First tests whether we have learned enough new clauses to warrant deletion.
 *
 * Deletes half of the learned clauses, preferring small lbd clauses, every
 * (10000/p) + (500/p)*num conflicts where num is the number of times we have
 * deleted clauses so far and p is the ratio of the number of learned clauses to
 * the number of conflicts.
 */
void lbdSweep(funcsat *f, void *user)
{
  UNUSED(user);
  static uint64_t num = 0, lastNumConflicts = 0, lastNumLearnedClauses = 0;
  uint64_t diffConfl = f->numConflicts - lastNumConflicts;
  uint64_t diffLearn = f->numLearnedClauses - lastNumLearnedClauses;
  uint64_t avg = diffConfl==0 ? 1 : Max(1, diffLearn/diffConfl);
  uint64_t next = ((uint64_t)20000/avg) + ((uint64_t)500/avg)*num;
  /* in the glucose paper they used 20000+500*num but I learn more clauses */
  assert(next > 0);
  if (diffConfl > next) {
    ++f->numSweeps;

    /* sort by lbd and delete half */
    qsort(f->learnedClauses.data, f->learnedClauses.size, sizeof(clause *), compareByLbdRev);

    fs_ifdbg (f, "sweep", 5) {
      uint64_t dupCount = 0;
      double lastActivity = 0.f;
      FILE *out = f->conf->debugStream;
      fprintf(out, "sorted:\n");
      clause **it;
      forVectorRev (clause **, it, &f->learnedClauses) {
        if ((*it)->activity == lastActivity) {
          dupCount++;
        } else {
          if (dupCount > 0) {
            fprintf(out, "(repeats %" PRIu64 " times) %f ", dupCount, (*it)->activity);
          } else {
            fprintf(out, "%f ", (*it)->activity);
          }
          dupCount = 0;
          lastActivity = (*it)->activity;
        }
      }
      if (dupCount > 0) {
        fprintf(out, "%f (repeats %" PRIu64 " times) ", lastActivity, dupCount);
      }

      fprintf(out, "done\n");
    }

    /* restore proper clause ids after sort. funcsatRemoveClause depends on
     * this. */
    for (unsigned i = 0; i < f->learnedClauses.size; i++) {
      clause *c = f->learnedClauses.data[i];
    }

    uint64_t numDeleted = 0;
    const uint64_t max = f->learnedClauses.size/2;
    fslog(f, "sweep", 1, "deleting at most %" PRIu64
          " clauses (of %u)\n", max, f->learnedClauses.size);
    for (variable i = 0; i < f->learnedClauses.size && f->learnedClauses.size > max; i++) {
      clause *c = f->learnedClauses.data[i];
      if (c->lbdScore == 2) continue;
      /* if (c->lbdScore < 3) break; */
      if (c->size > 2) {
        clause *rmClause = funcsatRemoveClause(f, c);
        if (rmClause) {
          clauseFree(c);
          --f->numLearnedClauses, ++f->numLearnedDeleted;
          numDeleted++;
        }
      }
    }
    fslog(f, "sweep", 1, ">>> deleted %" PRIu64 " clauses\n", numDeleted);

    num++;
    lastNumLearnedClauses = f->numLearnedClauses;
    lastNumConflicts = f->numConflicts;
  }
}


/* another strategy, from minisat 2.2.0 */
static int compareByActivityRev(const void *cl1, const void *cl2);

/* removes learned clauses, not including those that are currently the reason
 * for something else. */
void claActivitySweep(funcsat *f, void *user)
{
  UNUSED(user);
  static uint64_t num = 0, lastNumConflicts = 0, lastNumLearnedClauses = 0;
#if 0
  const uint64_t diffConfl = f->numConflicts - lastNumConflicts;
  const uint64_t diffLearn = f->numLearnedClauses - lastNumLearnedClauses;
  uint64_t size = f->learnedClauses.size;
  double extraLim = f->claInc / (size*1.f);
#endif

  if (f->numLearnedClauses*1.f >= f->maxLearned) {
    ++f->numSweeps;
    uint64_t numDeleted = 0;
    
    /* sort by activity and delete half */
    qsort(f->learnedClauses.data, f->learnedClauses.size, sizeof(clause *), compareByActivityRev);

    fs_ifdbg (f, "sweep", 5) {
      uint64_t dupCount = 0;
      int32_t lastLbd = LBD_SCORE_MAX;
      FILE *out = f->conf->debugStream;
      fprintf(out, "sorted:\n");
      clause **it;
      forVectorRev (clause **, it, &f->learnedClauses) {
        if ((*it)->lbdScore == lastLbd) {
          dupCount++;
        } else {
          if (dupCount > 0) {
            fprintf(out, "(repeats %" PRIu64 " times) %d ", dupCount, (*it)->lbdScore);
          } else {
            fprintf(out, "%d ", (*it)->lbdScore);
          }
          dupCount = 0;
          lastLbd = (*it)->lbdScore;
        }
      }
      if (dupCount > 0) {
        fprintf(out, "%d (repeats %" PRIu64 " times) ", lastLbd, dupCount);
      }

      fprintf(out, "done\n");
    }

    /* restore proper clause ids after sort. funcsatRemoveClause depends on
     * this. */
    for (unsigned i = 0; i < f->learnedClauses.size; i++) {
      clause *c = f->learnedClauses.data[i];
    }

    /* remove clauses with 0 activity */
#if 0
    for (variable i = 0; i < f->learnedClauses.size; i++) {
      clause *c = f->learnedClauses.data[i];
      if (c->activity == 0.f) {
        clause *rmClause = funcsatRemoveClause(f, c);
        if (rmClause) {
          clauseFree(c);
          --f->numLearnedClauses, ++f->numLearnedDeleted, ++numDeleted;
        }
      } else break;
    }
#endif
  
    const uint64_t max = f->learnedClauses.size/2;
    fslog(f, "sweep", 1, "deleting at most %" PRIu64
          " clauses (of %u)\n", max, f->learnedClauses.size);
    for (variable i = 0; i < f->learnedClauses.size && f->learnedClauses.size > max; i++) {
      clause *c = f->learnedClauses.data[i];
      if (c->size > 2) {
        clause *rmClause = funcsatRemoveClause(f, c);
        if (rmClause) {
          clauseFree(c);
          --f->numLearnedClauses, ++f->numLearnedDeleted, ++numDeleted;
        }
      }
    }
    fslog(f, "sweep", 1, ">>> deleted %" PRIu64 " clauses\n", numDeleted);

    num++;
    lastNumLearnedClauses = f->numLearnedClauses;
    lastNumConflicts = f->numConflicts;
  }
}

static void varDecayActivity(funcsat *f);
static void claDecayActivity(funcsat *f);
static void bumpClauseByActivity(funcsat *f, clause *c)
{
  /* bump clause activity */
  clause **cIt;
  if ((c->activity += f->claInc) > 1e20) {
    fslog(f, "sweep", 5, "rescale for activity %f\n", c->activity);
    /* rescale */
    forVector (clause **, cIt, &f->learnedClauses) {
      double oldActivity = (*cIt)->activity;
      (*cIt)->activity *= 1e-20;
      fslog(f, "sweep", 5, "setting activity from %f to %f\n", oldActivity, (*cIt)->activity);
    }
    double oldClaInc = f->claInc;
    f->claInc *= 1e-20;
    fslog(f, "sweep", 5, "setting claInc from %f to %f\n", oldClaInc, f->claInc);
  }
}

static void varDecayActivity(funcsat *f)
{
  f->varInc *= (1 / f->varDecay);
}
static void claDecayActivity(funcsat *f)
{
  double oldClaInc = f->claInc;
  f->claInc *= (1 / f->claDecay);
  fslog(f, "sweep", 9, "decaying claInc from %f to %f\n", oldClaInc, f->claInc);
}


void bumpReasonByActivity(funcsat *f, clause *c)
{
  bumpClauseByActivity(f, c);

  /* bump variable activity */
  literal *it;
  forClause (it, c) {
    varBumpScore(f, fs_lit2var(*it));
  }
}

void bumpLearnedByActivity(funcsat *f, clause *c)
{
  bumpClauseByActivity(f, c);

  /* bump variable activity */
  literal *it;
  forClause (it, c) {
    varBumpScore(f, fs_lit2var(*it));
    varBumpScore(f, fs_lit2var(*it));
  }

  if (--f->learnedSizeAdjustCnt == 0) {
    f->learnedSizeAdjustConfl *= f->learnedSizeAdjustInc;
    f->learnedSizeAdjustCnt    = (uint64_t)f->learnedSizeAdjustConfl;
    f->maxLearned             *= f->learnedSizeInc;
    fslog(f, "sweep", 1, "update: ml %f\n", f->maxLearned);
    
  }

  varDecayActivity(f);
  claDecayActivity(f);
}

@

@c
void bumpLearnedByLbd(funcsat *f, clause *c)
{
  c->lbdScore = computeLbdScore(f, c);

  /* bump variable activity */
  literal *it;
  forClause (it, c) {
    varBumpScore(f, fs_lit2var(*it));
  }
  varDecayActivity(f);
}

void bumpReasonByLbd(funcsat *f, clause *c)
{
  literal *it;
  forClause (it, c) {
    variable v = fs_lit2var(*it);
    clause *reason = getReason(f, (literal)v);
    if (reason && reason->lbdScore == 2) {
      varBumpScore(f, fs_lit2var(reason->data[0]));
    }
  }
  /* c->lbdScore = computeLbdScore(f, c); */
  /* bump variable activity */
  forClause (it, c) {
    varBumpScore(f, fs_lit2var(*it));
  }
}

void lbdDecayAfterConflict(funcsat *f)
{
  UNUSED(f);
  /* varDecayActivity(f); */
}

double funcsatDefaultStaticActivity(variable *v)
{
  UNUSED(v);
  return 1.f;
}

void lbdBumpActivity(funcsat *f, clause *c)
{
  for (variable i = 0; i < c->size; i++) {
    variable v = fs_lit2var(c->data[i]);
    clause *reason = getReason(f, (literal)v);
    if (reason && reason->lbdScore == 2) {
      varBumpScore(f, v);
    }
  }
}

void myDecayAfterConflict(funcsat *f)
{
  varDecayActivity(f);
  claDecayActivity(f);
}

void bumpUnitClauseByActivity(funcsat *f, clause *c)
{
  bumpClauseByActivity(f, c);
  assert(c->size > 0);
}

static void minimizeClauseMinisat1(funcsat *f, clause *learned)
{
  unsigned i, j;
  for (i = j = 1; i < learned->size; i++) {
    variable x = fs_lit2var(learned->data[i]);

    if (getReason(f, (literal)x) == NULL) learned->data[j++] = learned->data[i];
    else {
      clause *c = getReason(f, learned->data[i]);
      for (unsigned k = 1; k < c->size; k++) {
#if 0
        if (!seen[var(c[k])] && level(var(c[k])) > 0) {
          learned[j++] = learned[i];
          break;
        }
#endif
      }
    }
  }
  
}



/* utilities */
/************************************************************************/


variable resolveWithPos(funcsat *f, literal p, clause *learn, clause *pReason)
{
  variable i, c = 0;

  for (i = 0; i < pReason->size; i++) {
    literal q = pReason->data[i];
    if (f->litPos.data[fs_lit2var(q)] == UINT_MAX) {
      f->litPos.data[fs_lit2var(q)] = learn->size;
      clausePush(learn, pReason->data[i]);
      if (levelOf(f, fs_lit2var(q)) == (literal) f->decisionLevel) c++;
    }
  }
  /* remove -p from learn */
  i = f->litPos.data[fs_lit2var(p)];
  f->litPos.data[fs_lit2var(learn->data[learn->size-1])] = i;
  assert(!isAssumption(f, fs_lit2var(learn->data[f->litPos.data[fs_lit2var(p)]])));
  learn->data[f->litPos.data[fs_lit2var(p)]] = learn->data[--learn->size];
  f->litPos.data[fs_lit2var(p)] = UINT_MAX;

  return c;
}

static int compareByLbdRev(const void *cl1, const void *cl2)
{
  clause *c1 = *(clause **) cl1;
  clause *c2 = *(clause **) cl2;
  uint64_t s1 = c1->lbdScore, s2 = c2->lbdScore;
  if (s1 == s2) return 0;
  else if (s1 > s2) return -1;
  else return 1;
}

static int compareByActivityRev(const void *cl1, const void *cl2)
{
  clause *c1 = *(clause **) cl1;
  clause *c2 = *(clause **) cl2;
  double s1 = c1->activity, s2 = c2->activity;
  if (s1 == s2) return 0;
  else if (s1 > s2) return -1;
  else return 1;
}



@


@d LBD_SCORE_MAX 1000000
@d otherWatchIdx(watchIdx) ((watchIdx) == 0 ? 1 : 0)
   /*Gets the index of the other watch given the index of the current watch.*/
@d forEachWatchedClause(c, p, wi, nx, end, foundEnd)               
    if (c)                                                              
      for (wi = (c->data[0] == p ? 0 : 1), end = c->prev[wi], nx = c->next[wi], foundEnd = false; 
           !foundEnd;                                                   
           foundEnd = (c == end), c = nx, wi = (c->data[0] == p ? 0 : 1), nx = c->next[wi])
@d forEachClause(c, nx, end, foundEnd)                             
  if (c)                                                                
    for (end = c->prev[0], nx = c->next[0], foundEnd = false;           
         !foundEnd;                                                     
         foundEnd = (c == end), c = nx, nx = c->next[0])






@<Global def...@>=
unsigned int fsLitHash(void *lIn)
{
  /* TODO get better hash function */
  literal l = *(literal *) lIn;
  return (unsigned int) l;
}
int fsLitEq(void *a, void *b) {
  literal p = *(literal *) a, q = *(literal *) b;
  return p == q;
}
unsigned int fsVarHash(void *lIn)
{
  /* TODO get better hash function */
  literal l = *(literal *) lIn;
  return (unsigned int) fs_lit2var(l);
}
int fsVarEq(void *a, void *b) {
  literal p = *(literal *) a, q = *(literal *) b;
  return fs_lit2var(p) == fs_lit2var(q);
}



int litCompare(const void *l1, const void *l2)
{
  literal x = *(literal *) l1, y = *(literal *) l2;
  if (fs_lit2var(x) != fs_lit2var(y)) {
    return fs_lit2var(x) < fs_lit2var(y) ? -1 : 1;
  } else {
    if (x == y) {
      return 0;
    } else {
      return x < y ? -1 : 1;
    }
  }
}


#if 0
int clauseCompare(const void *cp1, const void *cp2)
{
  const clause *c = *(clause **) cp1, *d = *(clause **) cp2;
  if (c->size != d->size) {
    return c->size < d->size ? -1 : 1;
  } else {
    /* lexicographically compare */
    uint32_t i;
    for (i = 0; i < c->size; i++) {
      int ret;
      if (0 != (ret = litCompare(&c->data[i], &d->data[i]))) {
        return ret;
      }
    }
    return 0;
  }
}
#endif


void sortClause(clause *clause)
{
  qsort(clause->data, clause->size, sizeof(*clause->data), litCompare);
}


literal findLiteral(literal p, clause *clause)
{
  literal min = 0, max = clause->size-1, mid = -1;
  int res = -1;                 /* comparison */
  while (!(res == 0 || min > max)) {
    mid = min + ((max-min) / 2);
    res = litCompare(&p, &clause->data[mid]);
    if (res > 0) {
      min = mid + 1;
    } else {
      max = mid - 1;
    }
  } 
  return res == 0 ? mid : -1;
}

literal findVariable(variable v, clause *clause)
{
  literal min = 0, max = clause->size-1, mid = -1;
  int res = -1;                 /* comparison */
  while (!(res == 0 || min > max)) {
    mid = min + ((max-min) / 2);
    if (v == fs_lit2var(clause->data[mid])) {
      res = 0;
    } else {
      literal p = fs_var2lit(v);
      res = litCompare(&p, &clause->data[mid]);
    }
    if (res > 0) {
      min = mid + 1;
    } else {
      max = mid - 1;
    }
  } 
  return res == 0 ? mid : -1;
}



/* Low level clause manipulations. */
clause *clauseAlloc(uint32_t capacity)
{
  clause *c;
  MallocX(c, 1, sizeof(*c));
  clauseInit(c, capacity);
  return c;
}

void clauseInit(clause *v, uint32_t capacity)
{
  uint32_t c = capacity > 0 ? capacity : 4;
  CallocX(v->data, c, sizeof(*v->data));
  v->size = 0;
  v->capacity = c;
  v->isLearnt = false;
  v->lbdScore = LBD_SCORE_MAX;
  v->nx = NULL;
  v->is_watched = false;
  v->isReason = false;
  v->activity = 0.f;
}

void clauseFree(clause *v)
{
  clauseDestroy(v);
  free(v);
}

void clauseDestroy(clause *v)
{
  free(v->data);
  v->data = NULL;
  v->size = 0;
  v->capacity = 0;
  v->isLearnt = false;
  v->lbdScore = LBD_SCORE_MAX;
}

void clauseClear(clause *v)
{
  v->size = 0;
  v->lbdScore = LBD_SCORE_MAX;
  v->isLearnt = false;
}

void clausePush(clause *v, literal data)
{
  if (v->capacity <= v->size) {
    while (v->capacity <= v->size) {
      v->capacity = v->capacity*2+1;
    }
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  v->data[v->size++] = data;
}

void clausePushAt(clause *v, literal data, uint32_t i)
{
  uint32_t j;
  assert(i <= v->size);
  if (v->capacity <= v->size) {
    while (v->capacity <= v->size) {
      v->capacity = v->capacity*2+1;
    }
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  v->size++;
  for (j = v->size-(uint32_t)1; j > i; j--) {
    v->data[j] = v->data[j-1];
  }
  v->data[i] = data;
}


void clauseGrowTo(clause *v, uint32_t newCapacity)
{
  if (v->capacity < newCapacity) {
    v->capacity = newCapacity;
    ReallocX(v->data, v->capacity, sizeof(*v->data));
  }
  assert(v->capacity >= newCapacity);
}


literal clausePop(clause *v)
{
  assert(v->size != 0);
  return v->data[v->size-- - 1];
}

literal clausePopAt(clause *v, uint32_t i)
{
  uint32_t j;
  assert(v->size != 0);
  literal res = v->data[i];
  for (j = i; j < v->size-(uint32_t)1; j++) {
    v->data[j] = v->data[j+1];
  }
  v->size--;
  return res;
}

literal clausePeek(clause *v)
{
  assert(v->size != 0);
  return v->data[v->size-1];
}

void clauseSet(clause *v, uint32_t i, literal p)
{
  v->data[i] = p;
}

void clauseCopy(clause *dst, clause *src)
{
  uintptr_t i;
  for (i = 0; i < src->size; i++) {
    clausePush(dst, src->data[i]);
  }
  dst->isLearnt = src->isLearnt;
  dst->lbdScore = src->lbdScore;
}



@

@<Global def...@>=
/**
 * Generates a clause that is 
 *   1) satisfied if a given literal violates the current assignment and
 *   2) falsified only under the current assignment.
 */
clause *funcsatSolToClause(funcsat *f) {
  clause *c = clauseAlloc(f->trail.size);
  for(unsigned i = 0; i < f->trail.size; i++)
    clausePush(c, -f->trail.data[i]);
  return c;
}

funcsat_result funcsatFindAnotherSolution(funcsat *f) {
  clause *cur_sol = funcsatSolToClause(f);
  funcsat_result res = funcsatAddClause(f, cur_sol);
  if(res == FS_UNSAT) return FS_UNSAT;
  res = funcsatSolve(f);
  return res;  
}

int funcsatSolCount(funcsat *f, clause subset, clause *lastSolution)
{
  assert(f->assumptions.size == 0);
  int count = 0;
  for (unsigned i = 0; i < subset.size; i++) {
    funcsatResize(f, fs_lit2var(subset.data[i]));
  }
 
  clause assumptions;
  clauseInit(&assumptions, subset.size);

  unsigned twopn = (unsigned) round(pow(2., (double)subset.size));
  fslog(f, "countsol", 1, "%u incremental problems to solve\n", twopn);
  for (unsigned i = 0; i < twopn; i++) {
    fslog(f, "countsol", 2, "%u: ", i);
    clauseClear(&assumptions);
    clauseCopy(&assumptions, &subset);

    /* negate literals that are 0 in n. */
    unsigned n = i;
    for (unsigned j = 0; j < subset.size; j++) {
        /* 0 bit */
      if ((n % 2) == 0) assumptions.data[j] *= -1;
      n >>= 1;
    }

    if (funcsatPushAssumptions(f, &assumptions) == FS_UNSAT) {
      continue;
    }

    if (FS_SAT == funcsatSolve(f)) {
      count++;
      if (lastSolution) {
        clauseClear(lastSolution);
        clauseCopy(lastSolution, &f->trail);
      }
    }
    
    funcsatPopAssumptions(f, f->assumptions.size);
  }

  clauseDestroy(&assumptions);

  return count;
}


@*1 Rest.

@<Internal decl...@>=

static void finishSolving(funcsat *func);
static bool bcpAndJail(funcsat *f);


bool funcsatLubyRestart(funcsat *f, void *p);
bool funcsatNoRestart(funcsat *, void *);
bool funcsatInnerOuter(funcsat *f, void *p);
bool funcsatMinisatRestart(funcsat *f, void *p);
void parseDimacsCnf(const char *path, funcsat *f);

/**
 * Undoes the given set of unit assumptions.  Assumes the decision level is 0.
 */
void undoAssumptions(funcsat *func, clause *assumptions);

funcsat_result startSolving(funcsat *f);

/**
 * Adds a clause containing no duplicate literals to the problem.
 *
 * @@pre decision level is 0
 */
funcsat_result addClause(funcsat *f, clause *c);

/**
 * Analyses conflict, produces learned clauses, backtracks, and asserts the
 * learned clauses.  If returns false, this means the SAT problem is unsat; if
 * it returns true, it means the SAT problem is not known to be unsat.
 */
bool analyze_conflict(funcsat *func);

/**
 * Undoes the trail and assignments so that the new decision level is
 * ::newDecisionLevel.  The lowest decision level is 0.
 *
 * @@param func
 * @@param newDecisionLevel
 * @@param isRestart
 * @@param facts same as in ::trailPop
 */
void backtrack(funcsat *func, variable newDecisionLevel, head_tail *facts, bool isRestart);




/* Mutators */

static void fs_print_state(funcsat *, FILE *);
void funcsatPrintConfig(FILE *f, funcsat *);


bool funcsatIsResourceLimitHit(funcsat *, void *);
funcsat_result funcsatPreprocessNewClause(funcsat *, void *, clause *);
funcsat_result funcsatPreprocessBeforeSolve(funcsat *, void *);
variable funcsatLearnClauses(funcsat *, void *);

int varOrderCompare(fibkey *, fibkey *);
double funcsatDefaultStaticActivity(variable *v);
void lbdBumpActivity(funcsat *f, clause *c);

void bumpReasonByActivity(funcsat *f, clause *c);
void bumpLearnedByActivity(funcsat *f, clause *c);
void bumpReasonByLbd(funcsat *f, clause *c);
void bumpLearnedByLbd(funcsat *f, clause *c);
void bumpOriginal(funcsat *f, clause *c);
void bumpUnitClauseByActivity(funcsat *f, clause *c);

void singlesPrint(FILE *stream, clause *begin);


bool watcherFind(clause *c, clause **watches, uint8_t w);
void watcherPrint(FILE *stream, clause *c, uint8_t w);
void singlesPrint(FILE *stream, clause *begin);
void binWatcherPrint(FILE *stream, funcsat *f);


/**
 * FOR DEBUGGING
 */
bool isUnitClause(funcsat *f, clause *c);



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
literal findLiteral(literal l, clause *);
/**
 * Same as ::findLiteral but works on variables.
 */
literal findVariable(variable l, clause *);

unsigned int fsLitHash(void *);
int fsLitEq(void *, void *);
int litCompare(const void *l1, const void *l2);
typedef struct litpos
{
  clause *c;
  posvec *ix;
} litpos;



@

@<Global def...@>=

extern inline mbool funcsatValue(funcsat *f, literal p)
{
  variable v = fs_lit2var(p);                         
  if (f->level.data[v] == Unassigned) return unknown; 
  literal value = f->trail.data[f->model.data[v]];    
  return p == value;
}

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
  UNUSED(pos);
  setLitPos(f, l, UINT_MAX);
}

static inline bool hasLitPos(funcsat *f, literal l)
{
  return getLitPos(f,l) != UINT_MAX;
}

static inline void spliceUnitFact(funcsat *f, literal l, clause *c)
{
  head_tail_add(&f->unit_facts[fs_lit2var(l)], c);
}

@
@<Internal decl...@>=
static inline bool hasLitPos(funcsat *f, literal l);
static inline clause *getReason(funcsat *f, literal l);
static inline void spliceUnitFact(funcsat *f, literal l, clause *c);
static inline unsigned getLitPos(funcsat *f, literal l);


@ Prototype.
@<External decl...@>=
funcsat_result fs_parse_dimacs_solution(funcsat *f, FILE *solutionFile);


@ Parse solution.
@<Global def...@>=

static char parse_read_char(funcsat *f, FILE *solutionFile) {
  UNUSED(f);
  char c;
  c = fgetc(solutionFile);
#if 0
  bf_log(b, "bf", 8, "Read character '%c'\n", c);
#endif
  return c;
}

static const char *s_SATISFIABLE = "SATISFIABLE\n";
static const char *s_UNSATISFIABLE = "UNSATISFIABLE\n";

funcsat_result fs_parse_dimacs_solution(funcsat *f, FILE *solutionFile)
{
  char c;
  literal var;
  bool truth;
  funcsat_result result = FS_UNKNOWN;
  bool have_var = false;
  const char *cur;
state_new_line:
  while (true) {
    c = parse_read_char(f, solutionFile);
    switch (c) {
      case EOF:
        goto state_eof;
      case 'c':
        goto state_comment;
      case 's':
        goto state_satisfiablility;
      case 'v':
        goto state_variables;
      default:
        fslog(f, "fs", 1, "unknown line type '%c' in solution file\n", c);
        goto state_error;
    }
  }
state_comment:
  while (true) {
    c = parse_read_char(f, solutionFile);
    switch (c) {
      case EOF:
        goto state_eof;
      case '\n':
        goto state_new_line;
      default:
        break;
    }
  }
state_satisfiablility:
  cur = NULL;
  funcsat_result pending = FS_UNKNOWN;
  while (true) {
    c = parse_read_char(f, solutionFile);
    switch (c) {
      case ' ':
      case '\t':
        continue;
      case EOF:
        goto state_eof;
      default:
        ungetc(c, solutionFile);
        break;
    }
    break;
  }
  while (true) {
    c = parse_read_char(f, solutionFile);
    if (cur == NULL) {
      switch (c) {
        case 'S':
          cur = &s_SATISFIABLE[1];
          pending = FS_SAT;
          break;
        case 'U':
          cur = &s_UNSATISFIABLE[1];
          pending = FS_UNSAT;
          break;
        case EOF:
          goto state_eof;
        default:
          fslog(f, "fs", 1, "unknown satisfiability\n");
          goto state_error;
      }
    } else {
      if (c == EOF) {
        goto state_eof;
      } else if (c != *cur) {
        fslog(f, "fs", 1, "reading satisfiability, got '%c', expected '%c'\n", c, *cur);
        goto state_error;
      }
      if (c == '\n') {
        result = pending;
        goto state_new_line;
      }
      ++cur;
    }
  }
state_variables:
  while (true) {
    c = parse_read_char(f, solutionFile);
    switch (c) {
      case '\n':
        goto state_new_line;
      case ' ':
      case '\t':
        break;
      case EOF:
        goto state_eof;
      default:
        ungetc(c, solutionFile);
        goto state_sign;
    }
  }
state_sign:
  have_var = false;
  truth = true;
  c = parse_read_char(f, solutionFile);
  if (c == '-') {
    truth = false;
  } else {
    ungetc(c, solutionFile);
  }
  var = 0;
  goto state_variable;
state_variable:
  while (true) {
    c = parse_read_char(f, solutionFile);
    switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        have_var = true;
        var = var * 10 + (c - '0');
        break;
      default:
        ungetc(c, solutionFile);
        goto state_record_variable;
    }
  }
state_record_variable:
  if (have_var) {
    if (var == 0) {
      /* sometimes the SAT solver will be told ``there are 47000 variables but
       * I'm only going to use 100 of them in the instance.'' {\bf some solvers}
       * (lingeling) will say ``ah well i can get rid of 46000 variables! i have
       * been so helpful today!'' which is all well and good except that then it
       * outputs the solution on only the 100 of them that are used. in this
       * case, we log what happened and hope that giving an arbitrary assignment
       * (false) to the rest of the inputs is the right thing. */

      /* we assume this doesn't happen */

      goto state_exit;
    } else {
      variable v = fs_lit2var(var);
      funcsatPushAssumption(f, (truth ? (literal)v : -(literal)v));
    }
    goto state_variables;
  } else {
    abort();
    fslog(f, "fs", 1, "expected variable, but didn't get one\n");
    goto state_error;
  }
state_eof:
  if (ferror(solutionFile)) {
    abort();
    fslog(f, "fs", 1, "IO error reading solution file: %s", strerror(errno));
    goto state_error;
  } else {
    goto state_exit;
  }
state_error:
  result = FS_UNKNOWN;
  goto state_exit;
state_exit:
  return result;
}
@
@(funcsat_test.c@>=
#include "funcsat.h"
