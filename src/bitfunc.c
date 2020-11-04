/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <string.h>
#include <bitfunc.h>
#include <funcsat/posvec.h>
#include <funcsat.h>
#include <funcsat_internal.h>
/* #include <funcsat/frontend.h> */
#include <bitfunc/circuits.h>
#include <bitfunc/circuits_internal.h>
#include <bitfunc/bitfunc_internal.h>
#include <bitfunc/debug.h>
#include <unistd.h>

#ifdef INCREMENTAL_PICOSAT_ENABLED
#include <picosat.h>
#endif

#include "bfprobes.h"

/* from funcsat */
extern bool clauseInsert(clause *c, literal p);

#define min(x,y) ((x) <= (y) ? (x) : (y))
#define max(x,y) ((x) <= (y) ? (y) : (x))

static unsigned int hash_and(void *k) {
  struct and_key *and = (struct and_key*)k;
  unsigned hashval = 0;
  hashval += (unsigned)min(and->a, and->b);
  hashval *= 17;
  hashval += (unsigned)max(and->a, and->b);
  return hashval;
}

static int equal_and(void *k1, void *k2) {
  struct and_key *and1 = (struct and_key*)k1, *and2 = (struct and_key*)k2;
  return ((and1->a == and2->a) && (and1->b == and2->b))
    || ((and1->b == and2->a) && (and1->a == and2->b));
}

static void addAndAiger(bfman *m, literal lhs, literal rhs0, literal rhs1)
{
  assert(!bf_litIsConst(lhs));
  assert(!bf_litIsConst(rhs0));
  assert(!bf_litIsConst(rhs1));
  assert(lhs > 0);
  if (lhs < 0) {
    literal t = bfNewAnd(m, rhs0, rhs1);
    aiger_add_and(m->aig, bf_lit2aiger(lhs), bf_lit2aiger(-t), bf_lit2aiger(-t));
    return;
  }
  bf_log(m, "varmap", 2, "%jd = %jd & %jd [%jd %jd %jd]\n", lhs, rhs0, rhs1, bf_aiger2lit(bf_lit2aiger(lhs)), bf_aiger2lit(bf_lit2aiger(rhs0)), bf_aiger2lit(bf_lit2aiger(rhs1)));
  aiger_add_and(m->aig, bf_lit2aiger(lhs), bf_lit2aiger(rhs0), bf_lit2aiger(rhs1));
}

static void addOutputAiger(bfman *m, literal o)
{
  aiger_add_output(m->aig, bf_lit2aiger(o), NULL);
}

static unsigned hash_var(void *k)
{
  variable u = *(variable *) k;
  unsigned int hash = 0;
  unsigned int b = 378551, a = 63689;
  while (u > 0) {
    uint8_t byte = u & (uint8_t) 0xff;
    hash = hash * a + byte;
    a = a * b;
    u >>= 8;
  }
  return hash;
}

static int lit_equal(void *k1, void *k2)
{
  literal *l1 = (literal *) k1;
  literal *l2 = (literal *) k2;
  return *l1 == *l2;
}

extern void picosatAddClause(bfman *b, clause *c, void *ignore);

bfman *bfInit(bfmode mode)
{
  assert(LITERAL_TRUE  == -LITERAL_FALSE);
  assert(-LITERAL_TRUE ==  LITERAL_FALSE);


  bfman *m;
  CallocX(m, 1, sizeof(*m));

  mode = AIG_MODE;

  m->mode = mode;

  vectorInit(&m->memories, 8);
  m->numVars = 0;
  m->andhash = create_hashtable(1024, hash_and, equal_and);
  m->mode = mode;
  m->hash_hits = 0;
  m->addAnd = addAndAiger;
  m->addClause = add_clause_to_AIG;
  m->addAIGOutput = addOutputAiger;
  funcsat_config *conf = funcsatConfigInit(NULL);
  conf->useSelfSubsumingResolution = false;
  conf->minimizeLearnedClauses = false;
  conf->usePhaseSaving = false;
  m->funcsat = funcsatInit(conf);
#ifdef BF_DEBUG_FUNCSAT
  funcsatDebug(m->funcsat, "findUips", 5);
  funcsatDebug(m->funcsat, "bcp", 9);
  funcsatDebug(m->funcsat, "solve", 5);
#endif
  m->aig = aiger_init();
  m->aigAssignment_size = 16;
  MallocX(m->aigAssignment, m->aigAssignment_size, sizeof(*m->aigAssignment));
  bf_clear_assignment(m);
  m->addClauseToSolver = addClauseFs;

  m->picosatInitd = false;

  m->assumps = bfvAlloc(10);

  m->autocompress = true;
  m->memLoadWithSat = false;

  m->numPicosatIncrementalSolves = 0;
  m->timePicosatIncrementalSolve = 0.f;

  m->lit_names = create_hashtable(16, hash_var, lit_equal);
  m->logSyms = create_hashtable(16, hashString, stringEqual);
  posvecInit(&m->logStack, 2);
  posvecPush(&m->logStack, 0);
  m->dout = stderr;

  bfConfigureFuncsat(m);

  PROBE(register_probes());

  return m;
}

void bfDestroy(bfman *m)
{
  bool thenfree = true;
  unsigned numVars = m->numVars;
  funcsat_config *conf = m->funcsat->conf;
  funcsatDestroy(m->funcsat);
  if (thenfree) funcsatConfigDestroy(conf);
  else m->funcsat = funcsatInit(conf);
  if (thenfree) hashtable_destroy(m->andhash, true, true);
  else hashtable_clear(m->andhash, true, true);
  aiger_reset(m->aig);
  if (!thenfree) m->aig = aiger_init();
  if (thenfree) {
    memory **mIt;
    forVector (memory **, mIt, &m->memories) {
      bfmUnsafeFree(*mIt);
    }
    vectorDestroy(&m->memories);
  }
  if (thenfree) CONSUME_BITVEC(m->assumps);

  if(thenfree) free(m->aigAssignment);

#ifdef INCREMENTAL_PICOSAT_ENABLED
  if (m->picosatInitd) {
    picosat_reset();
    m->picosatInitd = false;
  }
#endif

  posvecDestroy(&m->logStack);
  hashtable_destroy(m->lit_names, true, true);
  hashtable_destroy(m->logSyms, true, true);

  if (!thenfree) {
    m->hash_hits = 0; 
    /* m->converted_ands = m->converted_outs = 0; */
    bfSetNumVars(m, numVars);
  } else {
    free(m);
  }
}

void bfPrintAIG_dot(bfman *b, const char *path)
{
  FILE *dot;
  if (NULL == (dot = fopen(path, "w"))) perror("fopen"), exit(1);
  fprintf(dot, "digraph G {\n");

  /* inputs */
  fprintf(dot, "// inputs:\n");
  for (unsigned i = 0; i < b->aig->num_inputs; i++) {
    aiger_literal input = b->aig->inputs[i].lit;
    fprintf(dot, "%u [color=\"green\"];\n", fs_lit2var(bf_aiger2lit(input)));
  }
  fprintf(dot, "\n");
  /* outputs */
  fprintf(dot, "// outputs:\n");
  for (unsigned i = 0; i < b->aig->num_outputs; i++) {
    aiger_symbol *o = &b->aig->outputs[i];
    fprintf(dot, "%u [color=\"red\"];\n", fs_lit2var(bf_aiger2lit(o->lit)));
  }
  fprintf(dot, "\n");


  /* gates */
  for (unsigned i = 0; i < b->aig->num_ands; i++) {
    aiger_and *theAnd = &b->aig->ands[i];
    aiger_literal lhs = theAnd->lhs;
    aiger_literal rhs0 = theAnd->rhs0;
    aiger_literal rhs1 = theAnd->rhs1;
    assert(!aiger_sign(lhs));
    fprintf(dot, "%u -> %u [%s];\n",
            fs_lit2var(bf_aiger2lit(rhs0)),
            fs_lit2var(bf_aiger2lit(lhs)),
            (aiger_sign(rhs0) ? "arrowhead=diamond" : ""));
    fprintf(dot, "%u -> %u [%s];\n",
            fs_lit2var(bf_aiger2lit(rhs1)),
            fs_lit2var(bf_aiger2lit(lhs)),
            (aiger_sign(rhs1) ? "arrowhead=diamond" : ""));
  }

  fprintf(dot, "}\n");
  if (0 != fclose(dot)) perror("fclose");
}

void bfPrintAIG(bfman *b, FILE *out)
{
  if (!out) out = stderr;
  fprintf(out, "inputs: ");
  for (unsigned i = 0; i < b->aig->num_inputs; i++) {
    aiger_literal input = b->aig->inputs[i].lit;
    fprintf(out, "%i ", bf_aiger2lit(input));
  }
  fprintf(out, "\n");

  int cnt = 0;
  for (unsigned i = 0; i < b->aig->num_ands; i++) {
    aiger_and *theAnd = &b->aig->ands[i];
    aiger_literal lhs = theAnd->lhs;
    aiger_literal rhs0 = theAnd->rhs0;
    aiger_literal rhs1 = theAnd->rhs1;
    cnt += fprintf(out, "%i < %i %i",
                   bf_aiger2lit(lhs),
                   bf_aiger2lit(rhs0),
                   bf_aiger2lit(rhs1));
    if (cnt > 70) {
      fprintf(out, "\n");
      cnt = 0;
    } else if (i+1 < b->aig->num_ands) {
      fprintf(out, " || ");
    }
  }
  fprintf(out, "\n");
  fprintf(out, "outputs: ");
  for (unsigned i = 0; i < b->aig->num_outputs; i++) {
    aiger_symbol *o = &b->aig->outputs[i];
    fprintf(out, "%i ", bf_aiger2lit(o->lit));
  }
  fprintf(out, "\n");
}

void bfResetFuncsat(bfman *m)
{
  assert(m->assumps->size == 0);
  funcsat_config *conf = m->funcsat->conf;
  funcsatDestroy(m->funcsat);
  m->funcsat = funcsatInit(conf);

  // clear 'clausesForThisNodeExists' for all AIG nodes.
  for (aiger_size i = 0; i < m->aig->num_ands; ++i)
    m->aig->ands[i].clausesForThisNodeExists = 0;
  for (aiger_size i = 0; i < m->aig->num_outputs; ++i)
    m->aig->outputs[i].clausesForThisNodeExists = 0;
}

void bfComputeCnf(bfman *m)
{
  if (m->mode == AIG_MODE) {
    /*
    for (unsigned i = 1; i < m->aig->maxvar; ++i) {
      if (!aiger_is_input(m->aig, aiger_var2lit(i)) && !aiger_is_and(m->aig, aiger_var2lit(i))) {
        aiger_add_input(m->aig, aiger_var2lit(i), NULL);
      }
    }
    */
    const char *error = aiger_check(m->aig);
    if (error) {
      printf("AIGer error: %s\n", error);
      exit(EXIT_FAILURE);
    }
    //aiger_write_to_file(m->aig, aiger_ascii_mode, stderr);
    //printf("\nAIG size: %u maxvar %u inputs %u outputs %u ands %u hash hits\n", m->aig->maxvar, m->aig->num_inputs, m->aig->num_outputs, m->aig->num_ands, m->hash_hits);
    //convertAIG2CNF(m);
  }
}

void bfReset(bfman *b)
{
  bfPopAssumptions(b, b->assumps->size);
}

void bfPopAssumptions(bfman *bf, unsigned num)
{
  INPUTCHECK(bf, num <= bf->assumps->size, "cannot pop more assumptions than there are");

  for(unsigned i = 0; i < num; i++) {
    literal p = bfvPop(bf->assumps);
    assert(p != LITERAL_FALSE);
    
    if(p == 0) continue;

    funcsatPopAssumptions(bf->funcsat, 1);
  }
}

bfresult bfPushAssumption(bfman *m, literal p)
{
  if(p == LITERAL_FALSE) return BF_UNSAT;

  if(p == LITERAL_TRUE) {
    bfvPush(m->assumps, 0);
    return BF_UNKNOWN;
  }

  funcsatReset(m->funcsat);

  if(funcsatPushAssumption(m->funcsat, p) == FS_UNSAT)
    return BF_UNSAT;
  bfvPush(m->assumps, p);

  return BF_UNKNOWN;
}

bfresult bfvPushAssumption(bfman *m, bitvec *p)
{
  for (unsigned i = 0; i < p->size; i++) {
    //Add relevant clauses to the SAT solver
    
    if (bfPushAssumption(m, p->data[i]) == BF_UNSAT) {
      bfPopAssumptions(m,i);
      CONSUME_BITVEC(p);
      return BF_UNSAT;
    }
  }
  CONSUME_BITVEC(p);
  return BF_UNKNOWN;
}

const char *bfResultAsString(bfresult result)
{
  switch (result) {
  case BF_SAT: return "BF_SAT";
  case BF_UNSAT: return "BF_UNSAT";
  case BF_UNKNOWN: return "BF_UNKNOWN";
  }
  abort();
}

unsigned bfNumVars(bfman *m)
{
  return m->numVars;
}

unsigned bfNumInputs(bfman *b)
{
  return b->aig->num_inputs;
}

void bfSetNumVars(bfman *m, unsigned numVars)
{
  funcsatResize(m->funcsat, m->numVars = numVars);
}




literal bflFromCounterExample(bfman *b, literal l)
{
  if (bf_litIsConst(l)) return l;

  mbool assign = bf_get_assignment_lazily(b, fs_lit2var(l));
  assert(assign != unknown);
  if (aiger_sign(bf_lit2aiger(l))) assign = !assign;

  if (assign) return LITERAL_TRUE;
  else return LITERAL_FALSE;
  assert(false && "impossible");
  abort();
}

bitvec *bfvFromCounterExample(bfman *b, bitvec *bv)
{
  bitvec *r = bfvAlloc(bv->size);
  r->size = bv->size;
  for (unsigned i = 0; i < r->size; i++) {
    r->data[i] = bflFromCounterExample(b, bv->data[i]);
  }
  return r;
}

literal bfAssignmentVar2Lit(bfman *b, variable v)
{
  literal conc = bflFromCounterExample(b, (literal)v);
  if (conc == LITERAL_TRUE) return (literal)v;
  else if (conc == LITERAL_FALSE) return -(literal)v;
  assert(false && "assignment not concrete");
  abort();
}

void bfPrintStats(bfman *b, FILE *out)
{
  fprintf(out, "c %u AIG inputs\n", b->aig->num_inputs);
  fprintf(out, "c %u AIG nodes\n", b->aig->num_ands);
  fprintf(out, "c %u AIG outputs\n", b->aig->num_outputs);
  fprintf(out, "c %u hash hits\n", b->hash_hits);
  fprintf(out, "c %f seconds in picosat incremental\n", b->timePicosatIncrementalSolve);
}


void bfFindCore_picomus(bfman *b, const char *core_output_file)
{
  /* code copied from competitionSolve */
  char *solverName = "picomus";

  char inputName[150], outputName[150];
  strcpy(inputName, "/tmp/cnf.XXXXXX");
  sprintf(outputName, "%s", core_output_file);
  int fd_in = mkstemp(inputName);
  FILE *fout = fopen(outputName, "w");
  int fd_out = fileno(fout);
  bf_log(b, "bf", 2, "input:  %s\n", inputName);
  bf_log(b, "bf", 2, "%s output: %s\n", solverName, outputName);

  FILE *solver_input = fdopen(fd_in, "w");
  fprintf(solver_input, "p cnf ");
  fpos_t vpos, cpos;
  int r = fgetpos(solver_input, &vpos);
  assert(r==0);
  fprintf(solver_input, "0000000000 ");
  r = fgetpos(solver_input, &cpos);
  assert(r==0);
  fprintf(solver_input, "000000000000000000\n");
  /* bfPrintCnf(b, solver_input); */

  /* make sure each clause gets generated for the file. unfortunately this means
   * other backends that depend on this flag will break =[ */
  for (unsigned i = 0; i < b->aig->num_ands; i++) {
    b->aig->ands[i].clausesForThisNodeExists = 0;
  }
#ifdef INCREMENTAL_PICOSAT_ENABLED
  /* need to reset incremental picosat because we destroyed the
   * clausesForThisNodeExists mark. */
  if (b->picosatInitd) {
    picosat_reset();
    b->picosatInitd = false;
  }
#endif

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

  r = fsetpos(solver_input, &vpos);
  fprintf(solver_input, "%010u", b->numVars);
  r = fsetpos(solver_input, &cpos);
  fprintf(solver_input, "%018u", cntClauses);

  fclose(solver_input);

  char cmd[1024];
  sprintf(cmd, "%s %s %s > /dev/null 2>&1", solverName, inputName, outputName);
  int ignore = system(cmd);

  /* keep input around sometimes when the user can see the filename */
  bf_IfDebug (b, "bf", 2) {}
  else {
    unlink(inputName);
  }
}

DECLARE_HASHTABLE(litname_insert, litname_search, litname_remove, literal, char)

void bfPrintNamedCNF(bfman *b, const char *cnf_file)
{
  funcsat_config *tmp_conf = funcsatConfigInit(NULL);
  funcsat *tmp_func = funcsatInit(tmp_conf);
  fprintf(stderr, "Parsing core CNF from '%s'...\n", cnf_file);
  parseDimacsCnf(cnf_file, tmp_func);
  bool *printed;
  CallocX(printed, bfNumVars(b)+1, sizeof(literal));
  clause **c;
  forVector (clause **, c, &tmp_func->origClauses) {
    literal *p;
    forClause (p, *c) {
      char *name;
      if (!printed[fs_lit2var(*p)] &&
          (NULL != (name = litname_search(b->lit_names, p)))) {
        fprintf(stderr, "[%u] %s\n", fs_lit2var(*p), name);
        printed[fs_lit2var(*p)] = true;
      }
    }
  }
  free(printed);
}

void bfWriteAigerToFile(bfman *b, const char *path)
{
  FILE *f = fopen(path, "w");
  aiger_write_to_file(b->aig, aiger_binary_mode, f);
  fclose(f);
}


bfresult bfSet(bfman *m, literal a)
{
  if (bf_litIsFalse(a)) {
    return BF_UNSAT;
  }
  if(!bf_litIsTrue(a)) {
    m->addAIGOutput(m, -a);
  }
  return BF_UNKNOWN;
}

void bfSetClause(bfman *b, bitvec *c)
{
  b->addClause(b,c);
}


void bfSimulate(bfman *b, bool *inputs)
{
  bfReset(b);
  bf_prep_assignment(b);
  bf_clear_assignment(b);
  const unsigned ni = bfNumInputs(b);
  for (unsigned i = 0; i < ni; i++) {
    aiger_symbol *input = &b->aig->inputs[i];
    b->aigAssignment[aiger_lit2var(input->lit)] = (mbool)inputs[i];
  }
}

unsigned bfGetInputIndex(bfman *b, variable v)
{
  aiger_literal l = bf_lit2aiger((literal)v);
  aiger_literal l_pos = aiger_strip(l);
  aiger_symbol *s;

  if (!(s = aiger_is_input(b->aig, l_pos))) {
    return UINT_MAX;
  }
  return (unsigned)(s - b->aig->inputs);
}


