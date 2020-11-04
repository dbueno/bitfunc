#ifndef sparsebits_h_included
#define sparsebits_h_included

#include <stdbool.h>
#include <stdint.h>

typedef struct sparse_bitset sparse_bitset;

sparse_bitset *sbs_create();
void sbs_destroy(sparse_bitset *);

bool sbs_test(sparse_bitset *, unsigned);
void sbs_set(sparse_bitset *, unsigned);
void sbs_reset(sparse_bitset *, unsigned);

#endif
