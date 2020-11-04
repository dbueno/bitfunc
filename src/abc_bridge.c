/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */

#include <bitfunc/config.h>

#ifdef HAVE_LIBABC

#include <assert.h>
#include <inttypes.h>
#include <base/abc/abc.h>
#include <aig/gia/gia.h>
#include <aig/gia/giaAig.h>
#include <base/main/main.h>
#include <sat/bsat/satSolver.h>
#include <proof/cec/cec.h>
#include <bitfunc/aiger.h>
#include <unistd.h>
#include <sys/wait.h>
#include <funcsat/vec.h>
#include <funcsat/intvec.h>
#include <funcsat/hashtable.h>

/* this file is a bridge to ABC, when ABC is linked with bitfunc. It allows one
 * to equivalence check & solve with ABC instead of another backend, without
 * touching the filesystem. */


/* the map between BF objects and the corresponding ABC objects */

struct bf_gia_map
{
  struct hashtable *map_inputs; /* maps strip'd aiger_literals to int PIs */
  struct hashtable *map_ands;   /* maps strip'd aiger_literals to int ANDs */
};

static struct bf_gia_map *bf_gia_map_init()
{
  struct bf_gia_map *map = calloc(1, sizeof(*map));
  map->map_inputs = create_hashtable(16, hash_aiger_literal, aiger_literal_equal);
  map->map_ands   = create_hashtable(16, hash_aiger_literal, aiger_literal_equal);
  return map;
}

static void bf_gia_map_destroy(struct bf_gia_map *map)
{
  hashtable_destroy(map->map_inputs, true, false);
  hashtable_destroy(map->map_ands, true, false);
  free(map);
}
DEFINE_HASHTABLE(bf_gia_map_insert, bf_gia_map_search, bf_gia_map_remove, aiger_literal, int)


/* ------------------------------------------------------------------------ */
                       /* convert BF AIG to GIA in ABC */
/* ------------------------------------------------------------------------ */


/* find the literal if it's in the bf_abc map, otherwise return NULL. Always
 * "finds" PIs, and will create them on demand. */
static int bf_get_abclit(aiger *aig, Gia_Man_t *gia, aiger_literal l,
                         struct bf_gia_map *map)
{
  aiger_literal l_pos = aiger_strip(l);
  aiger_and *and = NULL;
  int l_abc = 0, *l_abcp = NULL;
  if (aiger_is_input(aig, l_pos)) {
    if (!(l_abcp = bf_gia_map_search(map->map_inputs, &l_pos))) {
      aiger_literal *key = calloc(1, sizeof(*key));
      *key = l_pos;
      l_abc = Gia_ManAppendCi(gia);
      l_abcp = calloc(1, sizeof(*l_abcp));
      *l_abcp = l_abc;
      bf_gia_map_insert(map->map_inputs, key, l_abcp);
    }
  } else if ((and = aiger_is_and(aig, l_pos))) {
    l_abcp = bf_gia_map_search(map->map_ands, &l_pos);
  }
  if (l_abcp) {
    l_abc = *l_abcp;
    if (aiger_sign(l)) {
      l_abc = Abc_LitNot(l_abc);
    }
  }

  return l_abc;
}

#define CAST_IM2AL(IM) *(aiger_literal *)&(IM)
#define CAST_AL2IM(AL) *(int *)&(AL)

/* returns the ABC literal for the giver aiger_literal by converting (up to) the
 * entire cone of influence of the aiger_literal into ABC objects. */
static int bf_get_abclit_coi(aiger *aig, Gia_Man_t *gia,
                             struct bf_gia_map *map, aiger_literal l)
{
  intvec *stack = intvecAlloc(2);
  intvecPush(stack, CAST_AL2IM(l)); /* stack of literals, unstripped & possibly negated */

  while (stack->size > 0) {
    int *elt;
    int top = intvecPeek(stack);
    aiger_literal p = CAST_IM2AL(top);
    aiger_literal p_pos = aiger_strip(p);

    int abc_p = 0, abc_r0 = 0, abc_r1 = 0;
    aiger_and *and = NULL;
    if (aiger_is_input(aig, p_pos)) {
      abc_p = bf_get_abclit(aig, gia, p, map);
      assert(abc_p && "input is null");
      intvecPop(stack);
    } else if ((and = aiger_is_and(aig, p_pos))) {
      if (!(abc_p = bf_get_abclit(aig, gia, p, map))) {
        if (!(abc_r0 = bf_get_abclit(aig, gia, and->rhs0, map))) {
          intvecPush(stack, CAST_AL2IM(and->rhs0));
        }
        if (!(abc_r1 = bf_get_abclit(aig, gia, and->rhs1, map))) {
          intvecPush(stack, CAST_AL2IM(and->rhs1));
        }
        if (abc_r0 && abc_r1) {
          aiger_literal *key = calloc(1, sizeof(*key));
          *key = p_pos;
          abc_p = Gia_ManHashAnd(gia, abc_r0, abc_r1);
          int *value = calloc(1, sizeof(*value));
          *value = abc_p;
          bf_gia_map_insert(map->map_ands, key, value);
          intvecPop(stack);
        }
      } else {
        intvecPop(stack);
      }
    }
  }

  return bf_get_abclit(aig, gia, l, map);
}


struct Cec_ParCec_t_ params =
{
  .nBTLimit = 0,
  .TimeLimit = 0,     // the runtime limit in seconds
//    int              fFirstStop;    // stop on the first sat output
  .fUseSmartCnf = 1,  // use smart CNF computation
  .fRewriting = 1,    // enables AIG rewriting
  .fVeryVerbose = 0,  // verbose stats
  .fVerbose = 0,      // verbose stats
  .iOutFail = -1      // the number of failed output
};

/* #define BFEQCHECK_FORK_ABC */

int bfEqCheck_ABC(aiger *aig, aiger_literal x, aiger_literal y)
{

#ifdef BFEQCHECK_FORK_ABC
  pid_t child = fork();
  if (child) {
    int stat_loc;
    pid_t ret = wait(&stat_loc);
    /* assert(!WIFSIGNALED(stat_loc)); */
    /* assert(!WIFSTOPPED(stat_loc)); */
    fprintf(stderr, "child returned %d %d\n", ret, WEXITSTATUS(stat_loc));
    return !WEXITSTATUS(stat_loc);
  }
#endif

  int eq_check_result = 0;
  Gia_Man_t *gia = Gia_ManStart(16), *tmp;
  Gia_ManHashAlloc(gia);
  struct bf_gia_map *map = bf_gia_map_init(aig);
  int x_abc = bf_get_abclit_coi(aig, gia, map, x);
  int y_abc = bf_get_abclit_coi(aig, gia, map, y);

  Gia_ManAppendCo(gia, x_abc);
  Gia_ManAppendCo(gia, y_abc);

  Gia_ManHashStop(gia);
  gia = Gia_ManCleanup(tmp = gia);
  Gia_ManStop(tmp);
  Gia_ManSetRegNum(gia, 0);
  eq_check_result = Cec_ManVerify(gia, &params);

  bf_gia_map_destroy(map);
#ifdef BFEQCHECK_FORK_ABC
  exit(eq_check_result != 1);
#else
  return eq_check_result;
#endif
}

#endif
