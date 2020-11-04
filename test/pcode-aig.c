#define set_bool_byte(bv) for(int i= 1;i<BITS_IN_BYTE;i++) (bv) ->data[i]= LITERAL_FALSE; \

/*1:*/
#line 23 "./pcode-aig.w"

/*4:*/
#line 287 "./pcode-aig.w"

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
#include <limits.h> 
#include <getopt.h> 

#include <bitfunc.h> 
#include <bitfunc/pcode_definitions.h> 
#include <bitfunc/circuits_internal.h> /*:4*/
#line 24 "./pcode-aig.w"


/*2:*/
#line 155 "./pcode-aig.w"

void twoIntArgGen(bitvec*(*PCODE_OP)(bfman*,bitvec*,bitvec*),char*aig_name)
{
bfman*b= bfInit(AIG_MODE);
bitvec*in0= bfvInit(b,64);
bitvec*in1= bfvInit(b,64);
bitvec*out= PCODE_OP(b,in0,in1);

for(unsigned i= 0;i<out->size;i++){
b->addAIGOutput(b,out->data[i]);
}
FILE*outfile= fopen(aig_name,"w");
aiger_write_to_file(b->aig,aiger_binary_mode,outfile);
fclose(outfile);
fprintf(stderr,"Wrote '%s'.\n",aig_name);
CONSUME_BITVEC(out);
bfDestroy(b);
}

/*:2*//*3:*/
#line 176 "./pcode-aig.w"


void oneIntArgGen(bitvec*(*PCODE_OP)(bfman*,bitvec*),char*aig_name)
{
bfman*b= bfInit(AIG_MODE);
bitvec*in0= bfvInit(b,64);
bitvec*out= PCODE_OP(b,in0);

for(unsigned i= 0;i<out->size;i++){
b->addAIGOutput(b,out->data[i]);
}
FILE*outfile= fopen(aig_name,"w");
aiger_write_to_file(b->aig,aiger_binary_mode,outfile);
fclose(outfile);
fprintf(stderr,"Wrote '%s'.\n",aig_name);
CONSUME_BITVEC(out);
bfDestroy(b);
return;
}

void twoBoolArgGen(bitvec*(*PCODE_OP)(bfman*,bitvec*,bitvec*),char*aig_name)
{
bfman*b= bfInit(AIG_MODE);
bitvec*in0= bfvAlloc(BITS_IN_BYTE);in0->size= BITS_IN_BYTE;
bitvec*in1= bfvAlloc(BITS_IN_BYTE);in1->size= BITS_IN_BYTE;

in0->data[0]= bfNewVar(b);
in1->data[0]= bfNewVar(b);
for(unsigned i= 1;i<BITS_IN_BYTE;i++){
in0->data[i]= LITERAL_FALSE;
in1->data[i]= LITERAL_FALSE;
}

bitvec*out= PCODE_OP(b,in0,in1);

for(unsigned i= 0;i<out->size;i++){
b->addAIGOutput(b,out->data[i]);
}
FILE*outfile= fopen(aig_name,"w");
aiger_write_to_file(b->aig,aiger_binary_mode,outfile);
fclose(outfile);
fprintf(stderr,"Wrote '%s'.\n",aig_name);
CONSUME_BITVEC(out);
bfDestroy(b);
return;
}

void oneBoolArgGen(bitvec*(*PCODE_OP)(bfman*,bitvec*),char*aig_name)
{
bfman*b= bfInit(AIG_MODE);
bitvec*in0= bfvAlloc(BITS_IN_BYTE);in0->size= BITS_IN_BYTE;

in0->data[0]= bfNewVar(b);
for(unsigned i= 1;i<BITS_IN_BYTE;i++){
in0->data[i]= LITERAL_FALSE;
}

bitvec*out= PCODE_OP(b,in0);

for(unsigned i= 0;i<out->size;i++){
b->addAIGOutput(b,out->data[i]);
}
FILE*outfile= fopen(aig_name,"w");
aiger_write_to_file(b->aig,aiger_binary_mode,outfile);
fclose(outfile);
fprintf(stderr,"Wrote '%s'.\n",aig_name);
CONSUME_BITVEC(out);
bfDestroy(b);
return;
}

void extendGen(bitvec*(*PCODE_OP)(bfman*,bitvec*,uint32_t),char*aig_name)
{
bfman*b= bfInit(AIG_MODE);
bitvec*in0= bfvInit(b,8);
bitvec*out= PCODE_OP(b,in0,2);

for(unsigned i= 0;i<out->size;i++){
b->addAIGOutput(b,out->data[i]);
}
FILE*outfile= fopen(aig_name,"w");
aiger_write_to_file(b->aig,aiger_binary_mode,outfile);
fclose(outfile);
fprintf(stderr,"Wrote '%s'.\n",aig_name);
CONSUME_BITVEC(out);
bfDestroy(b);
return;
}

void subpieceGen(bitvec*(*PCODE_OP)(bfman*,bitvec*,uint32_t,uint32_t),char*aig_name)
{
bfman*b= bfInit(AIG_MODE);
bitvec*in0= bfvInit(b,16);
bitvec*out= PCODE_OP(b,in0,1,2);

for(unsigned i= 0;i<out->size;i++){
b->addAIGOutput(b,out->data[i]);
}
FILE*outfile= fopen(aig_name,"w");
aiger_write_to_file(b->aig,aiger_binary_mode,outfile);
fclose(outfile);
fprintf(stderr,"Wrote '%s'.\n",aig_name);
CONSUME_BITVEC(out);
bfDestroy(b);
return;
}



/*:3*/
#line 26 "./pcode-aig.w"


int main(int argc,char**argv)
{
if(argc<2){
fprintf(stderr,"need some more arguments, sucka\n");
exit(1);
}

bfman*b= bfInit(AIG_MODE);
bitvec*x= bfvInit(b,1*BITS_IN_BYTE);
bitvec*y= NULL;
bitvec*ret= NULL;
char*filename= argv[1];


if(0==strcmp("pINT_ADD.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_ADD(b,x,y);
}else if(0==strcmp("pINT_SUB.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_SUB(b,x,y);
}else if(0==strcmp("pINT_MULT.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_MULT(b,x,y);
}else if(0==strcmp("pINT_DIV.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_DIV(b,x,y);
}else if(0==strcmp("pINT_REM.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_REM(b,x,y);
}else if(0==strcmp("pINT_SDIV.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_SDIV(b,x,y);
}else if(0==strcmp("pINT_SREM.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_SREM(b,x,y);
}else if(0==strcmp("pINT_OR.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_OR(b,x,y);
}else if(0==strcmp("pINT_XOR.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_XOR(b,x,y);
}else if(0==strcmp("pINT_AND.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_AND(b,x,y);
}else if(0==strcmp("pINT_LEFT.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_LEFT(b,x,y);
}else if(0==strcmp("pINT_RIGHT.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_RIGHT(b,x,y);
}else if(0==strcmp("pINT_SRIGHT.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_SRIGHT(b,x,y);
}else if(0==strcmp("pINT_EQUAL.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_EQUAL(b,x,y);
}else if(0==strcmp("pINT_NOTEQUAL.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_NOTEQUAL(b,x,y);
}else if(0==strcmp("pINT_LESS.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_LESS(b,x,y);
}else if(0==strcmp("pINT_LESSEQUAL.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_LESSEQUAL(b,x,y);
}else if(0==strcmp("pINT_CARRY.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_CARRY(b,x,y);
}else if(0==strcmp("pINT_SLESS.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_SLESS(b,x,y);
}else if(0==strcmp("pINT_SLESSEQUAL.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_SLESSEQUAL(b,x,y);
}else if(0==strcmp("pINT_SCARRY.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_SCARRY(b,x,y);
}else if(0==strcmp("pINT_SBORROW.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pINT_SBORROW(b,x,y);
}else if(0==strcmp("pBOOL_OR.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
set_bool_byte(x);set_bool_byte(y);
ret= pBOOL_OR(b,x,y);
}else if(0==strcmp("pBOOL_XOR.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
set_bool_byte(x);set_bool_byte(y);
ret= pBOOL_XOR(b,x,y);
}else if(0==strcmp("pBOOL_AND.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
set_bool_byte(x);set_bool_byte(y);
ret= pBOOL_AND(b,x,y);
}else if(0==strcmp("pBOOL_NEGATE.aig",argv[1])){
set_bool_byte(x);
ret= pBOOL_NEGATE(b,x);
}else if(0==strcmp("pPIECE.aig",argv[1])){
y= bfvInit(b,1*BITS_IN_BYTE);
ret= pPIECE(b,x,y);
}else if(0==strcmp("pSUBPIECE.aig",argv[1])){
ret= pSUBPIECE(b,x,4,4);
}else if(0==strcmp("pINT_ZEXT.aig",argv[1])){
ret= pINT_ZEXT(b,x,4);
}else if(0==strcmp("pINT_SEXT.aig",argv[1])){
ret= pINT_SEXT(b,x,12);
}else if(0==strcmp("pINT_2COMP.aig",argv[1])){
ret= pINT_2COMP(b,x);
}else if(0==strcmp("pINT_NEGATE.aig",argv[1])){
ret= pINT_NEGATE(b,x);
}else{
printf("Unrecognized operation '%s'\n",argv[1]);
filename= NULL;
}

if(ret){
for(unsigned i= 0;i<ret->size;i++){
b->addAIGOutput(b,ret->data[i]);
}
}

if(filename)bfWriteAigerToFile(b,filename);

return 0;
}

/*:1*/
