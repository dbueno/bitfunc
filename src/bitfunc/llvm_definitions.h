#ifndef llvm_definitions_h_included
#define llvm_definitions_h_included


bitvec *bf_llvm_add(bfman *b, bitvec *input0, bitvec *input1,
                    literal *sw, literal *uw);
bitvec *bf_llvm_sub(bfman *b, bitvec *input0, bitvec *input1,
                    literal *sw, literal *uw);
bitvec *bf_llvm_mul(bfman *b, bitvec *input0, bitvec *input1,
                    literal *sw, literal *uw);
bitvec *bf_llvm_udiv(bfman *b, bitvec *input0, bitvec *input1,
                     literal *exact);
bitvec *bf_llvm_sdiv(bfman *b, bitvec *input0, bitvec *input1,
                     literal *exact);
bitvec *bf_llvm_urem(bfman *b, bitvec *input0, bitvec *input1);
bitvec *bf_llvm_srem(bfman *b, bitvec *input0, bitvec *input1);
bitvec *bf_llvm_shl(bfman *b, bitvec *input0, bitvec *input1,
                    literal *sw, literal *uw); 
bitvec *bf_llvm_lshr(bfman *b, bitvec *input0, bitvec *input1,
                     literal *exact); 
bitvec *bf_llvm_ashr(bfman *b, bitvec *input0, bitvec *input1,
                     literal *exact); 
bitvec *bf_llvm_sext(bfman *b, bitvec *input0, unsigned new_width);
bitvec *bf_llvm_and(bfman *b, bitvec *input0, bitvec *input1);
bitvec *bf_llvm_or(bfman *b, bitvec *input0, bitvec *input1);
bitvec *bf_llvm_xor(bfman *b, bitvec *input0, bitvec *input1);


enum icmp_comparison {
  ICMP_EQ, /* equal */
  ICMP_NE, /* not equal */
  ICMP_UGT, /* unsigned greater than */
  ICMP_UGE, /* unsigned greater or equal */
  ICMP_ULT, /* unsigned less than */
  ICMP_ULE, /* unsigned less or equal */
  ICMP_SGT, /* signed greater than */
  ICMP_SGE, /* signed greater or equal */
  ICMP_SLT, /* signed less than */
  ICMP_SLE, /* signed less or equal */
  ICMP_LAST
};
bitvec *bf_llvm_icmp(bfman *b, enum icmp_comparison cmp, bitvec *input0, bitvec *input1);


#endif
