/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <bitfunc.h>
#include <bitfunc/debug.h>
#include <bitfunc/bitvec.h>
#include <bitfunc/array.h>
#include <bitfunc/circuits.h>
#include <bitfunc/circuits_internal.h>
#include <bitfunc/bitfunc_internal.h>
#include <funcsat.h>
#include <funcsat/hashtable_itr.h>
#include <funcsat_internal.h>
#include <unistd.h>
#include <time.h>

#ifdef INCREMENTAL_PICOSAT_ENABLED
#include <picosat.h>
#endif

#include "bfprobes.h"

//extern bitvec *bfvAdd0(bfman *m, bitvec *x, bitvec *y);
extern variable fs_lit2var(literal l);
/** \todo this can be lazy */
extern void dimacsPrintClause(FILE *f, clause *clause);

static const char *s_SATISFIABLE = "SATISFIABLE\n";
static const char *s_UNSATISFIABLE = "UNSATISFIABLE\n";

static char parse_read_char(bfman *b, FILE *solutionFile) {
  char c;
  c = fgetc(solutionFile);
  bf_log(b, "bf", 8, "Read character '%c'\n", c);
  return c;
}

/* parses a solution and stores it in the current aigAssignment. */
static bfresult bf_parse_dimacs_solution(bfman *b, FILE *solutionFile)
{
  char c;
  literal var;
  bool truth;
  bfresult result = BF_UNKNOWN;
  bool have_var = false;
  const char *cur;
  bf_prep_assignment(b);
  bf_clear_assignment(b);
state_new_line:
  while (true) {
    c = parse_read_char(b, solutionFile);
    switch (c) {
      case EOF:
        goto state_eof;
      case 'c':
        goto state_comment;
      case 's':
        goto state_satisfiablility;
      case 'v':
        goto state_variables;
      case '\n':
        break; /* just ignore lone newlines - technically non conforming */
      default:
        bf_log(b, "bf", 1, "unknown line type '%c' in solution file\n", c);
        goto state_error;
    }
  }
state_comment:
  while (true) {
    c = parse_read_char(b, solutionFile);
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
  bfresult pending = BF_UNKNOWN;
  while (true) {
    c = parse_read_char(b, solutionFile);
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
    c = parse_read_char(b, solutionFile);
    if (cur == NULL) {
      switch (c) {
        case 'S':
          cur = &s_SATISFIABLE[1];
          pending = BF_SAT;
          break;
        case 'U':
          cur = &s_UNSATISFIABLE[1];
          pending = BF_UNSAT;
          break;
        case EOF:
          goto state_eof;
        default:
          bf_log(b, "bf", 1, "unknown satisfiability\n");
          goto state_error;
      }
    } else {
      if (c == EOF) {
        goto state_eof;
      } else if (c != *cur) {
        bf_log(b, "bf", 1, "reading satisfiability, got '%c', expected '%c'\n", c, *cur);
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
    c = parse_read_char(b, solutionFile);
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
  c = parse_read_char(b, solutionFile);
  if (c == '-') {
    truth = false;
  } else {
    ungetc(c, solutionFile);
  }
  var = 0;
  goto state_variable;
state_variable:
  while (true) {
    c = parse_read_char(b, solutionFile);
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
      /* sometimes the SAT solver will be told 'there are 47000 variables but
       * I'm only going to use 100 of them in the instance.' _some_ solvers
       * (lingeling) will say 'ah well i can get rid of 46000 variables! i have
       * been so helpful today!' which is all well and good except that then it
       * outputs the solution on only the 100 of them that are used. so this
       * means b->aigAssignment won't have all the inputs assigned. in this
       * case, we log what happened and hope that giving an arbitrary assignment
       * (false) to the rest of the inputs is the right thing. */
#ifdef COMPSOLVE_PARTIAL_ASSIGNMENT_COMPLETION
      for (unsigned i = 0; i < b->aig->num_inputs; i++) {
        aiger_symbol *input = b->aig->inputs + i;
        if (unknown == b->aigAssignment[aiger_lit2var(input->lit)]) {
          b->aigAssignment[aiger_lit2var(input->lit)] = false;
          bf_log(b, "bf", 8, "arbitrary assign: %i\n", bf_aiger2lit(input->lit));
        }
      }
#endif
  
      goto state_exit;
    } else {
      b->aigAssignment[aiger_lit2var(bf_lit2aiger(var))] = truth;
    }
    goto state_variables;
  } else {
    bf_log(b, "bf", 1, "expected variable, but didn't get one\n");
    goto state_error;
  }
state_eof:
  if (ferror(solutionFile)) {
    bf_log(b, "bf", 1, "IO error reading solution file: %s", strerror(errno));
    goto state_error;
  } else {
    goto state_exit;
  }
state_error:
  result = BF_UNKNOWN;
  goto state_exit;
state_exit:
  return result;
}


static void bfPrintCNFToFILE(bfman *b, FILE *solver_input)
{
  fprintf(solver_input, "p cnf ");
  fpos_t vpos;
  int r = fgetpos(solver_input, &vpos);
  assert(r==0);
  // Make enough space for variables and clauses
  fprintf(solver_input, "           ");
  fprintf(solver_input, "                  \n");

  /* make sure each clause gets generated for the file. unfortunately this means
   * other backends that depend on this flag will break =[ */
  for (unsigned i = 0; i < b->aig->num_ands; i++) {
    b->aig->ands[i].clausesForThisNodeExists = 0;
  }
  for (unsigned i = 0; i < b->aig->num_outputs; i++) {
    b->aig->outputs[i].clausesForThisNodeExists = 0;
  }
#ifdef INCREMENTAL_PICOSAT_ENABLED
  /* need to reset incremental picosat because we destroyed the
   * clausesForThisNodeExists mark. */
  if (b->picosatInitd) {
    picosat_reset();
    b->picosatInitd = false;
  }
#endif
  /** \todo any other incremental backends */

  funcsatReset(b->funcsat);
  if (b->funcsat->numVars != b->numVars) {
    funcsatReset(b->funcsat);
    funcsatResize(b->funcsat, b->numVars);
  }


  unsigned cntClauses = 0;
  for(unsigned i = 0; i < b->assumps->size; i++) {
    literal p = (b->assumps->data[i]);
    //Add relevant clauses to the SAT solver
    if(p!=0) {
      cntClauses += addLitConeOfInfluence(b, bf_lit2aiger(p), solver_input);
      fprintf(solver_input, "%i 0\n", p);
      cntClauses++;
    }
  }

  for (unsigned i = 0 ; i < b->aig->num_outputs; i++) {
    aiger_symbol *out = b->aig->outputs + i;
    literal o = bf_aiger2lit(out->lit);
    if(!bf_litIsTrue(o)) {
      cntClauses += addLitConeOfInfluence(b, bf_lit2aiger(o), solver_input);
      if (!out->clausesForThisNodeExists) {
        fprintf(solver_input, "%i 0\n", -o);
        cntClauses++;
        out->clausesForThisNodeExists = 1;
      }
    }
  }


  // Go back and smash the number of variables and clauses into the file. :(
  r = fsetpos(solver_input, &vpos);
  fprintf(solver_input, "%u ", b->numVars);
  fprintf(solver_input, "%u", cntClauses);
}

void bfPrintCNF(bfman *b, char *inputName)
{
  FILE *solver_input = fopen(inputName, "w");
  bfPrintCNFToFILE(b, solver_input);
  fclose(solver_input);
}


static bfresult competitionSolve(bfman *b, char *solverName, char *solverFlags)
{
  bf_log(b, "bf", 1, "competitionSolve %s %s\n", solverName, solverFlags);

  char inputName[150], outputName[150];
  strcpy(inputName, "/tmp/cnf.XXXXXX");
  sprintf(outputName, "/tmp/%s-out.XXXXXX", solverName);
  int fd_in = mkstemp(inputName);
  int fd_out = mkstemp(outputName);
  bf_log(b, "bf", 2, "input:  %s\n", inputName);
  bf_log(b, "bf", 2, "%s output: %s\n", solverName, outputName);

  FILE *solver_input = fdopen(fd_in, "w");
  bfPrintCNFToFILE(b, solver_input);
  fclose(solver_input);


  //  bfPopAssumptions(b, b->assumps->size);
  /* bfResetFuncsat(b); */

  char cmd[1024];
  sprintf(cmd, "%s %s %s > %s 2>&1", solverName, solverFlags, inputName, outputName);
  bf_log(b, "bf", 2, "system_cmd: %s\n", cmd);
  int ignore = system(cmd);

  /* keep input around sometimes when the user can see the filename */
  bf_IfDebug (b, "bf", 2) {}
  else {
    unlink(inputName);
  }

  FILE *solver_output = fdopen(fd_out, "r");

  bfresult result = bf_parse_dimacs_solution(b, solver_output);
  if (result == BF_UNKNOWN) {
    fprintf(b->dout, "error invoking solver: '%s'\n", cmd);
    fprintf(stderr,  "error invoking solver: '%s'\n", cmd);
  }

exit:
  fclose(solver_output);
  /* keep output around when the user can see the filename */
  bf_IfDebug (b, "bf", 2) {}
  else {
    unlink(outputName);
  }
  return result;
}

static bfresult abcReduceSolve(bfman *b, char *solverName, char *solverFlags)
{
  bf_log(b, "bf", 1, "in abcReduceSolve\n");
  char inputName[150], outputName[150];
  strcpy(inputName, "/tmp/cnf.XXXXXX");
  sprintf(outputName, "/tmp/%s-out.XXXXXX", solverName);
  int fd_in = mkstemp(inputName);
  int fd_out = mkstemp(outputName);
  bf_log(b, "bf", 2, "input:  %s\n", inputName);
  bf_log(b, "bf", 2, "%s output: %s\n", solverName, outputName);

  
  printAiger_allinputs(b);      //write the problem aig to /tmp/new_aig.aig
  char cmd[1024];
  //sprintf(cmd, "abc -c \"read /tmp/new_aig.aig; refactor; balance; write_cnf %s\"", inputName);
  //sprintf(cmd, "abc -c \"read /tmp/new_aig.aig; balance; resub; resub -K 6; balance; resub -z; resub -z -K 6; balance; resub -K 5; balance; csweep; write_cnf %s\"", inputName);
  sprintf(cmd, "abc -c \"read /tmp/new_aig.aig; balance -l; rewrite -l; refactor -l; balance -l; rewrite -l; rewrite -z -l; balance -l; refactor -z -l; rewrite -z -l; balance -l; write_cnf %s\"", inputName);
  int ignore = system(cmd);

  //  bfPopAssumptions(b, b->assumps->size);
  bf_IfDebug (b, "bf", 2) {
    bf_log(b, "bf", 2, "stats:\n");
    bfPrintStats(b, b->dout);
  }

  sprintf(cmd, "%s %s %s > %s 2>&1", solverName, solverFlags, inputName, outputName);
  ignore = system(cmd);

  FILE *solver_input = fdopen(fd_in, "r");

  int num_vars_after_abc = 0;
  uint8_t found_num_vars = 0;

  do {
    if(fscanf(solver_input, "p cnf %u", ((unsigned *)&num_vars_after_abc)) > 0) { found_num_vars = 1; break; }
  } while(fgetc(solver_input)!=EOF);

  if(found_num_vars == 0) assert(false && "found 0 vars");

  fclose(solver_input);
  unlink(inputName);

  //inputs are now the highest numbered variables
  int first_var = (num_vars_after_abc - (int)b->aig->num_inputs);

  //  fprintf(stderr, "%i %u %i\n", num_vars_after_abc, b->aig->num_inputs, first_var);

  FILE *solver_output = fdopen(fd_out, "r");

  literal var = 0;
  bool isSAT = false;

  do {
    if(fscanf(solver_output, "\nv %d", ((int *)&var)) > 0) { isSAT = true; break; }
  } while(fgetc(solver_output)!=EOF);

  //get solution
  if(isSAT) {
    assert(b->aigAssignment_size >= (b->aig->maxvar+1));
    bf_clear_assignment(b);
  read_next_literal:
    do {
      if(var == 0) {
        goto exit;
      }
      assert((var < 0 || var > 0) && "found variable 0?");
      bool v = var < 0 ? false : true;
      var = v?var:-var;
      if(var >= first_var && var < num_vars_after_abc) {
	var = (var - first_var);
	//fprintf(stderr, "%i = %d (%i)\n", var, v, aiger_lit2var(b->aig->inputs[var].lit));
	b->aigAssignment[aiger_lit2var(b->aig->inputs[var].lit)] = (mbool)v;
      }
      /* if(bfPushAssumption(m, var) == BF_UNSAT) assert(0 && "bad assumption"); */
    } while(fscanf(solver_output, "%d", ((int *)&var)) > 0);
    while(fgetc(solver_output)!=EOF) {
      if(fscanf(solver_output, "%d", ((int *)&var)) > 0) goto read_next_literal;
    }
  }


exit:
  fclose(solver_output);
  unlink(outputName);
  return isSAT ? BF_SAT : BF_UNSAT;
}


bfresult bfPicosatSolve(bfman *b)
{
  return competitionSolve(b, "picosat", "-v");
}

bfresult bfPicosatReduceSolve(bfman *b)
{
  return abcReduceSolve(b, "picosat", "-v");
}

bfresult bfLingelingReduceSolve(bfman *b)
{
  return abcReduceSolve(b, "lingeling", "-v");
}

bfresult bfLingelingSolve(bfman *b)
{
  return competitionSolve(b, "lingeling", "-v");
}

bfresult bfPlingelingSolve(bfman *b)
{
  return competitionSolve(b, "plingeling", "-t 4 -v");
}

bfresult bfPrecosatSolve(bfman *b)
{
  return competitionSolve(b, "precosat", "-v");
}

bfresult bfExternalSolve(bfman *b)
{
  return competitionSolve(b, "bfsolve", "");
}

// CURRENTLY BROKEN
bfresult bfABCSolve(bfman *m, int64_t UNUSED(lim))
{
  (void)m;
  fprintf(stderr, "bfABCSolve is broken. Please report this bug.");
  abort();
}

#ifdef INCREMENTAL_PICOSAT_ENABLED
void picosat_addClauseIncrementally(bfman *UNUSED(b), clause *c, void *UNUSED(ignore))
{
  literal *p;
  forClause (p, c) {
    picosat_add(*p);
  }
  picosat_add(0);
}
#endif

#ifndef INCREMENTAL_PICOSAT_ENABLED
bfresult bfPicosatIncrementalSolve(bfman *UNUSED(b))
{
  assert(false && "please reconfigure with --with-picosat");
  abort();
}
#else
bfresult bfPicosatIncrementalSolve(bfman *b)
{
  if (!b->picosatInitd) {
    picosat_init();
    b->picosatInitd = true;
    /* if we have to initialize picosat, it won't have any clauses. this loop
     * makes sure they lal get added. */
    for (unsigned i = 0; i < b->aig->num_ands; i++) {
      b->aig->ands[i].clausesForThisNodeExists = 0;
    }
    for (unsigned i = 0; i < b->aig->num_outputs; i++) {
      b->aig->outputs[i].clausesForThisNodeExists = 0;
    }
  }

  assert(b->numVars < INT_MAX);
  picosat_adjust(b->numVars);

  b->addClauseToSolver = picosat_addClauseIncrementally;
  for (unsigned i = 0; i < b->assumps->size; i++) {
    literal p = (b->assumps->data[i]);
    if(p!=0) addLitConeOfInfluence(b, bf_lit2aiger(p), NULL);
  }
  for (unsigned i = 0 ; i < b->aig->num_outputs; i++) {
    aiger_symbol *out = b->aig->outputs + i;
    literal o = bf_aiger2lit(out->lit);
    if(!bf_litIsTrue(o)) {
      addLitConeOfInfluence(b, bf_lit2aiger(o), NULL);
      if (!out->clausesForThisNodeExists) {
        picosat_add(-o);
        picosat_add(0);
        out->clausesForThisNodeExists = 1;
      }
    }
  }

  literal *assump;
  forBv (assump, b->assumps) {
    //if (*assump != 0) picosat_assume(bf_aiger2lit(bf_lit2aiger(*assump)));
    if (*assump != 0) picosat_assume(*assump);
  }

  clock_t start = clock();
  int result = picosat_sat(-1);
  b->timePicosatIncrementalSolve += (float)(clock()-start)/CLOCKS_PER_SEC;
  b->numPicosatIncrementalSolves++;
  if (result == PICOSAT_SATISFIABLE) {
    bf_prep_assignment(b);
    for (unsigned i = 0; i < b->aig->num_inputs; i++) {
      aiger_literal alit = b->aig->inputs[i].lit;
      literal lit = bf_aiger2lit(alit);
      b->aigAssignment[aiger_lit2var(alit)] = false;
      if ((int)lit <= picosat_variables()) {
        int pico_val = picosat_deref((int)lit);
        assert(pico_val != 0);  /* 0 is unknown */
        bool v;
        if (pico_val == 1) v = true; else v = false;
        b->aigAssignment[aiger_lit2var(alit)] = (mbool)v;
      }
    }
    return BF_SAT;
  }
  assert(result != PICOSAT_UNKNOWN);

  /* picosat_reset(); */

  return BF_UNSAT;
}
#endif

void bfConfigureFuncsat(bfman *b) { b->solve = bfFuncsatSolve; }
void bfConfigurePicosat(bfman *b) { b->solve = bfPicosatSolve; }
void bfConfigurePicosatIncremental(bfman *b) { b->solve = bfPicosatIncrementalSolve; }
void bfConfigureLingeling(bfman *b) { b->solve = bfLingelingSolve; }
void bfConfigurePlingeling(bfman *b) { b->solve = bfPlingelingSolve; }
void bfConfigurePrecosat(bfman *b) { b->solve = bfPrecosatSolve; }
void bfConfigurePicosatReduce(bfman *b) { b->solve = bfPicosatReduceSolve; }
void bfConfigureLingelingReduce(bfman *b) { b->solve = bfLingelingReduceSolve; }
void bfConfigureExternal(bfman *b) { b->solve = bfExternalSolve; }

bfresult bfSolve(bfman *b)
{
  PROBE(probe_meter_mark(&solve_meter, 1));
  const char *error = aiger_check(b->aig);
  if (error) {
    printf("AIGer error: %s\n", error);
    return BF_UNKNOWN;
  }
  bf_prep_assignment(b);
  bf_clear_assignment(b);
  bf_log(b, "bf", 1, "solving under %u assumptions\n", b->assumps->size);
  bf_IfDebug (b, "bf", 2) {
    bf_log(b, "bf", 1, "assumptions: ");
    literal *assump = NULL, last_assump = LITERAL_FALSE;
    unsigned cnt = 0;
    forBv (assump, b->assumps) {
      if (*assump == last_assump) cnt++;
      else {
        if (cnt > 1) fprintf(b->dout, "(%u times) ", cnt);
        cnt = 1;
        last_assump = *assump;
        fprintf(b->dout, "%i ", *assump);
      }
    }
    if (cnt > 1) fprintf(b->dout, "(%u times)", cnt);
    fprintf(b->dout, "\n");
  }
  return b->solve(b);
}

bfresult bfFuncsatSolve(bfman *b)
{
  unsigned cnt = 0;
  b->addClauseToSolver = addClauseFs;

  for(unsigned i = 0; i < b->assumps->size; i++) {
    literal p = (b->assumps->data[i]);
    //Add relevant clauses to funcsat
    if(p!=0) {
      addLitConeOfInfluence(b, bf_lit2aiger(p), NULL);
      cnt++;
    }
  }

  for (unsigned i = 0 ; i < b->aig->num_outputs; i++) {
    aiger_symbol *out = b->aig->outputs + i;
    literal o = bf_aiger2lit(out->lit);
    if(!bf_litIsTrue(o)) {
      if (!out->clausesForThisNodeExists) {
        clause *c1 = clauseAlloc(1);
        addLitConeOfInfluence(b, bf_lit2aiger(o), NULL);
        clauseInsertX(c1, -o);
        funcsatAddClause(b->funcsat, c1);
        out->clausesForThisNodeExists = 1;
      }
    }
  }


  /** \todo method for deleting all variables not touched by a clause */

  bf_log(b, "bf", 1, "calling funcsat p cnf %u %u (%" PRIu32 "/%u assumptions)\n",
         funcsatNumVars(b->funcsat),
         funcsatNumClauses(b->funcsat),
         b->funcsat->assumptions.size,
         cnt);
  funcsat_result r = funcsatSolve(b->funcsat);
  switch (r) {
  case FS_SAT:
    bf_prep_assignment(b);
    bf_clear_assignment(b);
    for (aiger_size i = 0; i < b->aig->num_inputs; i++) {
      literal fslit = bf_aiger2lit(b->aig->inputs[i].lit);
      assert(!aiger_sign(b->aig->inputs[i].lit));
      mbool v = false;
      if (b->funcsat->numVars >= fs_lit2var(fslit)) {
        v = funcsatValue(b->funcsat, fslit);
      }
      assert(v == true || v == false);
      b->aigAssignment[aiger_lit2var(b->aig->inputs[i].lit)] = v;
    }
    return BF_SAT;
  case FS_UNSAT:
    return BF_UNSAT;
  case FS_UNKNOWN:
    return BF_UNKNOWN;
  default:
    assert(0 && "unknown enum");
    exit(EXIT_FAILURE);
  }
}


mbool bfMustBeTrue_picosat(bfman *man, literal x)
{
  if (bf_litIsTrue(x)) return true;
  if (bf_litIsFalse(x)) return false;

  if(man->assumps->size != 0) return unknown;

  bfresult r = bfPushAssumption(man, -x);
  if(r != BF_UNSAT) {
    //r = bfSolve(man, -1);
    //r = bfPicosatReduceSolve(man);
    r = bfPicosatSolve(man);
    bfPopAssumptions(man, 1);
  }
  if (r == BF_UNKNOWN) return unknown;
  else if (r == BF_UNSAT) return true;
  else return false;
}

mbool bfMustBeTrue_picosatIncremental(bfman *man, literal x)
{
  if (bf_litIsTrue(x)) return true;
  if (bf_litIsFalse(x)) return false;

  if(man->assumps->size != 0) return unknown;

  bfresult r = bfPushAssumption(man, -x);
  if(r != BF_UNSAT) {
    //r = bfSolve(man, -1);
    //r = bfPicosatReduceSolve(man);
    r = bfPicosatIncrementalSolve(man);
    /* r = bfPicosatSolve(man); */
    bfPopAssumptions(man, 1);
  }
  if (r == BF_UNKNOWN) return unknown;
  else if (r == BF_UNSAT) return true;
  else return false;
}

struct worklist {
  aiger_literal l;
  struct worklist *next;
};

unsigned addLitConeOfInfluence(bfman *m, aiger_literal l, FILE *out)
{
  aiger_literal fanin0, fanin1;
  aiger_and *and;
  literal t;
  clause *c1, *c2, *c3;
  unsigned cnt = 0;

  struct worklist *wl, *tmp_wl;
  wl = calloc(1, sizeof(struct worklist));
  wl->l = l;

  while (wl != NULL) {
    aiger_literal l_pos = aiger_strip(wl->l);

    //Reached something that is not an and gate
    if(!(and = aiger_is_and(m->aig, l_pos))) {
      aiger_symbol *var;
      if(!(var = aiger_is_input(m->aig, l_pos))) {
        if(!(var = aiger_is_latch(m->aig, l_pos))) {
          //l_pos is an atomic element (l is either True or False); Or, does not
          //exist in the AIG.
          goto next;
        } else {
          //l_pos is a latch
          //This code only handles and gates, inputs, and outputs. assert if 'l' is a latch.
          assert(0 && "The AIG contains latches. Latches are currently unsupported.\n");
        }
      } else {
        //l_pos is an input
        //fprintf(stderr, "adding an input %u\n", l);
        goto next;
      }
    }
    //l_pos is an and

    //  fprintf(stderr, "%u, %u\n", and->lhs, l_pos);
    assert(and->lhs == l_pos);
    
    if(and->clausesForThisNodeExists) goto next;
    and->clausesForThisNodeExists = 1;
    
    /*
      cnt += addLitConeOfInfluence(m, and->rhs0, out);
      cnt += addLitConeOfInfluence(m, and->rhs1, out);
    */

    //  funcsatBumpLitPriority(m->funcsat, bf_aiger2lit(and->lhs));
  
    c1 = clauseAlloc(3);
  
    clausePush(c1, t = -bf_aiger2lit(and->rhs0));
    assert(!bf_litIsConst(t));
    clausePush(c1, t = -bf_aiger2lit(and->rhs1));
    assert(!bf_litIsConst(t));
    clausePush(c1, t = bf_aiger2lit(and->lhs));
    assert(!bf_litIsConst(t));
    bf_IfDebug (m, "bf", 3) {
      bf_log(m, "bf", 3, "addLitCOI clause: ");
      dimacsPrintClause(m->dout, c1);
      fprintf(m->dout, "\n");
    }
    if (out) dimacsPrintClause(out, c1), fprintf(out, "\n");
    else m->addClauseToSolver(m, c1, NULL);
    clauseFree(c1);
    cnt++;
  
    c2 = clauseAlloc(2);
    clausePush(c2, t = bf_aiger2lit(and->rhs0));
    assert(!bf_litIsConst(t));
    clausePush(c2, t = -bf_aiger2lit(and->lhs));
    assert(!bf_litIsConst(t));
    bf_IfDebug (m, "bf", 3) {
      bf_log(m, "bf", 3, "addLitCOI clause: ");
      dimacsPrintClause(m->dout, c2);
      fprintf(m->dout, "\n");
    }
    if (out) dimacsPrintClause(out, c2), fprintf(out, "\n");
    else m->addClauseToSolver(m, c2, NULL);
    clauseFree(c2);
    cnt++;

    c3 = clauseAlloc(3);
    clausePush(c3, t = bf_aiger2lit(and->rhs1));
    assert(!bf_litIsConst(t));
    clausePush(c3, t = -bf_aiger2lit(and->lhs));
    assert(!bf_litIsConst(t));
    bf_IfDebug (m, "bf", 3) {
      bf_log(m, "bf", 3, "addLitCOI clause: ");
      dimacsPrintClause(m->dout, c3);
      fprintf(stderr, "\n");
    }
    if (out) dimacsPrintClause(out, c3), fprintf(out, "\n");
    else m->addClauseToSolver(m, c3, NULL);
    clauseFree(c3);
    cnt++;

    wl->l = and->rhs0;
    tmp_wl = calloc(1, sizeof(struct worklist));
    tmp_wl->l = and->rhs1;
    tmp_wl->next = wl->next;
    wl->next = tmp_wl;
    continue;

  next:
    tmp_wl = wl;
    wl = wl->next;
    free(tmp_wl);
  }

  return cnt;
}

void addToNewAig(bfman *m, aiger *new_aig, aiger_literal l)
{
  aiger_literal fanin0, fanin1;
  aiger_and *and;
  aiger_literal l_pos = aiger_strip(l);
  literal t;
  clause *c1, *c2, *c3;

  //Reached something that is not an and gate
  if(!(and = aiger_is_and(m->aig, l_pos))) {
    aiger_symbol *var;
    if(!(var = aiger_is_input(m->aig, l_pos))) {
      if(!(var = aiger_is_latch(m->aig, l_pos))) {
	//l_pos is an atomic element (l is either True or False); Or, does not
	//exist in the AIG.
	return;
      } else {
	//l_pos is a latch
	//This code only handles and gates, inputs, and outputs. assert if 'l' is a latch.
	assert(0 && "The AIG contains latches. Latches are currently unsupported.\n");
      }
    } else {
      //l_pos is an input
      //fprintf(stderr, "adding an input %u\n", l);
      if(var->nodeCopied == 1) return;
      var->nodeCopied = 1;
      // aiger_add_input(new_aig, l_pos, NULL);
      return;
    }
  }
  //l_pos is an and

  //  fprintf(stderr, "%u, %u\n", and->lhs, l_pos);
  assert(and->lhs == l_pos);
  
  if(and->nodeCopied) return;
  and->nodeCopied = 1;
  
  addToNewAig(m, new_aig, and->rhs0);
  addToNewAig(m, new_aig, and->rhs1);

  aiger_add_and(new_aig, and->lhs, and->rhs0, and->rhs1);
}

void printAiger(bfman *m, char *filename, bitvec *output) {
  //Add assupmtions and all nodes to a new AIG.
  aiger *new_aig = aiger_init();
  
  for(unsigned i = 0; i < output->size; i++) {
    literal p = (output->data[i]);
    if(p != 0) {
      aiger_add_output(new_aig, bf_lit2aiger(p), NULL);
      addToNewAig(m, new_aig, bf_lit2aiger(p));
    }
  }

  for (aiger_size i = 0; i < m->aig->num_inputs; ++i)
    if (m->aig->inputs[i].nodeCopied == 1){
      aiger_add_input(new_aig, m->aig->inputs[i].lit, NULL);
    }
  
  
  aiger_open_and_write_to_file(new_aig, filename);

  aiger_reset(new_aig);

  for (aiger_size i = 0; i < m->aig->num_ands; ++i)
    m->aig->ands[i].nodeCopied = 0;

  for (aiger_size i = 0; i < m->aig->num_inputs; ++i)
    m->aig->inputs[i].nodeCopied = 0;
}

void printAiger_allinputs(bfman *m)
{
  //Add assumptions and all nodes to a new AIG.
  aiger *new_aig = aiger_init();
  
  literal assumps_and = LITERAL_TRUE;

  for(unsigned i = 0; i < m->assumps->size; i++) {
    literal p = (m->assumps->data[i]);
    if(p != 0) {
      assumps_and = bfNewAnd(m, assumps_and, p);
    }
  }

  for (aiger_size i = 0; i < m->aig->num_inputs; i++) {
    aiger_add_input(new_aig, m->aig->inputs[i].lit, m->aig->inputs[i].name);
    m->aig->inputs[i].nodeCopied = 1;
  }

  addToNewAig(m, new_aig, bf_lit2aiger(assumps_and));

  aiger_add_output(new_aig, bf_lit2aiger(assumps_and), NULL);


  aiger_open_and_write_to_file(new_aig, "/tmp/new_aig.aig");

  aiger_reset(new_aig);

  for (aiger_size i = 0; i < m->aig->num_ands; ++i)
    m->aig->ands[i].nodeCopied = 0;

  for (aiger_size i = 0; i < m->aig->num_inputs; ++i)
    m->aig->inputs[i].nodeCopied = 0;
}

void printFalseAig(bfman *m)
{
  aiger *new_aig = aiger_init();

  for (aiger_size i = 0; i < m->aig->num_inputs; ++i) {
    aiger_add_input(new_aig, m->aig->inputs[i].lit, m->aig->inputs[i].name);
  }

  aiger_add_output(new_aig, aiger_false, NULL);

  aiger_open_and_write_to_file(new_aig, "/tmp/False.aig");

  aiger_reset(new_aig);
}


static void checkConeOfInfluenceFromSolver(bfman *b, aiger_literal l)
{
  aiger_literal fanin0, fanin1;
  aiger_and *and;
  aiger_literal l_pos = aiger_strip(l);

  //Reached something that is not an and gate
  if(!(and = aiger_is_and(b->aig, l_pos))) {
    aiger_symbol *var;
    if(!(var = aiger_is_input(b->aig, l_pos))) {
      if(!(var = aiger_is_latch(b->aig, l_pos))) {
        //l_pos is an atomic element (l is either True or False); Or, does not
        //exist in the AIG.
        return;
      } else {
        //l_pos is a latch
        //This code only handles and gates, inputs, and outputs. assert if 'l' is a latch.
        assert(0 && "The AIG contains latches. Latches are currently unsupported.\n");
      }
    } else {
      //l_pos is an input
      //fprintf(stderr, "adding an input %u\n", l);
      mbool fromAig = b->aigAssignment[aiger_lit2var(l_pos)];
      bool fromFs  = (bool)funcsatValue(b->funcsat, bf_aiger2lit(l_pos));
      assert(fromAig == fromFs);
      return;
    }
  }
  //l_pos is an and

  //  fprintf(stderr, "%u, %u\n", and->lhs, l_pos);
  assert(and->lhs == l_pos);


  checkConeOfInfluenceFromSolver(b, and->rhs0);
  checkConeOfInfluenceFromSolver(b, and->rhs1);

  mbool fromAig = b->aigAssignment[aiger_lit2var(l_pos)];
  bool fromFs  = (bool)funcsatValue(b->funcsat, bf_aiger2lit(l_pos));
  assert(fromAig == fromFs);
  
}


void bf_clear_assignment(bfman *b)
{
  memset(b->aigAssignment, (int)unknown, b->aigAssignment_size * sizeof(*b->aigAssignment));
}


bool bf_get_assignment_lazily(bfman *b, variable p)
{
  aiger_variable p_pos = aiger_lit2var(bf_lit2aiger(p));
  if (b->aigAssignment[p_pos] == unknown) {
    intvec *queue = intvecAlloc(2);
    aiger_and *and;

    intvecPush(queue, bf_lit2aiger(p));

    while (queue->size > 0) {
      /* for (unsigned i = 0; i < queue->size; i++) { */
      /*   fprintf(stderr, "%i ", bf_aiger2lit(queue->data[i])); */
      /* } */
      /* fprintf(stderr, "\n"); */
      aiger_literal l     = intvecPeek(queue);
      aiger_literal l_pos = aiger_strip(l);

      //Reached something that is not an and gate
      if (!(and = aiger_is_and(b->aig, l_pos))) {
        aiger_symbol *var;
        if (!(var = aiger_is_input(b->aig, l_pos))) {
          if (!(var = aiger_is_latch(b->aig, l_pos))) {
            //l_pos is an atomic element (l is either True or False); Or, does not
            //exist in the AIG.
            intvecPop(queue);
          } else {
            //l_pos is a latch
            //This code only handles and gates, inputs, and outputs. assert if 'l' is a latch.
            assert(0 && "The AIG contains latches. Latches are currently unsupported.\n");
          }
        } else {
          //l_pos is an input
          //fprintf(stderr, "adding an input %u\n", l);
          assert(b->aigAssignment[aiger_lit2var(l)] != unknown);
          intvecPop(queue);
        }
      } else {
        //l_pos is an and
        assert(and->lhs == l_pos);
        if (b->aigAssignment[aiger_lit2var(l)] == unknown) {
          aiger_literal rhs0 = and->rhs0, rhs1 = and->rhs1;
          mbool rhs0val = b->aigAssignment[aiger_lit2var(rhs0)];
          mbool rhs1val = b->aigAssignment[aiger_lit2var(rhs1)];

          if (rhs0val == unknown) intvecPush(queue, rhs0);
          if (rhs1val == unknown) intvecPush(queue, rhs1);

          if (rhs0val != unknown && rhs1val != unknown) {
            assert(rhs0val != unknown);
            rhs0val = aiger_sign(rhs0) ? !rhs0val : rhs0val;
            assert(rhs1val != unknown);
            rhs1val = aiger_sign(rhs1) ? !rhs1val : rhs1val;
            b->aigAssignment[aiger_lit2var(l)] = rhs0val && rhs1val;
            intvecPop(queue);
          }
        } else intvecPop(queue);
      }
    }

    intvecDestroy(queue), free(queue);
  }

  assert(b->aigAssignment[p_pos] != unknown);
  return (bool)b->aigAssignment[p_pos];
}

void bf_prep_assignment(bfman *b)
{
  unsigned sz = b->aigAssignment_size;
  if (b->aigAssignment_size < b->aig->maxvar+1) {
    while (b->aigAssignment_size < b->aig->maxvar+1) {
      b->aigAssignment_size *= 2;
    }
    ReallocX(b->aigAssignment, b->aigAssignment_size, sizeof(*b->aigAssignment));
    assert(b->aig->maxvar+1 <= b->aigAssignment_size);

    memset(b->aigAssignment + sz, (int)unknown,
           sizeof(*b->aigAssignment) * (b->aigAssignment_size - sz));
  }
}
