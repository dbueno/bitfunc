/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <funcsat/hashtable_itr.h>

#include "bap_parse_types.h"
#include "bap_parse.h"

#define UNUSED(x) (void)(x)

DEFINE_HASHTABLE(bapSymbolInsert, bapSymbolSearch, bapSymbolRemove, char, bap_val)


/* make sure init_state shows up in the exports. */
extern void init_state(struct lex_state *state);

void bap_set_binding(bap_machine *MS, char *name, bap_val *v) {
  bap_clear_binding(MS, name);
  bapSymbolInsert(MS->bindings, strdup(name), bap_val_ref(MS, v));
}

void bap_clear_binding(bap_machine *MS, char *name) {
  // printf("clearing binding for %s\n", name);
  bap_val *old = bapSymbolRemove(MS->bindings, name);
  if (old != NULL) {
    // printf("unrefing\n");
    bap_val_unref(MS, old);
  }
}

bap_val *bap_lookup_ident(bap_machine *MS, char *name, bap_type t) {
  bap_val *b = bapSymbolSearch(MS->bindings, name);
  if (b != NULL) {
    /* FIXME check types */
    return bap_val_ref(MS, b);
  }
  bap_val *v;
  switch (t.kind) {
    case BAP_TYPE_KIND_BV: 
      {
        bitvec *bv = bfvInit(MS->bf, t.val_width);
        bap_set_binding(MS, name, v = bap_val_new_bv(bv));
      }
      break;
    case BAP_TYPE_KIND_ARRAY:
      {
        memory *m = bfmInit(MS->bf, t.idx_width, t.val_width);
        bap_set_binding(MS, name, v = bap_val_new_array(m));
      }
      break;
    default:
      fprintf(stderr, "Can't make a fresh %d\n", t.kind);
      MS->has_error = true;
      v = NULL;
  }
  return v;
}

char *bap_describe_type(bap_type t) {
  char buf[64];
  switch (t.kind) {
    case BAP_TYPE_KIND_BV:
      snprintf(buf, 64, "u%u", t.val_width);
      break;
    case BAP_TYPE_KIND_MEM:
      snprintf(buf, 64, "?u%u", t.val_width);
      break;
    case BAP_TYPE_KIND_ARRAY:
      snprintf(buf, 64, "u%u?u%u", t.idx_width, t.val_width);
      break;
    case BAP_TYPE_KIND_STR:
      strncpy(buf, "string", 63);
      break;
    default:
      strncpy(buf, "unknown", 63);
      break;
  }
  return strdup(buf);
}

bap_val *bap_val_new_bv(bitvec *bv) {
  bap_val *e = malloc(sizeof(bap_val));
  e->refcnt = 1;
  e->v.bv = bfvHold(bv);
  e->typ.flags = BAP_TYPE_FLAG_VALID;
  e->typ.kind = BAP_TYPE_KIND_BV;
  e->typ.idx_width = 0;
  e->typ.val_width = bv->size;
  return e;
}

bap_val *bap_val_new_array(memory *mem) {
  bap_val *e = malloc(sizeof(bap_val));
  e->refcnt = 1;
  e->v.mem = mem;
  e->typ.flags = BAP_TYPE_FLAG_VALID;
  e->typ.kind = BAP_TYPE_KIND_ARRAY;
  e->typ.idx_width = mem->idxsize;
  e->typ.val_width = mem->eltsize;
  return e;
}

bap_val *bap_val_new_string(char *s) {
  bap_val *e = malloc(sizeof(bap_val));
  e->refcnt = 1;
  e->v.str = strdup(s);
  e->typ.flags = BAP_TYPE_FLAG_VALID;
  e->typ.kind = BAP_TYPE_KIND_STR;
  e->typ.idx_width = 0;
  e->typ.val_width = 0;
  return e;
}

bap_val *bap_val_new_bot(bap_type t) {
  bap_val *e = malloc(sizeof(bap_val));
  e->refcnt = 1;
  e->v.str = 0;
  e->v.bv = 0;
  e->v.mem = 0;
  e->typ = t;
  return e;
}

bap_val *bap_val_ref(bap_machine *MS, bap_val *v) {
  UNUSED(MS);
  if (v != NULL) {
    v->refcnt++;
  }
  return v;
}

bap_val *bap_val_unref(bap_machine *MS, bap_val *v) {
  if (v == NULL) return NULL;
  assert(v->refcnt != 0);
  v->refcnt--;
  if (!v->refcnt) {
    switch (v->typ.kind) {
      case BAP_TYPE_KIND_BV:
        if (v->v.bv != NULL) {
          bfvRelease(v->v.bv);
          CONSUME_BITVEC(v->v.bv);
        }
        break;
      case BAP_TYPE_KIND_ARRAY:
        if (v->v.mem != NULL) {
          bfmDestroy(MS->bf, v->v.mem);
        }
        break;
      case BAP_TYPE_KIND_MEM:
        break;
      case BAP_TYPE_KIND_STR:
        if (v->v.str != NULL) {
          free(v->v.str);
        }
        break;
    }
    free(v);
    return NULL;
  } else {
    return v;
  }
}

bap_machine *bap_create_machine(bfman *bf) {
  bap_machine *mach = malloc(sizeof(bap_machine));
  symbol_table bindings = create_hashtable(16, hashString, stringEqual);
  *mach = (bap_machine) { bf, bindings, false, false, bfvHold(bfvInit(bf, 0)) };
  return mach;
}

void bap_destroy_machine(bap_machine *mach) {
  struct hashtable_itr *i = hashtable_iterator(mach->bindings);
  do {
    bap_val *b = (bap_val*)hashtable_iterator_value(i);
    bap_val_unref(mach, b);
  } while (hashtable_iterator_advance(i));
  free(i);
  hashtable_destroy(mach->bindings, true, false);
  CONSUME_BITVEC(bfvRelease(mach->path_constraints));
  free(mach);
}

bool bap_types_equal(bap_type *t1, bap_type *t2)
{
  if (t1->idx_width != t2->idx_width) return false;
  if (t1->val_width != t2->val_width) return false;
  if (t1->kind != t2->kind) return false;
  if (t1->flags != t2->flags) return false;
  return true;
}

#ifndef NDEBUG
#define DEBUG_PERROR(x) perror(x)
#else
#define DEBUG_PERROR(x) (void)0
#endif

bool bap_execute_il(bap_machine *mach, char *ilfile)
{
  yyscan_t scanner;
  lex_state state;
  FILE *in;

  init_state(&state);

  if (yylex_init_extra(&state, &scanner)) {
    DEBUG_PERROR("Initializing scanner");
    return false;
  }

  in = fopen(ilfile, "r");
  if (in == NULL) {
    DEBUG_PERROR("Opening input");
    return false;
  }

  yyset_in(in, scanner);

  void *parser = ParseAlloc(malloc);

  int yv;

  while ((yv = yylex(scanner)) != 0) {
    bap_token *tok = malloc(sizeof(bap_token));
    memcpy(tok, &state.token, sizeof(bap_token));
    // fprintf(stderr, "%p = yylex() %d %" PRIu64 " %p %d %d\n", tok, yv, tok->i, tok->ident, tok->line, tok->col);
    Parse(parser, yv, tok, mach);
    memset(&state.token, 0, sizeof(state.token));
    if ((yv == BAP_TOK_UNTERMINATED_STRING) || (yv == BAP_TOK_UNEXPECTED)) {
      break;
    }
    if (mach->has_error) {
      break;
    }
    if (mach->halted) {
      break;
    }
  }

  Parse(parser, 0, &state.token, mach);

  ParseFree(parser, free);
  fclose(in);
  yylex_destroy(scanner);

  
  return true;
}
