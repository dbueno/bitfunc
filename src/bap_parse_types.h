/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#ifndef bap_parse_types_included_h
#define bap_parse_types_included_h

#include <bitfunc.h>
#include <bitfunc/mem.h>

#include <bitfunc/bap.h>

typedef struct lex_state lex_state;

#define YY_EXTRA_TYPE lex_state *

#ifndef FLEX_SCANNER
#include "bap_lex.h"
#endif

/* define token type */
#ifndef YYSTYPE
typedef struct {
  unsigned line;
  unsigned col;
  uint64_t i;                   /* value, if it's an integer token */
  char *ident;                  /* name, if it has one. or string lit */
} bap_token;
# define YYSTYPE bap_token
# define YYSTYPE_IS_TRIVIAL 1
#endif

struct lex_state {
  YYSTYPE token;

  int line, col;
  bool lock_loc;
  char *string_buf_ptr;
};

inline void destroy_token(bap_token *tok) {
  if (tok->ident != NULL) free(tok->ident);
  free(tok);
}

inline void init_state(struct lex_state *state) {
  state->lock_loc = false;
  state->line = 1;
  state->col = 1;
  memset(&state->token, 0, sizeof(state->token));
}

extern void *ParseAlloc(void *(*mallocProc)(size_t));
extern void Parse(void *parser, int tokid, bap_token *, bap_machine *);
extern void ParseFree(void *parser, void (*freefn)(void*));
extern void ParseTrace(FILE *stream, char *zPrefix);


#endif
