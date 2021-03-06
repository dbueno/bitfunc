#include <bitfunc/config.h>

/***************************************************************************
Copyright (c) 2006-2007, Armin Biere, Johannes Kepler University.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
***************************************************************************/

#include <bitfunc/aiger.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

unsigned hash_aiger_literal(void *p)
{
  unsigned hashval = 0;
  aiger_literal l = *(aiger_literal *)p;
  if (sizeof(l) == 4) {
    hashval = l;
//  else if (sizeof(l) == 8) {
//    unsigned x = l & 0xffffffff;
//    unsigned y = (l >> 32) & 0xffffffff;
//    hashval += (unsigned)x;
//    hashval *= 17;
//    hashval += (unsigned)y;
  } else abort();
  return hashval;
}

int aiger_literal_equal(void *k1, void *k2)
{
  aiger_literal p = *(aiger_literal *)k1, q = *(aiger_literal *)k2;
  return p == q;
}

extern FILE *popen (const char *command, const char *modes);
extern int pclose (FILE *stream);

/*------------------------------------------------------------------------*/

const char *
aiger_id (void)
{
  return "$Id: aiger.c,v 1.94 2007-08-14 08:12:55 biere Exp $";
}

/*------------------------------------------------------------------------*/

const char *
aiger_version (void)
{
  return AIGER_VERSION;
}

/*------------------------------------------------------------------------*/

#define GZIP "gzip -c > %s 2>/dev/null"
#define GUNZIP "gunzip -c %s 2>/dev/null"

#define NEWN(p,n) \
  do { \
    size_t bytes = (n) * sizeof (*(p)); \
    (p) = private->malloc_callback (private->memory_mgr, bytes); \
    memset ((p), 0, bytes); \
  } while (0)

#define REALLOCN(p,m,n) \
  do { \
    size_t mbytes = (m) * sizeof (*(p)); \
    size_t nbytes = (n) * sizeof (*(p)); \
    size_t minbytes = (mbytes < nbytes) ? mbytes : nbytes; \
    void * res = private->malloc_callback (private->memory_mgr, nbytes); \
    memcpy (res, (p), minbytes); \
    if (nbytes > mbytes) \
      memset (((char*)res) + mbytes, 0, nbytes - mbytes); \
    private->free_callback (private->memory_mgr, (p), mbytes); \
    (p) = res; \
  } while (0)

#define FIT(p,m,n) \
  do { \
    size_t old_size = (m); \
    size_t new_size = (n); \
    if (old_size < new_size) \
      { \
	REALLOCN (p,old_size,new_size); \
	(m) = new_size; \
      } \
  } while (0)

#define ENLARGE(p,s) \
  do { \
    size_t old_size = (s); \
    size_t new_size = old_size ? 2 * old_size : 1; \
    REALLOCN (p,old_size,new_size); \
    (s) = new_size; \
  } while (0)

#define PUSH(p,t,s,l) \
  do { \
    if ((t) == (s)) \
      ENLARGE (p, s); \
    (p)[(t)++] = (l); \
  } while (0)

#define DELETEN(p,n) \
  do { \
    size_t bytes = (n) * sizeof (*(p)); \
    private->free_callback (private->memory_mgr, (p), bytes); \
    (p) = 0; \
  } while (0)

#define NEW(p) NEWN (p,1)
#define DELETE(p) DELETEN (p,1)

#define IMPORT_private_FROM(p) \
  aiger_private * private = (aiger_private*) (p)

#define EXPORT_public_FROM(p)    \
  aiger * public = &(p)->public

typedef struct aiger_private aiger_private;
typedef struct aiger_buffer aiger_buffer;
typedef struct aiger_reader aiger_reader;

struct aiger_private
{
  aiger public;

  aiger_type *types;		/* [0..maxvar] */
  size_t size_types;

  unsigned char * coi;
  aiger_size size_coi;

  size_t size_inputs;
  size_t size_latches;
  size_t size_outputs;
  size_t size_ands;

  size_t num_comments;
  size_t size_comments;

  void *memory_mgr;
  aiger_malloc malloc_callback;
  aiger_free free_callback;

  char *error;
};

struct aiger_buffer
{
  char *start;
  char *cursor;
  char *end;
};

struct aiger_reader
{
  void *state;
  aiger_get get;

  int ch;

  unsigned lineno;
  unsigned charno;

  unsigned lineno_at_last_token_start;

  int done_with_reading_header;
  int looks_like_aag;

  aiger_mode mode;
  aiger_variable maxvar;
  aiger_size inputs;
  aiger_size latches;
  aiger_size outputs;
  aiger_size ands;

  char *buffer;
  size_t top_buffer;
  size_t size_buffer;
};

aiger *
aiger_init_mem (void *memory_mgr,
		aiger_malloc external_malloc, aiger_free external_free)
{
  aiger_private *private;
  aiger *public;

  assert (external_malloc);
  assert (external_free);
  private = external_malloc (memory_mgr, sizeof (*private));
  memset (private, 0, sizeof (*private));
  private->memory_mgr = memory_mgr;
  private->malloc_callback = external_malloc;
  private->free_callback = external_free;
  public = &private->public;
  PUSH (public->comments, private->num_comments, private->size_comments, 0);

  return public;
}

#ifdef __GNUC__
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif

static void *
aiger_default_malloc (void *state UNUSED, size_t bytes)
{
  return malloc (bytes);
}

static void
aiger_default_free (void *state UNUSED, void *ptr, size_t bytes UNUSED)
{
  free (ptr);
}

aiger *
aiger_init (void)
{
  return aiger_init_mem (0, aiger_default_malloc, aiger_default_free);
}

static void
aiger_delete_str (aiger_private * private, char *str)
{
  if (str)
    DELETEN (str, strlen (str) + 1);
}

static char *
aiger_copy_str (aiger_private * private, const char *str)
{
  char *res;

  if (!str || !str[0])
    return 0;

  NEWN (res, strlen (str) + 1);
  strcpy (res, str);

  return res;
}

static unsigned
aiger_delete_symbols_aux (aiger_private * private,
			  aiger_symbol * symbols, size_t size)
{
  unsigned i, res;

  res = 0;
  for (i = 0; i < size; i++)
    {
      aiger_symbol *s = symbols + i;
      if (!s->name)
	continue;

      aiger_delete_str (private, s->name);
      s->name = 0;
      res++;
    }

  return res;
}

static void
aiger_delete_symbols (aiger_private * private,
		      aiger_symbol * symbols, size_t size)
{
  aiger_delete_symbols_aux (private, symbols, size);
  DELETEN (symbols, size);
}

static size_t
aiger_delete_comments (aiger * public)
{
  char **p = (char **) public->comments, *str;
  IMPORT_private_FROM (public);

  while ((str = *p++))
    aiger_delete_str (private, str);

  private->num_comments = 0;
  public->comments[0] = 0;

  return private->num_comments;
}

void
aiger_reset (aiger * public)
{
  IMPORT_private_FROM (public);

  aiger_delete_symbols (private, public->inputs, private->size_inputs);
  aiger_delete_symbols (private, public->latches, private->size_latches);
  aiger_delete_symbols (private, public->outputs, private->size_outputs);
  DELETEN (public->ands, private->size_ands);

  aiger_delete_comments (public);
  DELETEN (public->comments, private->size_comments);

  DELETEN (private->coi, private->size_coi);

  DELETEN (private->types, private->size_types);
  aiger_delete_str (private, private->error);
  DELETE (private);
}

static aiger_type *
aiger_import_literal (aiger_private * private, aiger_literal lit)
{
  aiger_variable var = aiger_lit2var (lit);
  EXPORT_public_FROM (private);

  if (var > public->maxvar)
    public->maxvar = var;

  while (var >= private->size_types)
    ENLARGE (private->types, private->size_types);

  return private->types + var;
}

void
aiger_add_input (aiger * public, aiger_literal lit, const char *name)
{
  IMPORT_private_FROM (public);
  aiger_symbol symbol;
  aiger_type *type;

  assert (!aiger_error (public));

  assert (lit);
  assert (!aiger_sign (lit));

  type = aiger_import_literal (private, lit);

  assert (!type->input);
  assert (!type->latch);
  assert (!type->the_and);

  type->input = 1;
  type->idx = public->num_inputs;

  symbol.lit = lit;
  symbol.name = aiger_copy_str (private, name);
  symbol.next = 0;
  symbol.nodeCopied = 0;

  PUSH (public->inputs, public->num_inputs, private->size_inputs, symbol);
}

void
aiger_add_latch (aiger * public,
		 aiger_literal lit, aiger_literal next, const char *name)
{
  IMPORT_private_FROM (public);
  aiger_symbol symbol;
  aiger_type *type;

  assert (!aiger_error (public));

  assert (lit);
  assert (!aiger_sign (lit));

  type = aiger_import_literal (private, lit);

  assert (!type->input);
  assert (!type->latch);
  assert (!type->the_and);

  /* Warning: importing 'next' makes 'type' invalid.
   */
  type->latch = 1;
  type->idx = public->num_latches;

  aiger_import_literal (private, next);

  symbol.lit = lit;
  symbol.next = next;
  symbol.name = aiger_copy_str (private, name);
  symbol.nodeCopied = 0;

  PUSH (public->latches, public->num_latches, private->size_latches, symbol);
}

void
aiger_add_output (aiger * public, aiger_literal lit, const char *name)
{
  IMPORT_private_FROM (public);
  aiger_symbol symbol;
  aiger_import_literal (private, lit);
  symbol.lit = lit;
  symbol.name = aiger_copy_str (private, name);
  symbol.next = 0;
  symbol.nodeCopied = 0;
  symbol.clausesForThisNodeExists = 0;
  PUSH (public->outputs, public->num_outputs, private->size_outputs, symbol);
}

void
aiger_add_and (aiger * public, aiger_literal lhs, aiger_literal rhs0, aiger_literal rhs1)
{
  IMPORT_private_FROM (public);
  aiger_type *type;
  aiger_and *and;

  assert (!aiger_error (public));

  assert (lhs > 1);
  assert (!aiger_sign (lhs));

  type = aiger_import_literal (private, lhs);

  assert (!type->input);
  assert (!type->latch);
  if(type->the_and) {
    and = public->ands + type->idx;
    assert(and->lhs == lhs);
    assert(and->rhs0 == rhs0);
    assert(and->rhs1 == rhs1);
    return;
  }

  type->the_and = 1;
  type->idx = public->num_ands;

  aiger_type *rhs0t = aiger_import_literal (private, rhs0);
  aiger_type *rhs1t = aiger_import_literal (private, rhs1);
  rhs0t->fanout++;
  rhs1t->fanout++;

  if (public->num_ands == private->size_ands)
    ENLARGE (public->ands, private->size_ands);

  and = public->ands + public->num_ands;

  and->lhs = lhs;
  and->rhs0 = rhs0;
  and->rhs1 = rhs1;
  and->nodeCopied = 0;
  and->clausesForThisNodeExists = 0;

  public->num_ands++;
}

void
aiger_add_comment (aiger * public, const char *comment)
{
  IMPORT_private_FROM (public);
  char **p;

  assert (!aiger_error (public));

  assert (!strchr (comment, '\n'));
  assert (private->num_comments);
  p = public->comments + private->num_comments - 1;
  assert (!*p);
  *p = aiger_copy_str (private, comment);
  PUSH (public->comments, private->num_comments, private->size_comments, 0);
}

static const char *
aiger_error_s (aiger_private * private, const char *s, const char *a)
{
  size_t tmp_len, error_len;
  char *tmp;
  assert (!private->error);
  tmp_len = strlen (s) + strlen (a) + 1;
  NEWN (tmp, tmp_len);
  sprintf (tmp, s, a);
  error_len = strlen (tmp) + 1;
  NEWN (private->error, error_len);
  memcpy (private->error, tmp, error_len);
  DELETEN (tmp, tmp_len);
  return private->error;
}

static const char *
aiger_error_L (aiger_private * private, const char *s, aiger_literal u)
{
  size_t tmp_len, error_len;
  char *tmp;
  assert (!private->error);
  tmp_len = strlen (s) + sizeof (u) * 4 + 1;
  NEWN (tmp, tmp_len);
  sprintf (tmp, s, u);
  error_len = strlen (tmp) + 1;
  NEWN (private->error, error_len);
  memcpy (private->error, tmp, error_len);
  DELETEN (tmp, tmp_len);
  return private->error;
}

static const char *
aiger_error_u (aiger_private * private, const char *s, unsigned u)
{
  size_t tmp_len, error_len;
  char *tmp;
  assert (!private->error);
  tmp_len = strlen (s) + sizeof (u) * 4 + 1;
  NEWN (tmp, tmp_len);
  sprintf (tmp, s, u);
  error_len = strlen (tmp) + 1;
  NEWN (private->error, error_len);
  memcpy (private->error, tmp, error_len);
  DELETEN (tmp, tmp_len);
  return private->error;
}

static const char *
aiger_error_LL (aiger_private * private, const char *s, aiger_literal a,
		aiger_literal b)
{
  size_t tmp_len, error_len;
  char *tmp;
  assert (!private->error);
  tmp_len = strlen (s) + sizeof (a) * 4 + sizeof (b) * 4 + 1;
  NEWN (tmp, tmp_len);
  sprintf (tmp, s, a, b);
  error_len = strlen (tmp) + 1;
  NEWN (private->error, error_len);
  memcpy (private->error, tmp, error_len);
  DELETEN (tmp, tmp_len);
  return private->error;
}

static const char *
aiger_error_uL (aiger_private * private, const char *s, unsigned a,
		aiger_literal b)
{
  size_t tmp_len, error_len;
  char *tmp;
  assert (!private->error);
  tmp_len = strlen (s) + sizeof (a) * 4 + sizeof (b) * 4 + 1;
  NEWN (tmp, tmp_len);
  sprintf (tmp, s, a, b);
  error_len = strlen (tmp) + 1;
  NEWN (private->error, error_len);
  memcpy (private->error, tmp, error_len);
  DELETEN (tmp, tmp_len);
  return private->error;
}

static const char *
aiger_error_uu (aiger_private * private, const char *s, unsigned a,
		unsigned b)
{
  size_t tmp_len, error_len;
  char *tmp;
  assert (!private->error);
  tmp_len = strlen (s) + sizeof (a) * 4 + sizeof (b) * 4 + 1;
  NEWN (tmp, tmp_len);
  sprintf (tmp, s, a, b);
  error_len = strlen (tmp) + 1;
  NEWN (private->error, error_len);
  memcpy (private->error, tmp, error_len);
  DELETEN (tmp, tmp_len);
  return private->error;
}

static const char *
aiger_error_usL (aiger_private * private,
		 const char *s, unsigned a, const char *t, aiger_literal b)
{
  size_t tmp_len, error_len;
  char *tmp;
  assert (!private->error);
  tmp_len = strlen (s) + strlen (t) + sizeof (a) * 4 + sizeof (b) * 4 + 1;
  NEWN (tmp, tmp_len);
  sprintf (tmp, s, a, t, b);
  error_len = strlen (tmp) + 1;
  NEWN (private->error, error_len);
  memcpy (private->error, tmp, error_len);
  DELETEN (tmp, tmp_len);
  return private->error;
}

static const char *
aiger_error_usu (aiger_private * private,
		 const char *s, unsigned a, const char *t, unsigned b)
{
  size_t tmp_len, error_len;
  char *tmp;
  assert (!private->error);
  tmp_len = strlen (s) + strlen (t) + sizeof (a) * 4 + sizeof (b) * 4 + 1;
  NEWN (tmp, tmp_len);
  sprintf (tmp, s, a, t, b);
  error_len = strlen (tmp) + 1;
  NEWN (private->error, error_len);
  memcpy (private->error, tmp, error_len);
  DELETEN (tmp, tmp_len);
  return private->error;
}

const char *
aiger_error (aiger * public)
{
  //  IMPORT_private_FROM (public);
  aiger_private * private = (aiger_private*) (public);

  return private->error;
}

static int
aiger_literal_defined (aiger_private * private, aiger_literal lit)
{
  aiger_variable var = aiger_lit2var (lit);
#ifndef NDEBUG
  EXPORT_public_FROM (private);
#endif
  aiger_type *type;

  assert (var <= public->maxvar);
  if (!var)
    return 1;

  type = private->types + var;

  return type->the_and || type->input || type->latch;
}

static void
aiger_check_next_defined (aiger_private * private)
{
  //EXPORT_public_FROM (private);
  aiger * public = &(private)->public;


  aiger_literal i, next, latch;
  aiger_symbol *symbol;

  if (private->error)
    return;

  for (i = 0; !private->error && i < public->num_latches; i++)
    {
      symbol = public->latches + i;
      latch = symbol->lit;
      next = symbol->next;

      assert (!aiger_sign (latch));
      assert (private->types[aiger_lit2var (latch)].latch);

      if (!aiger_literal_defined (private, next))
	aiger_error_LL (private,
			"next state function %u of latch %u undefined",
			next, latch);
    }
}

static void
aiger_check_right_hand_side_defined (aiger_private * private, aiger_and * and,
				     aiger_literal rhs)
{
  if (private->error)
    return;

  assert (and);
  assert(aiger_literal_defined (private, rhs));
  if (!aiger_literal_defined (private, rhs))
    aiger_error_LL (private, "literal %u in AND %u undefined", rhs, and->lhs);
}

static void
aiger_check_right_hand_sides_defined (aiger_private * private)
{
  //EXPORT_public_FROM (private);
  aiger * public = &(private)->public;

  aiger_and *and;
  unsigned i;

  if (private->error)
    return;

  for (i = 0; !private->error && i < public->num_ands; i++)
    {
      and = public->ands + i;
      aiger_check_right_hand_side_defined (private, and, and->rhs0);
      aiger_check_right_hand_side_defined (private, and, and->rhs1);
    }
}

static void
aiger_check_outputs_defined (aiger_private * private)
{
  //EXPORT_public_FROM (private);
  aiger * public = &(private)->public;

  aiger_size i;
  aiger_literal output;

  if (private->error)
    return;

  for (i = 0; !private->error && i < public->num_outputs; i++)
    {
      output = public->outputs[i].lit;
      output = aiger_strip (output);
      if (output <= 1)
	continue;

      if (!aiger_literal_defined (private, output))
	aiger_error_L (private, "output %u undefined", output);
    }
}

static void
aiger_check_for_cycles (aiger_private * private)
{
  aiger_variable i, j, *stack;
  size_t size_stack, top_stack;
  aiger_variable tmp;

  //EXPORT_public_FROM (private);
  aiger * public = &(private)->public;

  aiger_type *type;
  aiger_and *and;

  if (private->error)
    return;

  stack = 0;
  size_stack = top_stack = 0;

  for (i = 1; !private->error && i <= public->maxvar; i++)
    {
      type = private->types + i;

      if (!type->the_and || type->mark)
	continue;

      PUSH (stack, top_stack, size_stack, i);
      while (top_stack)
	{
	  j = stack[top_stack - 1];

	  if (j)
	    {
	      type = private->types + j;
	      if (type->mark && type->onstack)
		{
		  aiger_error_L (private,
				 "cyclic definition for and gate %u", j);
		  break;
		}

	      if (!type->the_and || type->mark)
		{
		  top_stack--;
		  continue;
		}

	      /* Prefix code.
	       */
	      type->mark = 1;
	      type->onstack = 1;
	      PUSH (stack, top_stack, size_stack, 0);

	      assert (type->idx < public->num_ands);
	      and = public->ands + type->idx;

	      tmp = aiger_lit2var (and->rhs0);
	      if (tmp)
		PUSH (stack, top_stack, size_stack, tmp);

	      tmp = aiger_lit2var (and->rhs1);
	      if (tmp)
		PUSH (stack, top_stack, size_stack, tmp);
	    }
	  else
	    {
	      /* All descendends traversed.  This is the postfix code.
	       */
	      assert (top_stack >= 2);
	      top_stack -= 2;
	      j = stack[top_stack];
	      assert (j);
	      type = private->types + j;
	      assert (type->mark);
	      assert (type->onstack);
	      type->onstack = 0;
	    }
	}
    }

  DELETEN (stack, size_stack);
}

const char *
aiger_check (aiger * public)
{
  //  IMPORT_private_FROM (public);
  aiger_private * private = (aiger_private*) public;

  assert (!aiger_error (public));

  aiger_check_next_defined (private);
  aiger_check_outputs_defined (private);
  aiger_check_right_hand_sides_defined (private);
  aiger_check_for_cycles (private);

  if(private->error) {
    fprintf(stderr, "%s\n", private->error);
  }

  return private->error;
}

static int
aiger_default_get (FILE * file)
{
  return getc (file);
}

static int
aiger_default_put (char ch, FILE * file)
{
  return putc ((unsigned char) ch, file);
}

static int
aiger_string_put (char ch, aiger_buffer * buffer)
{
  unsigned char res;
  if (buffer->cursor == buffer->end)
    return EOF;
  *buffer->cursor++ = ch;
  res = (unsigned char) ch;
  return ch;
}

static int
aiger_put_s (void *state, aiger_put put, const char *str)
{
  const char *p;
  char ch;

  for (p = str; (ch = *p); p++)
    if (put (ch, state) == EOF)
      return EOF;

  return p - str;		/* 'fputs' semantics, >= 0 is OK */
}

static int
aiger_put_L (void *state, aiger_put put, unsigned u)
{
  char buffer[sizeof (u) * 4];
  sprintf (buffer, "%u", u);
  return aiger_put_s (state, put, buffer);
}

#define aiger_put_S aiger_put_L
#define aiger_put_V aiger_put_L

static int
aiger_put_u (void *state, aiger_put put, unsigned u)
{
  char buffer[sizeof (u) * 4];
  sprintf (buffer, "%u", u);
  return aiger_put_s (state, put, buffer);
}

static int
aiger_write_delta (void *state, aiger_put put, aiger_literal delta)
{
  unsigned char ch;
  aiger_literal tmp = delta;

  while (tmp & (aiger_literal) ~0x7f)
    {
      ch = tmp & 0x7f;
      ch |= 0x80;

      if (put ((char)ch, state) == EOF)
	return 0;

      tmp >>= 7;
    }

  ch = tmp;
  return put ((char)ch, state) != EOF;
}

static int
aiger_write_header (aiger * public,
		    const char *format_string,
		    int compact_inputs_and_latches,
		    void *state, aiger_put put)
{
  unsigned i;

  if (aiger_put_s (state, put, format_string) == EOF ||
      put (' ', state) == EOF ||
      aiger_put_V (state, put, public->maxvar) == EOF ||
      put (' ', state) == EOF ||
      aiger_put_S (state, put, public->num_inputs) == EOF ||
      put (' ', state) == EOF ||
      aiger_put_S (state, put, public->num_latches) == EOF ||
      put (' ', state) == EOF ||
      aiger_put_S (state, put, public->num_outputs) == EOF ||
      put (' ', state) == EOF ||
      aiger_put_S (state, put, public->num_ands) == EOF ||
      put ('\n', state) == EOF)
    return 0;

  if (!compact_inputs_and_latches && public->num_inputs)
    {
      for (i = 0; i < public->num_inputs; i++)
	if (aiger_put_L (state, put, public->inputs[i].lit) == EOF ||
	    put ('\n', state) == EOF)
	  return 0;
    }

  if (public->num_latches)
    {
      for (i = 0; i < public->num_latches; i++)
	{
	  if (!compact_inputs_and_latches)
	    {
	      if (aiger_put_L (state, put, public->latches[i].lit) == EOF ||
		  put (' ', state) == EOF)
		return 0;
	    }

	  if (aiger_put_L (state, put, public->latches[i].next) == EOF ||
	      put ('\n', state) == EOF)
	    return 0;
	}
    }

  if (public->num_outputs)
    {
      for (i = 0; i < public->num_outputs; i++)
	if (aiger_put_L (state, put, public->outputs[i].lit) == EOF ||
	    put ('\n', state) == EOF)
	  return 0;
    }

  return 1;
}

static int
aiger_have_at_least_one_symbol_aux (aiger * public UNUSED,
				    aiger_symbol * symbols, aiger_size size)
{
  unsigned i;

  for (i = 0; i < size; i++)
    if (symbols[i].name)
      return 1;

  return 0;
}

static int
aiger_have_at_least_one_symbol (aiger * public)
{
  if (aiger_have_at_least_one_symbol_aux (public,
					  public->inputs, public->num_inputs))
    return 1;

  if (aiger_have_at_least_one_symbol_aux (public,
					  public->outputs,
					  public->num_outputs))
    return 1;

  if (aiger_have_at_least_one_symbol_aux (public,
					  public->latches,
					  public->num_latches))
    return 1;

  return 0;
}

static int
aiger_write_symbols_aux (aiger * public UNUSED,
			 void *state, aiger_put put,
			 const char *type,
			 aiger_symbol * symbols, aiger_size size)
{
  unsigned i;

  for (i = 0; i < size; i++)
    {
      if (!symbols[i].name)
	continue;

      assert (symbols[i].name[0]);

      if (aiger_put_s (state, put, type) == EOF ||
	  aiger_put_u (state, put, i) == EOF ||
	  put (' ', state) == EOF ||
	  aiger_put_s (state, put, symbols[i].name) == EOF ||
	  put ('\n', state) == EOF)
	return 0;
    }

  return 1;
}

static int
aiger_write_symbols (aiger * public, void *state, aiger_put put)
{
  if (!aiger_write_symbols_aux (public, state, put,
				"i", public->inputs, public->num_inputs))
    return 0;

  if (!aiger_write_symbols_aux (public, state, put,
				"l", public->latches, public->num_latches))
    return 0;

  if (!aiger_write_symbols_aux (public, state, put,
				"o", public->outputs, public->num_outputs))
    return 0;

  return 1;
}

int
aiger_write_symbols_to_file (aiger * public, FILE * file)
{
  assert (!aiger_error (public));
  return aiger_write_symbols (public, file, (aiger_put) aiger_default_put);
}

static int
aiger_write_comments (aiger * public, void *state, aiger_put put)
{
  char **p, *str;

  for (p = public->comments; (str = *p); p++)
    {
      if (aiger_put_s (state, put, str) == EOF)
	return 0;

      if (put ('\n', state) == EOF)
	return 0;
    }

  return 1;
}

int
aiger_write_comments_to_file (aiger * public, FILE * file)
{
  assert (!aiger_error (public));
  return aiger_write_comments (public, file, (aiger_put) aiger_default_put);
}

static int
aiger_write_ascii (aiger * public, void *state, aiger_put put)
{
  aiger_and *and;
  unsigned i;

  assert (!aiger_check (public));

  if (!aiger_write_header (public, "aag", 0, state, put))
    return 0;

  for (i = 0; i < public->num_ands; i++)
    {
      and = public->ands + i;
      if (aiger_put_L (state, put, and->lhs) == EOF ||
	  put (' ', state) == EOF ||
	  aiger_put_L (state, put, and->rhs0) == EOF ||
	  put (' ', state) == EOF ||
	  aiger_put_L (state, put, and->rhs1) == EOF ||
	  put ('\n', state) == EOF)
	return 0;
    }

  return 1;
}

static aiger_literal
aiger_max_input_or_latch (aiger * public)
{
  aiger_size i;
  aiger_literal res, tmp;

  res = 0;

  for (i = 0; i < public->num_inputs; i++)
    {
      tmp = public->inputs[i].lit;
      assert (!aiger_sign (tmp));
      if (tmp > res)
	res = tmp;
    }

  for (i = 0; i < public->num_latches; i++)
    {
      tmp = public->latches[i].lit;
      assert (!aiger_sign (tmp));
      if (tmp > res)
	res = tmp;
    }

  return res;
}

int
aiger_is_reencoded (aiger * public)
{
  aiger_size i;
  aiger_literal tmp, lhs, max;
  aiger_and *and;

  max = 0;
  for (i = 0; i < public->num_inputs; i++)
    {
      max += 2;
      tmp = public->inputs[i].lit;
      if (max != tmp)
	return 0;
    }

  for (i = 0; i < public->num_latches; i++)
    {
      max += 2;
      tmp = public->latches[i].lit;
      if (max != tmp)
	return 0;
    }

  lhs = aiger_max_input_or_latch (public) + 2;
  for (i = 0; i < public->num_ands; i++)
    {
      and = public->ands + i;

      if (and->lhs <= max)
	return 0;

      if (and->lhs != lhs)
	return 0;

      if (and->lhs < and->rhs0)
	return 0;

      if (and->rhs0 < and->rhs1)
	return 0;

      lhs += 2;
    }

  return 1;
}

static void
aiger_new_code (aiger_variable var, aiger_variable *new, aiger_variable *code)
{
  aiger_literal lit = aiger_var2lit (var);
  aiger_variable res;
  assert (!code[lit]);
  res = *new;
  code[lit] = res;
  code[lit + 1] = res + 1;
  *new += 2;
}

static aiger_literal
aiger_reencode_lit (aiger * public, aiger_literal lit,
		    aiger_literal *new, aiger_variable *code,
		    aiger_variable *stack, aiger_size size_stack)
{
  aiger_literal child0, child1, tmp;
  aiger_variable res, *top, old, var;
  IMPORT_private_FROM (public);
  aiger_type *type;
  aiger_and *and;

  if (lit < 2)
    return lit;

  res = code[lit];
  if (res)
    return res;

  var = aiger_lit2var (lit);
  assert (var <= public->maxvar);
  type = private->types + var;

  if (type->the_and)
    {
      top = stack;
      *top++ = var;

      while (top > stack)
	{
	  old = *--top;
	  if (old)
	    {
	      if (code[aiger_var2lit (old)])
		continue;

	      assert (old <= public->maxvar);
	      type = private->types + old;
	      if (type->onstack)
		continue;

	      type->onstack = 1;

	      *top++ = old;
	      *top++ = 0;

	      assert (type->the_and);
	      assert (type->idx < public->num_ands);

	      and = public->ands + type->idx;
	      assert (and);

	      child0 = aiger_lit2var (and->rhs0);
	      child1 = aiger_lit2var (and->rhs1);

	      if (child0 < child1)
		{
		  tmp = child0;
		  child0 = child1;
		  child1 = tmp;
		}

	      assert (child0 >= child1);	/* smaller child first */

	      if (child0)
		{
		  type = private->types + child0;
		  if (!type->input && !type->latch && !type->onstack)
		    {
		      assert (top < stack + size_stack);
		      *top++ = child0;
		    }
		}

	      if (child1)
		{
		  type = private->types + child1;
		  if (!type->input && !type->latch && !type->onstack)
		    {
		      assert (top < stack + size_stack);
		      *top++ = child1;
		    }
		}
	    }
	  else
	    {
	      assert (top > stack);
	      old = *--top;
	      assert (!code[aiger_var2lit (old)]);
	      type = private->types + old;
	      assert (type->onstack);
	      type->onstack = 0;
	      aiger_new_code (old, new, code);
	    }
	}
    }
  else
    {
      assert (type->input || type->latch);
      assert (lit < *new);

      code[lit] = lit;
      code[aiger_not (lit)] = aiger_not (lit);
    }

  assert (code[lit]);

  return code[lit];
}

static int
cmp_lhs (const void *a, const void *b)
{
  const aiger_and *c = a;
  const aiger_and *d = b;
  return ((int) c->lhs) - (int) d->lhs;
}

void
aiger_reencode (aiger * public)
{
  unsigned i, j;
  aiger_variable *stack;
  aiger_size size_stack, size_code;
  aiger_literal *code, old, new, lhs, rhs0, rhs1, tmp;
  IMPORT_private_FROM (public);

  assert (!aiger_error (public));

  aiger_symbol *symbol;
  aiger_type *type;
  aiger_and *and;

  if (aiger_is_reencoded (public))
    return;

  size_code = 2 * (public->maxvar + 1);
  if (size_code < 2)
    size_code = 2;

  NEWN (code, size_code);

  size_stack = 2 * public->num_ands;
  NEWN (stack, size_stack);

  code[1] = 1;			/* not used actually */

  new = 2;

  for (i = 0; i < public->num_inputs; i++)
    {
      old = public->inputs[i].lit;

      code[old] = new;
      code[old + 1] = new + 1;

      new += 2;
    }

  for (i = 0; i < public->num_latches; i++)
    {
      old = public->latches[i].lit;

      code[old] = new;
      code[old + 1] = new + 1;

      new += 2;
    }

  for (i = 0; i < public->num_latches; i++)
    {
      old = public->latches[i].next;
      public->latches[i].next =
	aiger_reencode_lit (public, old, &new, code, stack, size_stack);
    }

  for (i = 0; i < public->num_outputs; i++)
    {
      old = public->outputs[i].lit;
      public->outputs[i].lit =
	aiger_reencode_lit (public, old, &new, code, stack, size_stack);
    }

  DELETEN (stack, 2 * public->num_ands);

  j = 0;
  for (i = 0; i < public->num_ands; i++)
    {
      and = public->ands + i;
      lhs = code[and->lhs];
      if (!lhs)
	continue;

      rhs0 = code[and->rhs0];
      rhs1 = code[and->rhs1];

      and = public->ands + j++;

      if (rhs0 < rhs1)
	{
	  tmp = rhs1;
	  rhs1 = rhs0;
	  rhs0 = tmp;
	}

      assert (lhs > rhs0);
      assert (rhs0 >= rhs1);

      and->lhs = lhs;
      and->rhs0 = rhs0;
      and->rhs1 = rhs1;
    }
  public->num_ands = j;

  qsort (public->ands, j, sizeof (*and), cmp_lhs);

  assert (new);
  assert (public->maxvar >= aiger_lit2var (new - 1));
  public->maxvar = aiger_lit2var (new - 1);

  /* Reset types.
   */
  for (i = 1; i <= public->maxvar; i++)
    {
      type = private->types + i;
      type->input = 0;
      type->latch = 0;
      type->the_and = 0;
      type->idx = 0;
    }

  /* Fix types for ANDs.
   */
  for (i = 0; i < public->num_ands; i++)
    {
      and = public->ands + i;
      type = private->types + aiger_lit2var (and->lhs);
      type->the_and = 1;
      type->idx = i;
    }

  /* Fix types for inputs.
   */
  for (i = 0; i < public->num_inputs; i++)
    {
      symbol = public->inputs + i;
      assert (symbol->lit < size_code);
      symbol->lit = code[symbol->lit];
      type = private->types + aiger_lit2var (symbol->lit);
      type->input = 1;
      type->idx = i;
    }

  /* Fix types for latches.
   */
  for (i = 0; i < public->num_latches; i++)
    {
      symbol = public->latches + i;
      symbol->lit = code[symbol->lit];
      type = private->types + aiger_lit2var (symbol->lit);
      type->latch = 1;
      type->idx = i;
    }

  DELETEN (code, size_code);

#ifndef NDEBUG
  for (i = 0; i <= public->maxvar; i++)
    {
      type = private->types + i;
      assert (!(type->input && type->latch));
      assert (!(type->input && type->the_and));
      assert (!(type->latch && type->the_and));
    }
#endif
  assert (aiger_is_reencoded (public));
  assert (!aiger_check (public));
}

const unsigned char *
aiger_coi (aiger * public)
{
  IMPORT_private_FROM (public);
  private->size_coi = public->maxvar + 1;
  NEWN (private->coi, private->size_coi);
  memset (private->coi, 1, private->size_coi);
  return private->coi;
}

static int
aiger_write_binary (aiger * public, void *state, aiger_put put)
{
  aiger_and *and;
  aiger_literal lhs;
  aiger_size i;

  assert (!aiger_check (public));

  aiger_reencode (public);

  if (!aiger_write_header (public, "aig", 1, state, put))
    return 0;

  lhs = aiger_max_input_or_latch (public) + 2;

  for (i = 0; i < public->num_ands; i++)
    {
      and = public->ands + i;

      assert (lhs == and->lhs);
      assert (lhs > and->rhs0);
      assert (and->rhs0 >= and->rhs1);

      if (!aiger_write_delta (state, put, lhs - and->rhs0))
	return 0;

      if (!aiger_write_delta (state, put, and->rhs0 - and->rhs1))
	return 0;

      lhs += 2;
    }

  return 1;
}

aiger_size
aiger_strip_symbols_and_comments (aiger * public)
{
  IMPORT_private_FROM (public);
  size_t res;

  assert (!aiger_error (public));

  res = aiger_delete_comments (public);

  res += aiger_delete_symbols_aux (private,
				   public->inputs, private->size_inputs);

  res += aiger_delete_symbols_aux (private,
				   public->latches, private->size_latches);

  res += aiger_delete_symbols_aux (private,
				   public->outputs, private->size_outputs);
  return res;
}

int
aiger_write_generic (aiger * public,
		     aiger_mode mode, void *state, aiger_put put)
{
  assert (!aiger_error (public));

  if ((mode & aiger_ascii_mode))
    {
      if (!aiger_write_ascii (public, state, put))
	return 0;
    }
  else
    {
      if (!aiger_write_binary (public, state, put))
	return 0;
    }

  if (!(mode & aiger_stripped_mode))
    {
      if (aiger_have_at_least_one_symbol (public))
	{
	  if (!aiger_write_symbols (public, state, put))
	    return 0;
	}

      if (public->comments[0])
	{
	  if (aiger_put_s (state, put, "c\n") == EOF)
	    return 0;

	  if (!aiger_write_comments (public, state, put))
	    return 0;
	}
    }

  return 1;
}

int
aiger_write_to_file (aiger * public, aiger_mode mode, FILE * file)
{
  assert (!aiger_error (public));
  return aiger_write_generic (public,
			      mode, file, (aiger_put) aiger_default_put);
}

int
aiger_write_to_string (aiger * public, aiger_mode mode, char *str, size_t len)
{
  aiger_buffer buffer;
  int res;

  assert (!aiger_error (public));

  buffer.start = str;
  buffer.cursor = str;
  buffer.end = str + len;
  res = aiger_write_generic (public,
			     mode, &buffer, (aiger_put) aiger_string_put);

  if (!res)
    return 0;

  if (aiger_string_put (0, &buffer) == EOF)
    return 0;

  return 1;
}

static int
aiger_has_suffix (const char *str, const char *suffix)
{
  if (strlen (str) < strlen (suffix))
    return 0;

  return !strcmp (str + strlen (str) - strlen (suffix), suffix);
}

int
aiger_open_and_write_to_file (aiger * public, const char *file_name)
{
  IMPORT_private_FROM (public);
  int res, pclose_file;
  char *cmd;
  size_t size_cmd;
  aiger_mode mode;
  FILE *file;

  assert (!aiger_error (public));

  assert (file_name);

  if (aiger_has_suffix (file_name, ".gz"))
    {
      size_cmd = strlen (file_name) + strlen (GZIP);
      NEWN (cmd, size_cmd);
      sprintf (cmd, GZIP, file_name);
      file = popen (cmd, "w");
      DELETEN (cmd, size_cmd);
      pclose_file = 1;
    }
  else
    {
      file = fopen (file_name, "w");
      pclose_file = 0;
    }

  if (!file)
    return 0;

  if (aiger_has_suffix (file_name, ".aag") ||
      aiger_has_suffix (file_name, ".aag.gz"))
    mode = aiger_ascii_mode;
  else
    mode = aiger_binary_mode;

  res = aiger_write_to_file (public, mode, file);

  if (pclose_file)
    pclose (file);
  else
    fclose (file);

  if (!res)
    unlink (file_name);

  return res;
}

static int
aiger_next_ch (aiger_reader * reader)
{
  int res;

  res = reader->get (reader->state);

  if (isspace (reader->ch) && !isspace (res))
    reader->lineno_at_last_token_start = reader->lineno;

  reader->ch = res;

  if (reader->done_with_reading_header && reader->looks_like_aag)
    {
      if (!isspace (res) && !isdigit (res) && res != EOF)
	reader->looks_like_aag = 0;
    }

  if (res == '\n')
    reader->lineno++;

  if (res != EOF)
    reader->charno++;

  return res;
}

/* Read a number assuming that the current character has already been
 * checked to be a digit, e.g. the start of the number to be read.
 */
static unsigned
aiger_read_number (aiger_reader * reader)
{
  unsigned res;

  assert (isdigit (reader->ch));
  res = (unsigned) (reader->ch - '0');

  while (isdigit (aiger_next_ch (reader)))
    res = 10 * res + ((unsigned) (reader->ch - '0'));

  return res;
}

/* Expect and read an unsigned number followed by at least one white space
 * character.  The white space should either the space character or a new
 * line as specified by the 'followed_by' parameter.  If a number can not be
 * found or there is no white space after the number, an apropriate error
 * message is returned.
 */
static const char *
aiger_read_literal (aiger_private * private,
		    aiger_reader * reader,
		    aiger_literal *res_ptr, char followed_by)
{
  unsigned res;

  assert (followed_by == ' ' || followed_by == '\n');

  if (!isdigit (reader->ch))
    return aiger_error_u (private,
			  "line %u: expected literal", reader->lineno);

  res = aiger_read_number (reader);

  if (followed_by == ' ')
    {
      if (reader->ch != ' ')
	return aiger_error_uu (private,
			       "line %u: expected space after literal %u",
			       reader->lineno_at_last_token_start, res);
    }
  else
    {
      if (reader->ch != '\n')
	return aiger_error_uu (private,
			       "line %u: expected new line after literal %u",
			       reader->lineno_at_last_token_start, res);
    }

  aiger_next_ch (reader);	/* skip white space */

  *res_ptr = res;

  return 0;
}

static const char *
aiger_already_defined (aiger * public, aiger_reader * reader, aiger_literal lit)
{
  IMPORT_private_FROM (public);
  aiger_type *type;
  aiger_variable var;

  assert (lit);
  assert (!aiger_sign (lit));

  var = aiger_lit2var (lit);

  if (public->maxvar < var)
    return 0;

  type = private->types + var;

  if (type->input)
    return aiger_error_uL (private,
			   "line %u: literal %u already defined as input",
			   reader->lineno_at_last_token_start, lit);

  if (type->latch)
    return aiger_error_uL (private,
			   "line %u: literal %u already defined as latch",
			   reader->lineno_at_last_token_start, lit);

  if (type->the_and)
    return aiger_error_uL (private,
			   "line %u: literal %u already defined as AND",
			   reader->lineno_at_last_token_start, lit);
  return 0;
}

static const char *
aiger_read_header (aiger * public, aiger_reader * reader)
{
  IMPORT_private_FROM (public);
  aiger_literal lit, next;
  aiger_size i;
  const char *error;

  aiger_next_ch (reader);
  if (reader->ch != 'a')
    return aiger_error_u (private,
			  "line %u: expected 'a' as first character",
			  reader->lineno);

  if (aiger_next_ch (reader) != 'i' && reader->ch != 'a')
    return aiger_error_u (private,
			  "line %u: expected 'i' or 'a' after 'a'",
			  reader->lineno);

  if (reader->ch == 'a')
    reader->mode = aiger_ascii_mode;
  else
    reader->mode = aiger_binary_mode;

  if (aiger_next_ch (reader) != 'g')
    return aiger_error_u (private,
			  "line %u: expected 'g' after 'a[ai]'",
			  reader->lineno);

  if (aiger_next_ch (reader) != ' ')
    return aiger_error_u (private,
			  "line %u: expected ' ' after 'a[ai]g'",
			  reader->lineno);

  aiger_next_ch (reader);

  if (aiger_read_literal (private, reader, &reader->maxvar, ' ') ||
      aiger_read_literal (private, reader, &reader->inputs, ' ') ||
      aiger_read_literal (private, reader, &reader->latches, ' ') ||
      aiger_read_literal (private, reader, &reader->outputs, ' ') ||
      aiger_read_literal (private, reader, &reader->ands, '\n'))
    {
      assert (private->error);
      return private->error;
    }

  if (reader->mode == aiger_binary_mode)
    {
      i = reader->inputs;
      i += reader->latches;
      i += reader->ands;

      if (i != reader->maxvar)
	return aiger_error_u (private,
			      "line %u: invalid maximal variable index",
			      reader->lineno);
    }

  public->maxvar = reader->maxvar;

  FIT (private->types, private->size_types, public->maxvar + 1);
  FIT (public->inputs, private->size_inputs, reader->inputs);
  FIT (public->latches, private->size_latches, reader->latches);
  FIT (public->outputs, private->size_outputs, reader->outputs);
  FIT (public->ands, private->size_ands, reader->ands);

  for (i = 0; i < reader->inputs; i++)
    {
      if (reader->mode == aiger_ascii_mode)
	{
	  error = aiger_read_literal (private, reader, &lit, '\n');
	  if (error)
	    return error;

	  if (!lit || aiger_sign (lit)
	      || aiger_lit2var (lit) > public->maxvar)
	    return aiger_error_uL (private,
				   "line %u: literal %u is not a valid input",
				   reader->lineno_at_last_token_start, lit);

	  error = aiger_already_defined (public, reader, lit);
	  if (error)
	    return error;
	}
      else
	lit = 2 * (i + 1);

      aiger_add_input (public, lit, 0);
    }

  for (i = 0; i < reader->latches; i++)
    {
      if (reader->mode == aiger_ascii_mode)
	{
	  error = aiger_read_literal (private, reader, &lit, ' ');
	  if (error)
	    return error;

	  if (!lit || aiger_sign (lit)
	      || aiger_lit2var (lit) > public->maxvar)
	    return aiger_error_uL (private,
				   "line %u: literal %u is not a valid latch",
				   reader->lineno_at_last_token_start, lit);

	  error = aiger_already_defined (public, reader, lit);
	  if (error)
	    return error;
	}
      else
	lit = 2 * (i + reader->inputs + 1);

      error = aiger_read_literal (private, reader, &next, '\n');
      if (error)
	return error;

      if (aiger_lit2var (next) > public->maxvar)
	return aiger_error_uL (private,
			       "line %u: literal %u is not a valid literal",
			       reader->lineno_at_last_token_start, next);

      aiger_add_latch (public, lit, next, 0);
    }

  for (i = 0; i < reader->outputs; i++)
    {
      error = aiger_read_literal (private, reader, &lit, '\n');
      if (error)
	return error;

      if (aiger_lit2var (lit) > public->maxvar)
	return aiger_error_uL (private,
			       "line %u: literal %u is not a valid output",
			       reader->lineno_at_last_token_start, lit);

      aiger_add_output (public, lit, 0);
    }

  reader->done_with_reading_header = 1;
  reader->looks_like_aag = 1;

  return 0;
}

static const char *
aiger_read_ascii (aiger * public, aiger_reader * reader)
{
  IMPORT_private_FROM (public);
  aiger_size i;
  aiger_literal lhs, rhs0, rhs1;
  const char *error;

  for (i = 0; i < reader->ands; i++)
    {
      error = aiger_read_literal (private, reader, &lhs, ' ');
      if (error)
	return error;

      if (!lhs || aiger_sign (lhs) || aiger_lit2var (lhs) > public->maxvar)
	return aiger_error_uL (private,
			       "line %u: "
			       "literal %u is not a valid LHS of AND",
			       reader->lineno_at_last_token_start, lhs);

      error = aiger_already_defined (public, reader, lhs);
      if (error)
	return error;

      error = aiger_read_literal (private, reader, &rhs0, ' ');
      if (error)
	return error;

      if (aiger_lit2var (rhs0) > public->maxvar)
	return aiger_error_uL (private,
			       "line %u: literal %u is not a valid literal",
			       reader->lineno_at_last_token_start, rhs0);

      error = aiger_read_literal (private, reader, &rhs1, '\n');
      if (error)
	return error;

      if (aiger_lit2var (rhs1) > public->maxvar)
	return aiger_error_uL (private,
			       "line %u: literal %u is not a valid literal",
			       reader->lineno_at_last_token_start, rhs1);

      aiger_add_and (public, lhs, rhs0, rhs1);
    }

  return 0;
}

static const char *
aiger_read_delta (aiger_private * private, aiger_reader * reader,
		  unsigned *res_ptr)
{
  unsigned res, i, charno;
  unsigned char ch;

  if (reader->ch == EOF)
  UNEXPECTED_EOF:
    return aiger_error_u (private,
			  "character %u: unexpected end of file",
			  reader->charno);
  i = 0;
  res = 0;
  ch = reader->ch;

  charno = reader->charno;

  while ((ch & 0x80))
    {
      assert (sizeof (unsigned) == 4);

      if (i == 5)
      INVALID_CODE:
	return aiger_error_u (private, "character %u: invalid code", charno);

      res |= (unsigned) ((ch & 0x7f) << (7 * i++));
      aiger_next_ch (reader);
      if (reader->ch == EOF)
	goto UNEXPECTED_EOF;

      ch = reader->ch;
    }

  if (i == 5 && ch >= 8)
    goto INVALID_CODE;

  res |= (unsigned) (ch << (7 * i));
  *res_ptr = res;

  aiger_next_ch (reader);

  return 0;
}

static const char *
aiger_read_binary (aiger * public, aiger_reader * reader)
{
  unsigned i, delta = 0, charno;
  aiger_literal lhs, rhs0, rhs1;
  IMPORT_private_FROM (public);
  const char *error;

  lhs = aiger_max_input_or_latch (public);

  for (i = 0; i < reader->ands; i++)
    {
      lhs += 2;
      charno = reader->charno;
      error = aiger_read_delta (private, reader, &delta);
      if (error)
	return error;

      if (delta > lhs)		/* can at most be the same */
      INVALID_DELTA:
	return aiger_error_u (private, "character %u: invalid delta", charno);

      rhs0 = lhs - delta;

      charno = reader->charno;
      error = aiger_read_delta (private, reader, &delta);
      if (error)
	return error;

      if (delta > rhs0)		/* can well be the same ! */
	goto INVALID_DELTA;

      rhs1 = rhs0 - delta;

      aiger_add_and (public, lhs, rhs0, rhs1);
    }

  return 0;
}

static void
aiger_reader_push_ch (aiger_private * private, aiger_reader * reader, char ch)
{
  PUSH (reader->buffer, reader->top_buffer, reader->size_buffer, ch);
}

static const char *
aiger_read_symbols (aiger * public, aiger_reader * reader)
{
  IMPORT_private_FROM (public);
  const char *error, *type;
  aiger_size pos, num, count;
  aiger_symbol *symbol;

  assert (!reader->buffer);

  for (count = 0;; count++)
    {
      if (reader->ch == EOF || reader->ch == 'c')
	return 0;

      if (reader->ch != 'i' && reader->ch != 'l' && reader->ch != 'o')
	{
	  if (count)
	    aiger_error_u (private,
			   "line %u: expected 'c', 'i', 'l', 'o' or EOF",
			   reader->lineno);

	  if (reader->looks_like_aag)
	    return aiger_error_u (private,
				  "line %u: corrupted symbol table "
				  "('aig' instead of 'aag' header?)",
				  reader->lineno);
	}

      if (reader->ch == 'i')
	{
	  type = "input";
	  num = public->num_inputs;
	  symbol = public->inputs;
	}
      else if (reader->ch == 'l')
	{
	  type = "latch";
	  num = public->num_latches;
	  symbol = public->latches;
	}
      else
	{
	  assert (reader->ch == 'o');
	  type = "output";
	  num = public->num_outputs;
	  symbol = public->outputs;
	}

      aiger_next_ch (reader);
      error = aiger_read_literal (private, reader, &pos, ' ');
      if (error)
	return error;

      if (pos >= num)
	return aiger_error_usL (private,
				"line %u: "
				"%s symbol table entry position %u too large",
				reader->lineno_at_last_token_start, type,
				pos);

      symbol += pos;

      if (symbol->name)
	return aiger_error_usL (private,
				"line %u: %s %u has multiple symbols",
				reader->lineno_at_last_token_start, type,
				symbol->lit);

      while (reader->ch != '\n' && reader->ch != EOF)
	{
	  aiger_reader_push_ch (private, reader, reader->ch);
	  aiger_next_ch (reader);
	}

      if (reader->ch == EOF)
	return aiger_error_u (private,
			      "line %u: new line missing", reader->lineno);

      assert (reader->ch == '\n');
      aiger_next_ch (reader);

      aiger_reader_push_ch (private, reader, 0);
      symbol->name = aiger_copy_str (private, reader->buffer);
      reader->top_buffer = 0;
    }
}

static const char *
aiger_read_comments (aiger * public, aiger_reader * reader)
{
  IMPORT_private_FROM (public);

  if (reader->ch == EOF)
    return 0;

  if (reader->ch != 'c')
    return aiger_error_u (private,
			  "line %u: expected 'c' or end of file",
			  reader->lineno);

  if (aiger_next_ch (reader) != '\n')
    return aiger_error_u (private,
			  "line %u: expected new line after 'c'",
			  reader->lineno);

  aiger_next_ch (reader);

  while (reader->ch != EOF)
    {
      while (reader->ch != '\n')
	{
	  aiger_reader_push_ch (private, reader, reader->ch);
	  aiger_next_ch (reader);

	  if (reader->ch == EOF)
	    return aiger_error_u (private,
				  "line %u: new line after comment missing",
				  reader->lineno);
	}

      aiger_next_ch (reader);
      aiger_reader_push_ch (private, reader, 0);
      aiger_add_comment (public, reader->buffer);
      reader->top_buffer = 0;
    }

  return 0;
}

const char *
aiger_read_generic (aiger * public, void *state, aiger_get get)
{
  IMPORT_private_FROM (public);
  aiger_reader reader;
  const char *error;

  assert (!aiger_error (public));

  memset (&reader, 0, sizeof (reader));

  reader.lineno = 1;
  reader.state = state;
  reader.get = get;
  reader.ch = ' ';

  error = aiger_read_header (public, &reader);
  if (error)
    return error;

  if (reader.mode == aiger_ascii_mode)
    error = aiger_read_ascii (public, &reader);
  else
    error = aiger_read_binary (public, &reader);

  if (error)
    return error;

  error = aiger_read_symbols (public, &reader);
  if (!error)
    error = aiger_read_comments (public, &reader);

  DELETEN (reader.buffer, reader.size_buffer);

  if (error)
    return error;

  return aiger_check (public);
}

const char *
aiger_read_from_file (aiger * public, FILE * file)
{
  assert (!aiger_error (public));
  return aiger_read_generic (public, file, (aiger_get) aiger_default_get);
}

const char *
aiger_open_and_read_from_file (aiger * public, const char *file_name)
{
  IMPORT_private_FROM (public);
  char *cmd;
  size_t size_cmd;
  const char *res;
  int pclose_file;
  FILE *file;

  assert (!aiger_error (public));

  if (aiger_has_suffix (file_name, ".gz"))
    {
      size_cmd = strlen (file_name) + strlen (GUNZIP);
      NEWN (cmd, size_cmd);
      sprintf (cmd, GUNZIP, file_name);
      file = popen (cmd, "r");
      DELETEN (cmd, size_cmd);
      pclose_file = 1;
    }
  else
    {
      file = fopen (file_name, "r");
      pclose_file = 0;
    }

  if (!file)
    return aiger_error_s (private, "can not read '%s'", file_name);

  res = aiger_read_from_file (public, file);

  if (pclose_file)
    pclose (file);
  else
    fclose (file);

  return res;
}

const char *
aiger_get_symbol (aiger * public, aiger_literal lit)
{
  IMPORT_private_FROM (public);
  aiger_symbol *symbol;
  aiger_type *type;
  aiger_variable var;

  assert (!aiger_error (public));

  assert (lit);
  assert (!aiger_sign (lit));

  var = aiger_lit2var (lit);
  assert (var <= public->maxvar);
  type = private->types + var;

  if (type->input)
    symbol = public->inputs;
  else if (type->latch)
    symbol = public->latches;
  else
    return 0;

  return symbol[type->idx].name;
}

aiger_type *
aiger_lit2type (aiger * public, aiger_literal lit)
{
  IMPORT_private_FROM (public);
  aiger_type *type;
  aiger_variable var;

  assert (!aiger_sign (lit));
  var = aiger_lit2var (lit);
  assert (var <= public->maxvar);
  type = private->types + var;

  return type;
}

aiger_size
aiger_get_fanout (aiger * public, aiger_literal lit)
{
  return aiger_lit2type(public, lit)->fanout;
}

aiger_symbol *
aiger_is_input (aiger * public, aiger_literal lit)
{
  aiger_type *type;
  aiger_symbol *res;

  assert (!aiger_error (public));
  type = aiger_lit2type (public, lit);
  if (!type->input)
    return 0;

  res = public->inputs + type->idx;

  return res;
}

aiger_symbol *
aiger_is_latch (aiger * public, aiger_literal lit)
{
  aiger_symbol *res;
  aiger_type *type;

  assert (!aiger_error (public));
  type = aiger_lit2type (public, lit);
  if (!type->latch)
    return 0;

  res = public->latches + type->idx;

  return res;
}

aiger_and *
aiger_is_and (aiger * public, aiger_literal lit)
{
  aiger_type *type;
  aiger_and *res;

  assert (!aiger_error (public));
  type = aiger_lit2type (public, lit);
  if (!type->the_and)
    return 0;

  res = public->ands + type->idx;

  return res;
}
