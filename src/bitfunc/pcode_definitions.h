#ifndef AIG_PCODEDEF_H
#define AIG_PCODEDEF_H

#define BITS_IN_BYTE 8
//pcode function definitions

bitvec *pCOPY(bfman *b, bitvec *input0);
bitvec *pINT_ADD(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_SUB(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_MULT(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_DIV(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_REM(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_SDIV(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_SREM(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_OR(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_XOR(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_AND(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_LEFT(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_RIGHT(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_SRIGHT(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_EQUAL(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_NOTEQUAL(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_LESS(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_LESSEQUAL(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_CARRY(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_SLESS(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_SLESSEQUAL(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_SCARRY(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pINT_SBORROW(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pBOOL_OR(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pBOOL_XOR(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pBOOL_AND(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pBOOL_NEGATE(bfman *b, bitvec *input0);
bitvec *pPIECE(bfman *b, bitvec *input0, bitvec *input1);
bitvec *pSUBPIECE(bfman *b, bitvec *input0, uint32_t input1, uint32_t output_size);
bitvec *pINT_ZEXT(bfman *b, bitvec *input0, uint32_t output_size);
bitvec *pINT_SEXT(bfman *b, bitvec *input0, uint32_t output_size);
bitvec *pINT_2COMP(bfman *b, bitvec *input0);
bitvec *pINT_NEGATE(bfman *b, bitvec *input0);

//pcode helper functions

#endif
