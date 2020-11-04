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
#ifndef __USE_POSIX199309
#define __USE_POSIX199309
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <signal.h>
#include <zlib.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>
#include <getopt.h>

#include <funcsat.h>
#include <funcsat/memory.h>
#include <funcsat/system.h>
#include <funcsat/types.h>
#include <funcsat/internal.h>
#include <funcsat/restart.h>
#include <funcsat/debug.h>
#include <funcsat/hashtable.h>
#include <funcsat/frontend.h>
#include <funcsat/learning.h>
#include <bitfunc.h>
#include <bitfunc/array.h>
#include <bitfunc/circuits.h>
#include <bitfunc/program.h>
#include <bitfunc/pcode_definitions.h>

static aiger_literal lit_bf2aiger(literal l){
 if (l == LITERAL_FALSE) return aiger_false;
  if (l == LITERAL_TRUE) return aiger_true;
  if (l < 0) {
    return aiger_not(aiger_var2lit(-l));
  } else {
    return aiger_var2lit(l);
  }
}

void twoIntArgGen(bitvec *(*PCODE_OP)(bfman *, bitvec *, bitvec *), char *aig_name) {
  bfman *b = bfInit(AIG_MODE);
  bitvec *in0 = bfvInit(b, 64);
  bitvec *in1 = bfvInit(b, 64);
  bitvec *out = PCODE_OP(b, in0, in1);

  for(unsigned i = 0; i < in0->size; i++){
    aiger_add_input(b->aig, lit_bf2aiger(in0->data[i]), NULL);
  }

  for(unsigned i = 0; i < in1->size; i++){
    aiger_add_input(b->aig, lit_bf2aiger(in1->data[i]), NULL);
  }

  for(unsigned i = 0; i < out->size; i++){
    b->addAIGOutput(b, out->data[i]);
  }
  FILE *outfile = fopen(aig_name, "w");
  aiger_write_to_file(b->aig, aiger_binary_mode, outfile);
  fclose(outfile);
  bfDestroy(b);
  return;
}

void oneIntArgGen(bitvec *(*PCODE_OP)(bfman *, bitvec *), char *aig_name) {
  bfman *b = bfInit(AIG_MODE);
  bitvec *in0 = bfvInit(b, 64);
  bitvec *out = PCODE_OP(b, in0);

  for(unsigned i = 0; i < in0->size; i++){
    aiger_add_input(b->aig, lit_bf2aiger(in0->data[i]), NULL);
  }

  for(unsigned i = 0; i < out->size; i++){
    b->addAIGOutput(b, out->data[i]);
  }
  FILE *outfile = fopen(aig_name, "w");
  aiger_write_to_file(b->aig, aiger_binary_mode, outfile);
  fclose(outfile);
  bfDestroy(b);
  return;
}

void twoBoolArgGen(bitvec *(*PCODE_OP)(bfman *, bitvec *, bitvec *), char *aig_name) {
  bfman *b = bfInit(AIG_MODE);
  bitvec *in0 = bfvInit(b, BITS_IN_BYTE);
  bitvec *in1 = bfvInit(b, BITS_IN_BYTE);
  
  
  for(unsigned i = 1; i < BITS_IN_BYTE; i++){
    in0->data[i] = LITERAL_FALSE;
    in1->data[i] = LITERAL_FALSE;
  }

  bitvec *out = PCODE_OP(b, in0, in1);
  
  aiger_add_input(b->aig, lit_bf2aiger(in0->data[0]), NULL);
  aiger_add_input(b->aig, lit_bf2aiger(in1->data[0]), NULL);
  
  for(unsigned i = 0; i < out->size; i++){
    b->addAIGOutput(b, out->data[i]);
  }
  FILE *outfile = fopen(aig_name, "w");
  aiger_write_to_file(b->aig, aiger_binary_mode, outfile);
  fclose(outfile);
  bfDestroy(b);
  return;
}

void oneBoolArgGen(bitvec *(*PCODE_OP)(bfman *, bitvec *), char *aig_name) {
  bfman *b = bfInit(AIG_MODE);
  bitvec *in0 = bfvInit(b, BITS_IN_BYTE);
  
  for(unsigned i = 1; i < BITS_IN_BYTE; i++){
    in0->data[i] = LITERAL_FALSE;
  }

  bitvec *out = PCODE_OP(b, in0);
  
  aiger_add_input(b->aig, lit_bf2aiger(in0->data[0]), NULL);
  
  for(unsigned i = 0; i < out->size; i++){
    b->addAIGOutput(b, out->data[i]);
  }
  FILE *outfile = fopen(aig_name, "w");
  aiger_write_to_file(b->aig, aiger_binary_mode, outfile);
  fclose(outfile);
  bfDestroy(b);
  return;
}

void extendGen(bitvec *(*PCODE_OP)(bfman *, bitvec *, uint32_t), char *aig_name){
  bfman *b = bfInit(AIG_MODE);
  bitvec *in0 = bfvInit(b, 8);
  bitvec *out = PCODE_OP(b, in0, 2);

  for(unsigned i = 0; i < in0->size; i++){
    aiger_add_input(b->aig, lit_bf2aiger(in0->data[i]), NULL);
  }

  for(unsigned i = 0; i < out->size; i++){
    b->addAIGOutput(b, out->data[i]);
  }
  FILE *outfile = fopen(aig_name, "w");
  aiger_write_to_file(b->aig, aiger_binary_mode, outfile);
  fclose(outfile);
  bfDestroy(b);
  return;  
}

void subpieceGen(bitvec *(*PCODE_OP)(bfman *, bitvec *, uint32_t, uint32_t), char *aig_name){
  bfman *b = bfInit(AIG_MODE);
  bitvec *in0 = bfvInit(b, 16);
  bitvec *out = PCODE_OP(b, in0, 1, 2);

  for(unsigned i = 0; i < in0->size; i++){
    aiger_add_input(b->aig, lit_bf2aiger(in0->data[i]), NULL);
  }

  for(unsigned i = 0; i < out->size; i++){
    b->addAIGOutput(b, out->data[i]);
  }
  FILE *outfile = fopen(aig_name, "w");
  aiger_write_to_file(b->aig, aiger_binary_mode, outfile);
  fclose(outfile);
  bfDestroy(b);
  return;  
}


int main(){
  twoIntArgGen(pINT_ADD, "INT_ADD.aig");
  twoIntArgGen(pINT_SUB, "INT_SUB.aig");
  twoIntArgGen(pINT_MULT, "INT_MULT.aig");
  twoIntArgGen(pINT_DIV, "INT_DIV.aig");
  twoIntArgGen(pINT_REM, "INT_REM.aig");
  twoIntArgGen(pINT_SDIV, "INT_SDIV.aig");
  twoIntArgGen(pINT_SREM, "INT_SREM.aig");
  twoIntArgGen(pINT_OR, "INT_OR.aig");
  twoIntArgGen(pINT_XOR, "INT_XOR.aig");
  twoIntArgGen(pINT_AND, "INT_AND.aig");
  twoIntArgGen(pINT_LEFT, "INT_LEFT.aig");
  twoIntArgGen(pINT_RIGHT, "INT_RIGHT.aig");
  twoIntArgGen(pINT_SRIGHT, "INT_SRIGHT.aig");
  twoIntArgGen(pINT_EQUAL, "INT_EQUAL.aig");
  twoIntArgGen(pINT_NOTEQUAL, "INT_NOTEQUAL.aig");
  twoIntArgGen(pINT_LESS, "INT_LESS.aig");
  twoIntArgGen(pINT_LESSEQUAL, "INT_LESSEQUAL.aig");
  twoIntArgGen(pINT_CARRY, "INT_CARRY.aig");
  twoIntArgGen(pINT_SCARRY, "INT_SCARRY.aig");
  twoIntArgGen(pINT_SBORROW, "INT_SBORROW.aig");
  twoBoolArgGen(pBOOL_OR, "BOOL_OR.aig");
  twoBoolArgGen(pBOOL_XOR, "BOOL_XOR.aig");
  twoBoolArgGen(pBOOL_AND, "BOOL_AND.aig");
  twoBoolArgGen(pBOOL_OR, "BOOL_OR.aig");
  oneBoolArgGen(pBOOL_NEGATE, "BOOL_NEGATE.aig");
  twoIntArgGen(pPIECE, "PIECE.aig");
  subpieceGen(pSUBPIECE, "SUBPIECE.aig");
  extendGen(pINT_ZEXT, "INT_ZEXT.aig");
  extendGen(pINT_SEXT, "INT_SEXT.aig");
  oneIntArgGen(pINT_2COMP, "INT_2COMP.aig");
  oneIntArgGen(pINT_NEGATE, "INT_NEGATE.aig");
  return 0;
}


