/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <perfprobes/probes.h>

METER(hash_miss_meter, "hash misses", EWMA_NSEC(2), EWMA_10SEC, EWMA_30SEC);
METER(hash_hit_meter, "hash hit", EWMA_NSEC(2), EWMA_10SEC, EWMA_30SEC);
METER(var_alloc_meter, "var allocations", EWMA_10SEC, EWMA_30SEC);
METER(solve_meter, "solve calls", EWMA_10SEC, EWMA_30SEC);
COUNTER(bitvec_count, "bitvecs");

static int done_register = 0;

void register_probes()
{
  if (done_register) return;
  done_register = 1;
  register_meter(&hash_miss_meter);
  register_meter(&hash_hit_meter);
  register_meter(&var_alloc_meter);
  register_meter(&solve_meter);
  register_counter(&bitvec_count);
}
