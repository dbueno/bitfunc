/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <funcsat/config.h>

#include "funcsat/types.h"
#include "funcsat/restart.h"

void incLubyRestart(funcsat *f, bool skip);

bool funcsatNoRestart(funcsat *f, void *p) {
  return false;
}

bool funcsatLubyRestart(funcsat *f, void *p)
{
  if ((int)f->numConflicts >= f->lrestart && f->decisionLevel > 2) {
    incLubyRestart(f, false);
    return true;
  }
  return false;
}

bool funcsatInnerOuter(funcsat *f, void *p)
{
  static uint64_t inner = 100UL, outer = 100UL, conflicts = 1000UL;
  if (f->numConflicts >= conflicts) {
    conflicts += inner;
    if (inner >= outer) {
      outer *= 1.1;
      inner = 100UL;
    } else {
      inner *= 1.1;
    }
    return true;
  }
  return false;
}

bool funcsatMinisatRestart(funcsat *f, void *p)
{
  static uint64_t cutoff = 100UL;
  if (f->numConflicts >= cutoff) {
    cutoff *= 1.5;
    return true;
  }
  return false;
}



/* This stuff was cribbed from picosat and changed a smidge just to get bigger
 * integers. */


int64_t luby(int64_t i)
{
  int64_t k;
  for (k = 1; k < (int64_t)sizeof(k); k++)
    if (i == (1 << k) - 1)
      return 1 << (k - 1);

  for (k = 1;; k++)
    if ((1 << (k - 1)) <= i && i < (1 << k) - 1)
      return luby (i - (1 << (k-1)) + 1);
}

void incLubyRestart(funcsat *f, bool skip)
{
  uint64_t delta;

  /* Luby calculation takes a really long time around 255? */
  if (f->lubycnt > 250) {
    f->lubycnt = 0;
    f->lubymaxdelta = 0;
    f->waslubymaxdelta = false;
  }
  delta = 100 * (uint64_t)luby((int64_t)++f->lubycnt);
  f->lrestart = (int64_t)(f->numConflicts + delta);

  /* if (waslubymaxdelta) */
  /*   report (1, skip ? 'N' : 'R'); */
  /* else */
  /*   report (2, skip ? 'n' : 'r'); */

  if (delta > f->lubymaxdelta) {
    f->lubymaxdelta = delta;
    f->waslubymaxdelta = 1;
  } else {
    f->waslubymaxdelta = 0;
  }
}
