/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <bitfunc/config.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <zlib.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>
#include <getopt.h>

#include <bitfunc.h>
#include <bitfunc/debug.h>
#include <bitfunc/circuits.h>
#include <bitfunc/program.h>
#include <bitfunc/program_internal.h>

#define UNUSED(x) (void)(x)


program *bfpInit(bfman *b, specification *spec)
{
  program *p;
  CallocX(p, 1, sizeof(*p));
  CallocX(p->instruction, sizeof(*p->instruction), spec->minlen);
  for (unsigned i = 0; i < spec->minlen; i++) {
    CallocX(p->instruction[i], sizeof(*p->instruction[i]), spec->instructions->size);
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      p->instruction[i][j] = bfNewVar(b);
    }
  }
  p->len = spec->minlen;
  p->isValid = programIsValid(b, spec, p);
  return p;
}

void bfpDestroy(bfman *b, program *p, specification *spec)
{
  if (p->instruction) {
    for (unsigned i = 0; i < spec->minlen; i++) {
      free(p->instruction[i]);
    }
    free(p->instruction);
  }

  bfmsDestroy(b, p->symIn);
  bfmsDestroy(b, p->symOut);

  free(p);
}


program *programForTestVector(bfman *b, specification *spec, program *src, machine_state *init)
{
  program *p;
  CallocX(p, 1, sizeof(*p));

  CallocX(p->instruction, sizeof(*p->instruction), src->len);
  for (unsigned i = 0; i < src->len; i++) {
    CallocX(p->instruction[i], sizeof(*p->instruction[i]), spec->instructions->size);
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      p->instruction[i][j] = src->instruction[i][j];
    }
  }
  p->len = src->len;
  p->symIn = bfmsCopy(b, init);
  p->symOut = runProgram(b, spec, p, p->symIn);
  p->isValid = src->isValid;
  return p;
}

void programResize(bfman *b, specification *spec, program *p, unsigned newlen)
{
  assert(newlen > p->len);
  assert(newlen <= spec->maxlen);

  ReallocX(p->instruction, sizeof(*p->instruction), newlen);
  for (unsigned i = p->len; i < newlen; i++) {
    MallocX(p->instruction[i], sizeof(*p->instruction[i]), spec->instructions->size);
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      p->instruction[i][j] = bfNewVar(b);
    }
  }

  /* step from current symOut to new symOut */
  /* machine_state *finalState = bfmsCopy(b, p->symOut); */
  /* for (unsigned i = p->len; i < newlen; i++) { */
  /*   machine_state *newState = stepProgram(b, spec, p, i, finalState); */
  /*   finalState = newState; */
  /* } */
  /* p->symOut = finalState; */

  /* require that exactly one instruction is used per line of the program. */
  for (unsigned i = p->len; i < newlen; i++) {
    literal insnIsValid = LITERAL_FALSE;
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      p->isValid = bfNewSelect(b,
                               p->isValid,
                               -bfNewAnd(b, insnIsValid, p->instruction[i][j]),
                               LITERAL_FALSE);
      insnIsValid = bfNewSelect(b,
                                -insnIsValid,
                                p->instruction[i][j],
                                LITERAL_TRUE);
    }
    p->isValid = bfNewSelect(b,
                             p->isValid,
                             insnIsValid,
                             LITERAL_FALSE);
  }

  p->len = newlen;
}

void programResizeForNewMachine(bfman *b, specification *spec, program *p, program *source)
{
  assert(source->len > p->len);

  ReallocX(p->instruction, sizeof(*p->instruction), source->len);
  for (unsigned i = p->len; i < source->len; i++) {
    MallocX(p->instruction[i], sizeof(*p->instruction[i]), spec->instructions->size);
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      p->instruction[i][j] = source->instruction[i][j];
    }
  }

  /* step from current symOut to new symOut */
  machine_state *finalState = p->symOut;
  for (unsigned i = p->len; i < source->len; i++) {
    machine_state *inState = finalState;
    finalState = stepProgram(b, spec, p, i, finalState);
    bfmsDestroy(b, inState);
  }
  p->symOut = finalState;
  p->len = source->len;
  p->isValid = source->isValid;
}

machine_state *stepProgram(bfman *b, specification *spec, program *p, unsigned insn, machine_state *m)
{
  machine_state *retMachine = bfmsCopy(b, m);
  instruction **insns = (instruction **)spec->instructions->data;
  for (unsigned gi = 0; gi < spec->instructions->size; gi++) {
    literal match = p->instruction[insn][gi];
    if (match == LITERAL_FALSE) continue;
    machine_state *n = bfmsCopy(b, m);
    insns[gi]->impl(b, n);
    machine_state *o = retMachine;
    retMachine = bfmsSelect(b, match, n, retMachine);
  }
  return retMachine;
}

machine_state *runProgram(bfman *b, specification *spec, program *program, machine_state *initState)
{
  machine_state *finalState = bfmsCopy(b, initState);
  for (unsigned i = 0; i < program->len; i++) {
    machine_state *inState = finalState;
    finalState = stepProgram(b, spec, program, i, finalState);
    bfmsDestroy(b, inState);
  }
  return finalState;
}

program *bfpCopy(bfman *b, specification *spec, program *prog)
{
  program *p;
  CallocX(p, 1, sizeof(*p));
  p->instruction = malloc(sizeof(*p->instruction) * prog->len);
  for (unsigned i = 0; i < prog->len; i++) {
    p->instruction[i] = malloc(sizeof(*p->instruction[i]) * spec->instructions->size);
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      p->instruction[i][j] = prog->instruction[i][j];
    }
  }
  p->len = prog->len;
  if (prog->symIn) p->symIn = bfmsCopy(b, prog->symIn);
  if (prog->symOut) p->symOut = bfmsCopy(b, prog->symOut);
  p->isValid = prog->isValid;
  return p;
}

void bfpPrint(bfman *b, FILE *stream, specification *spec, program *p)
{
  for (unsigned i = 0; i < p->len; i++) {
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      if (bfGet(b, p->instruction[i][j]) == true) {
        instruction *insn = spec->instructions->data[j];
        fprintf(stream, "%s\n", insn->desc);
      }
    }
  }
}

void bfpPrintStats(bfman *b, FILE *stream, specification *spec, program *p)
{
  UNUSED(b), UNUSED(spec);
  fprintf(stream, "Synthesis time: %f; Oracle check time: %f\n",
          p->totalSynthSecs, p->totalCheckSecs);
}

void programPrintDebug(bfman *b, FILE *stream, specification *spec, program *p)
{
  UNUSED(b);
  fprintf(stream, "program of %u insns, %u instructions (isValid %i)\n",
          p->len, spec->instructions->size, p->isValid);

  for (unsigned i = 0; i < p->len; i++) {
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      fprintf(stream, "%i ", p->instruction[i][j]);
    }
    fprintf(stream, "\n");
  }

  for (unsigned g = 0; g < spec->instructions->size; g++) {
    instruction **insns = (instruction **)spec->instructions->data;
    fprintf(stream, "%p ", insns[g]);
  }
  fprintf(stream, "\n");
}

literal programIsValid(bfman *b, specification *spec, program *program)
{
  /* Require that exactly one instruction is used per line of the program. */
  literal programIsValid = LITERAL_TRUE;
  for (unsigned i = 0; i < program->len; i++) {
    literal insnIsValid = LITERAL_FALSE;
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      programIsValid = bfNewSelect(b,
                                   programIsValid,
                                   -bfNewAnd(b, insnIsValid, program->instruction[i][j]),
                                   LITERAL_FALSE);
      insnIsValid = bfNewSelect(b,
                                -insnIsValid,
                                program->instruction[i][j],
                                LITERAL_TRUE);
    }
    programIsValid = bfNewSelect(b,
                                 programIsValid,
                                 insnIsValid,
                                 LITERAL_FALSE);
  }

  return programIsValid;
}


/* ensures the program chosen is sane, i.e., the ith instruction slot is filled
 * with exactly one instruction. */
void checkProgramAssignment(bfman *b, specification *spec, program *program)
{
  for (unsigned i = 0; i < program->len; i++) {
    unsigned c = 0;
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      mbool v = bfGet(b, program->instruction[i][j]);
      if (v == true) c++;
      else assert(v == false);
    }
    assert(c == 1);
  }
}


machine_state *bfmsFromCounterExample(bfman *b, machine_state *symIn)
{
  machine_state *cx = bfmsCopyNoRam(b, symIn);
  for (unsigned i = 0; i < symIn->numregs; i++) {
    bfRegStore_le(b, cx, i, bfvFromCounterExample(b, symIn->reg[i]));
  }
  cx->ram = bfmFromCounterExample(b, symIn->ram);

  return cx;
}

program *bfpFromCounterExample(bfman *b, specification *spec, program *symProg)
{
  program *p = bfpCopy(b, spec, symProg);
  for (unsigned i = 0; i < p->len; i++) {
    for (unsigned j = 0; j < spec->instructions->size; j++) {
      mbool v = bfGet(b, p->instruction[i][j]);
      assert(v != unknown);
      p->instruction[i][j] = bf_bool2lit(v);
      assert(bf_litIsConst(p->instruction[i][j]));
    }
  }
  return p;
}


static float toseconds(clock_t t)
{
  return (float)t/CLOCKS_PER_SEC;
}


program *bfSynthesizeProgram(bfman *b, specification *spec)
{
  assert(spec->minlen <= spec->maxlen);
  assert(spec->testVectors->size > 0);

  /* \todo make sure testVectors is compatible with initial state */

  float totalSynthSecs = 0.f, totalEqSecs = 0.f;
  clock_t startTime, endTime;

  /* Construct instruction selector variables. */
  program *theProgram = bfpInit(b, spec);
  literal testVectorsSat = LITERAL_TRUE;
  literal statesValid = spec->initialState->isValid;

  /* construct a constraint for each test vector. the constraint "runs" the
   * program on the test vector and ensures the final state is a valid state. */
  vector *programs = vectorAlloc(spec->testVectors->size+2);
  machine_state **testVector;
  forVector (machine_state **, testVector, spec->testVectors) {
    if ((*testVector)->isValid == LITERAL_FALSE) goto Failure;

    bf_IfDebug (b, "program", 2) {
      bf_log(b, "program", 2, "input:\n");
      bfmsPrint(b, b->dout, *testVector);
    }

    program *progForTestVector = programForTestVector(b, spec, theProgram, *testVector);
    literal oracleTestVectorSat =
      spec->oracle(b, progForTestVector->symIn, progForTestVector->symOut);
    testVectorsSat = bfNewAnd(b, oracleTestVectorSat, testVectorsSat);
    statesValid = bfNewAnd(b, progForTestVector->symOut->isValid, statesValid);
    vectorPush(programs, progForTestVector);
  }
  assert(programs->size == spec->testVectors->size);

  bf_log(b, "program", 1, "attempting synthesis of program of length %u with %u test vectors\n", spec->minlen, spec->testVectors->size);



  while (spec->minlen <= spec->maxlen) {
    /* if (plen > 25 || spec->testVectors->size > 25) break; */
    machine_state **input;

    /* fprintf(stderr, "Inputs (len = %u):\n", plen); */
    /* forVector (machine_state **, input, spec->testVectors) { */
    /*   bfmsPrint(b, stderr, *input); */
    /* } */

    if (BF_UNSAT == bfPushAssumption(b, theProgram->isValid)) {
      bf_log(b, "program", 1, "program is valid trivially unsat\n");
      assert(b->assumps->size == 0);
      goto IncreaseProgramSize;
    }

    if (BF_UNSAT == bfPushAssumption(b, statesValid)) {
      bf_log(b, "program", 1, "statesValid trivially unsat\n");
      assert(b->assumps->size == 1);
      goto Failure;
    }
    
    if (BF_UNSAT == bfPushAssumption(b, testVectorsSat)) {
      bf_log(b, "program", 1, "testVectors trivially unsat\n");
      assert(b->assumps->size == 2);
      goto IncreaseProgramSize;
    }


    assert(b->assumps->size == 3);
    bf_log(b, "program", 1, "solving... ");
    startTime = clock();
    bfresult result = bfPicosatSolve(b);
    endTime = clock();
    totalSynthSecs += toseconds(endTime-startTime);

    bf_IfDebug (b, "program", 1) {
      fprintf(b->dout, "%s\n", bfResultAsString(result));
    }
    bf_log(b, "program", 2,
           "synth %f (total %f)\n",
           toseconds(endTime-startTime), (float)totalSynthSecs);
    
    if (result == BF_UNSAT) {
      /* see if _any_ program has a valid final state by popping testVectorsSat
       * assumption */
      if (spec->sanityCheckFinalStates) {
        bf_log(b, "program", 1, "any valid final ");
        assert(b->assumps->size == 3);
        bfPopAssumptions(b,1);
        startTime = clock();
        result = bfSolve(b);
        endTime = clock();
        totalSynthSecs += toseconds(endTime-startTime);
        bf_IfDebug (b, "program", 1) {
          fprintf(b->dout, " %s\n", bfResultAsString(result));
        }
        if (result == BF_UNSAT) goto Failure;
      }

    IncreaseProgramSize:
      bfPopAssumptions(b, b->assumps->size);
      assert(b->assumps->size == 0);
      spec->minlen++;
      if (!(spec->minlen <= spec->maxlen)) break;
      bf_log(b, "program", 1, "No such program. Trying length %u.\n", spec->minlen);
      programResize(b, spec, theProgram, spec->minlen);
      testVectorsSat = LITERAL_TRUE;
      statesValid = spec->initialState->isValid;
      program **p;
      forVector (program **, p, programs) {
        programResizeForNewMachine(b, spec, *p, theProgram);
        testVectorsSat =
          bfNewAnd(b, spec->oracle(b, (*p)->symIn, (*p)->symOut), testVectorsSat);
        statesValid = bfNewAnd(b, (*p)->symOut->isValid, statesValid);
      }
      continue;
    }


    bf_log(b, "program", 1, "found program!\n");
    program *candProgram = bfpFromCounterExample(b, spec, theProgram);
    bf_IfDebug (b, "program", 2) {
      bfpPrint(b, stderr, spec, candProgram);
    }
    checkProgramAssignment(b, spec, theProgram);

    bfPopAssumptions(b, b->assumps->size);

    candProgram->symIn = bfmsCopy(b, spec->initialState);
    candProgram->symOut = runProgram(b, spec, candProgram, candProgram->symIn);

    literal oracleSat = spec->oracle(b, candProgram->symIn, candProgram->symOut);
    if (BF_UNSAT == bfPushAssumption(b, -oracleSat)) {
      goto Done;
    }

    bf_log(b, "program", 1, "checking ...");
    startTime = clock();
    result = bfSolve(b);
    endTime = clock();
    totalEqSecs += toseconds(endTime-startTime);
    bf_IfDebug (b, "program", 1) {
      fprintf(b->dout, " %s\n", bfResultAsString(result));
    }
    /* fprintf(stderr, "equiv check = %f (total %f)\n", */
    /*         toseconds(endTime-startTime), (float)totalEqSecs); */
    if (result == BF_UNSAT) goto Done;


    /* counterexample is a new machine input to refine our program */
    machine_state *nextInput = bfmsFromCounterExample(b, candProgram->symIn);
    bfpDestroy(b, candProgram, spec);
    bf_IfDebug (b, "program", 2) {
      bfmsPrint(b, stderr, nextInput);
    }
    program *newProgram = programForTestVector(b, spec, theProgram, nextInput);
    testVectorsSat = bfNewAnd(b,
                              spec->oracle(b, newProgram->symIn, newProgram->symOut),
                              testVectorsSat);
    statesValid = bfNewAnd(b, newProgram->symOut->isValid, statesValid);
    vectorPush(programs, newProgram);
    vectorPush(spec->testVectors, nextInput);
    bf_log(b, "program", 1, "added counterexample (%u)\n", spec->testVectors->size);

    assert(b->assumps->size == 1);
    bfPopAssumptions(b, 1);
    continue;


Done:
    /* printf("\nProgram works for all inputs! Q.E.D.\n"); */
    /* printf("Needed %u inputs to synthesize.\n", spec->testVectors->size); */
    
    candProgram->totalSynthSecs = totalSynthSecs;
    candProgram->totalCheckSecs = totalEqSecs;
    bfPopAssumptions(b, b->assumps->size);
    program **ps;
    forVector (program **, ps, programs) {
      bfpDestroy(b, *ps, spec);
    }
    bfpDestroy(b, theProgram, spec);
    vectorDestroy(programs), free(programs);
    return candProgram;
  }
Failure:
  bfPopAssumptions(b, b->assumps->size);
  return NULL;
}
