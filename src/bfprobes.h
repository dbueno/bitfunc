/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#ifndef BFPROBES_H
#define BFPROBES_H

#include "config.h"
#if HAVE_PERFPROBES

#include <perfprobes/probes.h>

extern struct probe_meter hash_miss_meter;
extern struct probe_meter hash_hit_meter;
extern struct probe_meter var_alloc_meter;
extern struct probe_meter solve_meter;
extern struct probe_counter bitvec_count;

extern void register_probes();

#define PROBE(x) x

#else

#define PROBE(x) (void)0

#endif

#endif
