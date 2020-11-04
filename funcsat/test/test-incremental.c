#include "funcsat/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "funcsat.h"
#include "funcsat/types.h"
#include "funcsat/internal.h"
#include "funcsat/clause.h"
#include "funcsat/frontend.h"
#include "funcsat/learning.h"
#include "genrand.h"

#define CLAUSE_NUM 1
#define CLAUSE_DENOM 5000

#define CONFIG_NUM 1
#define CONFIG_DENOM 5000

#ifdef NDEBUG

#define check(b) if (!(b)) { \
  fprintf(stderr, "%s:%d\n", __FILE__, __LINE__); \
  assert(b); exit(1); }
#define checkmsg(b, msg) if (!(b)) { \
  fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, msg); \
  assert(b && msg); exit(1); }

#else
#define check(b) assert(b)
#define checkmsg(b, msg) assert(b && msg)

#endif

extern funcsat_config funcsatDefaultConfig;

clause incrNilAssumps;

// TODO MAL use forVector when it gets written

static bool checkForUnknown(funcsat_result test)
{
  if (FS_UNKNOWN == test) {
    fprintf(stderr, "Odd. One side returned unknown.\n");
    return true;
  }
  return true;
}

static void incrMakeAssgnAssump(funcsat *f, clause* assgn)
{
  clauseClear(assgn);
  for (variable v = 1; v < f->numVars; v++) {
    literal l = funcsatValue(f, fs_var2lit(v)) == true ? fs_var2lit(v) : -1 * fs_var2lit(v);
    funcsatPushAssumption(f, l);
  }
}

static clause *incrClauseCopy(clause *src) 
{
  /* Caller is responsible for handling clauseRelease on this guy */
  clause *dst = clauseAlloc(src->size);
  clauseCopy(dst, src);
  return dst;
}

/** 
 * Here's the idea: 
 * If the incremental test got UNSAT, there shouldn't be an assignment
 * Then golden with no assignments on the same clauses should still be UNSAT
 * If the incremental test got SAT, the assignment is what it assigned
 * Then golden under assumptions for that assignment should still be SAT
 **/
static bool incrTestSoln(funcsat *f,
                           vector *clauses,
                           uint32_t subset)
{
  funcsat_result expected = f->lastResult;
  bool res = true;
  // TODO MAL may need to be able to deal with timeouts
  res = res && checkForUnknown(expected);
  check(res);

  funcsat *golden = funcsatInit(f->conf);
  /* Add clauses to golden up to length subset from allClauses */
  for (uint32_t cnum = 0; cnum < subset; cnum++) {
    clause *cnew = incrClauseCopy(clauses->data[cnum]);
    funcsatAddClause(golden, cnew);
  }

  funcsat_result golres;
  if (FS_SAT == expected) {
    /* If SAT, make the incrementally-solved assignment into assumptions */
    clause assignment;
    clauseInit(&assignment, funcsatNumVars(f));
    incrMakeAssgnAssump(f, &assignment);
    golres = funcsatSolve(golden);
    funcsatPopAssumptions(f, f->assumptions.size);
  } else {
    /* If UNSAT, solve with no assumptions */
    golres = funcsatSolve(golden);
  }

  // TODO MAL may need to be able to deal with timeouts
  res = res && checkForUnknown(golres);
  check(res);
  res = res && (golres == expected);
  // TODO MAL adjust configuration of golden to get this printed
  dmsg(golden, "testInc", 3, true, "GoldenResult: %d ExpectedResult: %d \n", 
       golres, expected);
  check(res);

  funcsatDestroy(golden);
  return res;
}

/** 
 * Here's the idea: 
 * If the incremental test got UNSAT, check that there's a conflict clause
 * More proof-based verification can go in here later
 * If the incremental test got SAT, the assignment is what it assigned
 * Make sure every variable is assigned a value
 * Walk through each clause and make sure that every one is satisfied
 * under the assignment
 * Make sure all the assumptions are validly met, too
 * Caveat: I only check that the assumptions sitting in f are met;
 * a different check should make sure that any assumptions we expect
 * on any given run are included in the assignment or as assumptions in f
 **/
static bool incrVerifySoln(funcsat *f,
                             vector *clauses,
                             uint32_t subset) 
{
  funcsat_result expected = f->lastResult;
  bool res = true;

  checkmsg(FS_UNKNOWN != expected, 
           "ERROR: Returned Unknown; can't verify solution.");

  // TODO MAL may need to be able to deal with timeouts
  if (FS_UNSAT == expected) {
    /* If UNSAT, verify conflict clause exists (non-NULL) for now */
    check(NULL != f->conflictClause);

  } else {
    /* If SAT, ensure that all variables in the solver are assigned */
    for (variable v = 1; v < f->numVars; v++) {
      check(unknown != funcsatValue(f, fs_var2lit(v)));
    }

    /* Verify that all clauses are satisfied by assignment */
    for (uint32_t cnum = 0; cnum < subset; cnum++) {
      bool cres = false;
      clause *curr = clauses->data[cnum];
      for (uint32_t litnum = 0; 
           litnum < curr->size;
           litnum++) {
        mbool val = funcsatValue(f, curr->data[litnum]);
        /* Mark that the clause is satisfied */
        cres = cres || (true == val);
      }
      /* Fail if the clause wasn't satisfied */
      checkmsg(cres, "Found unsatisfied clause");
    }

    /* Also, verify that all assumptions are in the assignment */
    if (0 != f->assumptions.size) {
      for (uint32_t anum = 0; anum < f->assumptions.size; anum++) {
        check(true == funcsatValue(f, f->assumptions.data[anum]));
      }
    }
  }
  return res;
}


static vector *incrGetFullClauses(const char *srcPath)
{
  /** Depending on type of test we're running, get all the clauses we'll
   * eventually need:
   * 1) Fuzzing: get random problem from system/fuzzsat
   * 2) Read in known problem from somewhere (test dir?)
   **/

  if (NULL == srcPath) {
    check(srcPath);
    // TODO MAL call system and get from fuzzsat, 
    // then pass that path to next part?
  }

  //TODO MAL make sure that the default config does minimal preprocessing
  // on the clauses that we're going to read in now.
  funcsat_config conf;
  memcpy(&conf, &funcsatDefaultConfig, sizeof(conf));
  funcsat *ft = funcsatInit(&conf);
  parseDimacsCnf(srcPath, ft);

  vector *ret = malloc(sizeof(vector));
  vectorInit(ret, ft->origClauses.size);

  for (unsigned cnum = 0; cnum < ft->origClauses.size; cnum++) {
    vectorPush(ret, incrClauseCopy(ft->origClauses.data[cnum]));
  }

  /* See if the solver can solve the full problem */
  funcsat_result testret = funcsatSolve(ft);
  check(FS_UNKNOWN != testret);
  /* For now, exit if timeouts; maybe deal differently later */

  funcsatDestroy(ft);

  return ret;
}

static vector *incrKillFullClauses(vector *clauses)
{
  for (unsigned i = 0; i < clauses->size; i++) {
    clause *c = clauses->data[i];
  }
  vectorDestroy(clauses);

  return NULL;
}

static void incrRandomizeClauses(vector *vec) 
{
  for (uint32_t ub = vec->size; ub > 1; ub--) {
    long int idx = randomPosNum(ub);
    void *clause = vectorPopAt(vec, (unsigned)(idx-1));
    vectorPushAt(vec, clause, (ub-1));
  }
}

//TODO MAL return value here - to set up for multi-fail tests?
static void incrSolveSubset(funcsat *f,
                            vector *clauses, 
                            uint32_t subset_start,
                            uint32_t subset_end)
{

  for (uint32_t cnum = subset_start; cnum < subset_end; cnum++) {
    clause *cnew = incrClauseCopy(clauses->data[cnum]);
    funcsatAddClause(f, cnew);
  }

  funcsat_result result = funcsatSolve(f);
  check(result == f->lastResult);

  incrVerifySoln(f, clauses, subset_end);
  incrTestSoln(f, clauses, subset_end);

}

// TODO MAL numUipsToLearn should be UINT32_MAX to match main?
// TODO MAL ssr help says "on by default"
  /** Other input options on command line that maybe shoudl be tested
   *  timeout //Global Timeout set
   *  live-stats on
   *  graph-stats off
   *  nosolprint (just the result)
   *  count-solutions (on variable subset)
   *  assume (the given literals)
   *  TODO MAL what about debug labels?  doesn't seem unreasonable...
   *  name (= basename(filename))
   **/

/** Return whether we modified the conf
 *  If we're not at the last one, just increment and return 
 *  If we are and wrap is true, go back to the first one
 *  If we are and wrap is false, don't change it
 **/
#define confIterMacro(cimVals, cimSz, cimFld, cimWrap, cimInit, cimChngd) {  \
    if (true == cimInit) { cimFld = cimVals[0]; cimChngd = false;       \
    } else {                                                            \
      uint32_t cimIdx_1 = 0;                                            \
      while (cimIdx_1 < cimSz && cimFld != cimVals[cimIdx_1++]);        \
      if (cimIdx_1 != cimSz) {                                          \
        cimFld = cimVals[cimIdx_1];                                     \
        cimChngd = true;                                                \
      } else if (true == cimWrap) {                                     \
        cimFld = cimVals[0];                                            \
        cimChngd = true;                                                \
      }                                                                 \
    }                                                                   \
  }
  

static bool confIterPassThrough(funcsat_config *currConf, 
                                bool wrap,
                                bool init)
{
  return false;
}

static bool confIterGC(funcsat_config *currConf, 
                       bool wrap,
                       bool init)
{
  uint32_t size = 3;
  //bool (*confIter) (funcsat_config *, bool, bool);
  void(*vals[3])(funcsat*, void*) = 
    {lbdSweep, claActivitySweep, NULL};
  bool changed = false;

  confIterMacro(vals, size, currConf->sweepClauses, wrap, init, changed);
  if (NULL == currConf->sweepClauses) {
    currConf->gc = false;
  } else {
    currConf->gc = true;
  }
  return changed;
}

static bool confIterJail(funcsat_config *currConf, 
                         bool wrap,
                         bool init)
{
  // TODO Check with Denis on values 
  // TODO how to see how far it's gone - if it learned any more 
  // to be able to increment on a per-problem basis?
  unsigned vals[] = 
    {0, 1, 2, 9, 50, 128, UINT_MAX};
  uint32_t size = sizeof(vals)/sizeof(unsigned);
  bool changed = false;

  confIterMacro(vals, size, currConf->maxJailDecisionLevel, wrap, init, changed);
  return changed;
}

static bool confIterUips(funcsat_config *currConf, 
                         bool wrap,
                         bool init)
{
  // TODO Check with Denis on values 
  // TODO how to see how far it's gone - if it learned any more 
  // to be able to increment on a per-problem basis?
  uint32_t vals[] =
    {UINT_MAX, 0, 1, 2, 5, 9};
  uint32_t size = sizeof(vals)/sizeof(uint32_t);
  bool changed = false;

  confIterMacro(vals, size, currConf->numUipsToLearn, wrap, init, changed);
  return changed;
}

static bool confIterRestart(funcsat_config *currConf, 
                            bool wrap,
                            bool init)
{
  uint32_t size = 4;
  bool(*vals[4])(funcsat*, void*) = 
    {funcsatLubyRestart, funcsatNoRestart, funcsatInnerOuter, 
     funcsatMinisatRestart};
  bool changed = false;

  confIterMacro(vals, size, currConf->isTimeToRestart, wrap, init, changed);
  return changed;
}

static bool confIterSSR(funcsat_config *currConf, 
                        bool wrap,
                        bool init)
{
  uint32_t size = 2;
  bool vals[2] = {false, true};
  bool changed = false;

  confIterMacro(vals, size, currConf->useSelfSubsumingResolution, wrap, init, changed);
  return changed;
}

static bool confIterMinLC(funcsat_config *currConf, 
                          bool wrap,
                          bool init)
{
  uint32_t size = 2;
  bool vals[2] = {false, true};
  bool changed = false;

  confIterMacro(vals, size, currConf->minimiseLearnedClauses, wrap, init, changed);
  return changed;
}

static bool confIterPhaseSave(funcsat_config *currConf, 
                              bool wrap,
                              bool init)
{
  uint32_t size = 2;
  bool vals[2] = {false, true};
  bool changed = false;

  confIterMacro(vals, size, currConf->usePhaseSaving, wrap, init, changed);
  return changed;
}

/* vector of function pointers */
static vector *confIterFuncs;

static void confIterInit()
{
  confIterFuncs = malloc(sizeof(*confIterFuncs));
  //TODO MAL compile-time changeable constant?
  vectorInit(confIterFuncs, 15);

//TODO MAL mumble()
  vectorPush(confIterFuncs, &confIterPhaseSave);

  /* vectorPush(confIterFuncs, &confIterSSR); */
  vectorPush(confIterFuncs, &confIterPassThrough);
  vectorPush(confIterFuncs, &confIterMinLC);

  vectorPush(confIterFuncs, &confIterGC);
  vectorPush(confIterFuncs, &confIterRestart);

  vectorPush(confIterFuncs, &confIterUips);
  /* vectorPush(confIterFuncs, &confIterPassThrough); */
  vectorPush(confIterFuncs, &confIterJail);

  /* vectorPush(confIterFuncs, &confIterShowProof); */

  while (confIterFuncs->size < 15) {
    vectorPush(confIterFuncs, &confIterPassThrough);
  }
}

static funcsat_config *incrGenNextConf(funcsat_config *currConf,
                                       bool init)
{
  funcsat_config *curr;
  curr = malloc(sizeof(funcsat_config));
  memcpy(curr, currConf, sizeof(funcsat_config));

  bool (*confIter) (funcsat_config *, bool, bool);
  int32_t i;
  
  if (true == init) {
    /* Get the initial conf */
    for (i = 0; i < (int32_t)confIterFuncs->size; i++) {
      confIter = confIterFuncs->data[i];
      confIter(curr, false, true);
    }      
    return curr;
  }

  bool found;
  for (i = 0, found = false; 
       i < (int32_t)confIterFuncs->size && false == found; 
       i++){
    confIter = confIterFuncs->data[i];
    found = confIter(curr, false, false);
  }    
  /* Now i is the one after the one that succeeded in changing the conf */
  if (false == found) {
    /* We didn't find one to change */
    return NULL;
  }
  check(0 != i);
  /* Wrap everything before the one that changed, ignoring returns */
  for (i = i -2; i >= 0; i--) {
    confIter = confIterFuncs->data[i];
    confIter(curr, true, false);
  }

  return curr;
}

//TODO MAL return value here - to set up for multi-fail tests?
static void incrIterSolve(funcsat_config *confin, 
                          vector *allClauses) 
{
  funcsat_config confcp;
  bool doall;
  /** Make sure the copy that the solver gets is changeable 
   * and initially valid 
   **/
  if (NULL == confin) {
    doall = true;
    memcpy(&confcp, &funcsatDefaultConfig, sizeof(confcp));
  } else {
    doall = false;
    memcpy(&confcp, confin, sizeof(confcp));
  }

  /* funcsat *f = funcsatInit(&confcp, NULL); */
  /* for (uint32_t ssnum = 0; ssnum < allClauses->size; ssnum++) { */
  /*   // TODO more random subsets; merge in previous function? */
  /*   dmsg(f, "testInc", 2, true, "* testIter before %d\n", ssnum+1); */
  /*   incrSolveSubset(f, allClauses, ssnum, ssnum+1, &incrNilAssumps); */
  /* } */

  /* funcsatDestroy(f); */

  // TODO verify consistency across all runs?

  /* Iterate through configurations */
  //TODO allow to not do all configs - random? specify?
  for (funcsat_config *curr = incrGenNextConf(&confcp, doall);
       NULL != curr; 
       curr = incrGenNextConf(&confcp, false)) {

    /* Make sure the copy that the solver gets is changeable */
    memcpy(&confcp, curr, sizeof(confcp));
    free(curr);
    curr = NULL;

    if (randomPosNum(CONFIG_DENOM) > CONFIG_NUM) {
      // Keep iterating until we get a new config to test
      // Generates a random number no more than ub and greater than 0.
      /* fprintf(stderr, "Skipping\n"); */
      continue;
    }

    funcsat *f = funcsatInit(&confcp);  
    /* fprintf(stderr, "Not Skipping:\n"); */
    funcsatPrintConfig(stderr, f);

    /* Iterate through randomly adding clauses */
    uint32_t start = 0;
    for (uint32_t ssnum = 0; ssnum < allClauses->size; ssnum++) {
      if (randomPosNum(CLAUSE_DENOM) > CLAUSE_NUM) {
        // Keep iterating until we get a new config to test
        // Generates a random number no more than ub and greater than 0.
        continue;
      }

      dmsg(f, "testInc", 2, true, "* testIter from %d to before %d\n", start, ssnum+1);
      incrSolveSubset(f, allClauses, start, ssnum+1);
      start = ssnum;
    }

    funcsatDestroy(f);
  }

}


static void incrTestInit() {
  time_t seedt;
  clauseInit(&incrNilAssumps, 0); 
  time(&seedt);
  uint32_t seed = (uint32_t) seedt;
  fprintf(stderr, "seed is %d\n", seed);
  srandom(seed);

  //TODO MAL adjustable
  confIterInit();
}

static vector *incrIterInit(const char *srcPath) 
{
  fprintf(stderr, "  * testWholeProblem %s\n", srcPath);
  vector *allClauses = incrGetFullClauses(srcPath);
  incrRandomizeClauses(allClauses);

  return allClauses;
}

static void incrIterPost(vector *allClauses) 
{
  // TODO MAL copy this down here (inline)
  incrKillFullClauses(allClauses);
}

int main(int argc, char **argv)
{
  /* uint32_t numTests = 10; */
  /* unsigned maxLen = 6, maxVar = 12; */
  int ret = 0;
  funcsat_config *conf = funcsatConfigInit(NULL);
  funcsat *f = funcsatInit(conf);

  /* MAL's tests begin here */
  incrTestInit();
  
  vector *allClauses = 
  /*   incrIterInit("/moonlight/internet/software/sat/problems/blocksworld/bw_large.a.cnf.gz");  */
  /* incrIterSolve(NULL, allClauses); */
  /* incrIterPost(allClauses); */

  /* allClauses =  */
  /*   incrIterInit("/moonlight/internet/software/sat/problems/blocksworld/anomaly.cnf.gz");  */
  /* incrIterSolve(NULL, allClauses); */
  /* incrIterPost(allClauses); */

    /** TODO MAL from NO phsv	bump	NO ssr	NO min	gc minisat	restart luby	NO proof
learn 4294967295 uips		Jail up to 0 uips
test-incremental: funcsat.c:142: funcsatDestroy: Assertion `clause->refCnt == 3' failed. **/

  /* allClauses =  */
  /*   incrIterInit("/moonlight/internet/software/sat/problems/blocksworld/bw_large.b.cnf.gz");  */
  /* incrIterSolve(NULL, allClauses); */
  /* incrIterPost(allClauses); */

  /* allClauses =  */
/*     incrIterInit("/moonlight/internet/software/sat/problems/blocksworld/medium.cnf.gz");  */
/*   incrIterSolve(NULL, allClauses); */
/*   incrIterPost(allClauses); */

/*   * testWholeProblem /moonlight/internet/software/sat/problems/blocksworld/bw_large.d.cnf.gz */
/* NO phsv	NO bump	NO ssr	NO min	gc glucose	restart luby	NO proof */
/* learn 4294967295 uips		Jail up to 0 uips */
/* /bin/sh: line 5: 20265 Killed                  ${dir}$tst */
/* FAIL: test-incremental */
/*   allClauses =  */
/*     incrIterInit("/moonlight/internet/software/sat/problems/blocksworld/bw_large.d.cnf.gz");  */
/*   incrIterSolve(NULL, allClauses); */
/*   incrIterPost(allClauses); */

/*   allClauses =  */
    incrIterInit("/sunlight/internet/software/sat/instances/industrial/babic/hsatv17/hsat_vc11773.cnf.gz");
  incrIterSolve(NULL, allClauses);
  incrIterPost(allClauses);

  /* allClauses =  */
  /*   incrIterInit("/moonlight/internet/software/sat/problems/blocksworld/bw_large.c.cnf.gz");  */
  /* incrIterSolve(NULL, allClauses); */
  /* incrIterPost(allClauses); */


  /* ret = ret || testGenClauseSorted(numTests, maxLen, maxVar); */
  /* ret = ret || testInsertFind(numTests, maxLen, maxVar); */

  fprintf(stderr, "done\n");
  funcsatDestroy(f);
  funcsatConfigDestroy(conf);

  return ret;
}
 
