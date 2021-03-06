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

/*------------------------------------------------------------------------*/
/* This file contains the API of the 'AIGER' library, which is a reader and
 * writer for the AIGER AIG format.  The code of the library 
 * consists of 'aiger.c' and 'aiger.h'.  It is independent of 'simpaig.c'
 * and 'simpaig.h'.
 * library.
 */
#ifndef aiger_h_INCLUDED
#define aiger_h_INCLUDED

#include <stdio.h>
#include <stdint.h>

/*------------------------------------------------------------------------*/

#define AIGER_VERSION "20071012"

/*------------------------------------------------------------------------*/

typedef unsigned aiger_literal;
typedef unsigned aiger_variable;
typedef unsigned aiger_size;

unsigned hash_aiger_literal(void *k);
int aiger_literal_equal(void *k1, void *k2);

typedef struct aiger aiger;
typedef struct aiger_and aiger_and;
typedef struct aiger_symbol aiger_symbol;

/*------------------------------------------------------------------------*/
/* AIG references are represented as unsigned integers and are called
 * literals.  The least significant bit is the sign.  The index of a literal
 * can be obtained by dividing the literal by two.  Only positive indices
 * are allowed, which leaves 0 for the boolean constant FALSE.  The boolean
 * constant TRUE is encoded as the unsigned number 1 accordingly.
 */
#define aiger_false 0
#define aiger_true 1

#define aiger_sign(l) \
  (((aiger_literal)(l))&1)

#define aiger_strip(l) \
  (((aiger_literal)(l))&((aiger_literal)~1))

#define aiger_not(l) \
  (((aiger_literal)(l))^1)

/*------------------------------------------------------------------------*/
/* Each literal is associated to a variable having an unsigned index.  The
 * variable index is obtained by deviding the literal index by two, which is
 * the same as removing the sign bit.
 */
#define aiger_var2lit(i) \
  (((aiger_literal)(i)) << 1)

#define aiger_lit2var(l) \
  (((aiger_literal)(l)) >> 1)

/*------------------------------------------------------------------------*/
/* Callback functions for client memory management.  The 'free' wrapper will
 * get as last argument the size of the memory as it was allocated.
 */
typedef void *(*aiger_malloc) (void *mem_mgr, size_t);
typedef void (*aiger_free) (void *mem_mgr, void *ptr, size_t);

/*------------------------------------------------------------------------*/
/* Callback function for client character stream reading.  It returns an
 * ASCII character or EOF.  Thus is has the same semantics as the standard
 * library 'getc'.   See 'aiger_read_generic' for more details.
 */
typedef int (*aiger_get) (void *client_state);

/*------------------------------------------------------------------------*/
/* Callback function for client character stream writing.  The return value
 * is EOF iff writing failed and otherwise the character 'ch' casted to an
 * unsigned char.  It has therefore the same semantics as 'fputc' and 'putc'
 * from the standard library.
 */
typedef int (*aiger_put) (char ch, void *client_state);

/*------------------------------------------------------------------------*/

enum aiger_mode
{
  aiger_binary_mode = 0,
  aiger_ascii_mode = 1,
  aiger_stripped_mode = 2	/* can be ORed with one of the previous */
};

typedef enum aiger_mode aiger_mode;

/*------------------------------------------------------------------------*/

struct aiger_and
{
  aiger_literal lhs;			/* as literal [2..2*maxvar], even */
  aiger_literal rhs0;		/* as literal [0..2*maxvar+1] */
  aiger_literal rhs1;		/* as literal [0..2*maxvar+1] */
  uint8_t clausesForThisNodeExists:1;
  uint8_t nodeCopied:1;
};

/*------------------------------------------------------------------------*/

struct aiger_symbol
{
  aiger_literal lit;			/* as literal [0..2*maxvar+1] */
  aiger_literal next;		/* -"- (only used for latches) */
  char *name;
  uint8_t clausesForThisNodeExists:1; /* only for outputs */
  uint8_t nodeCopied:1;
};

/*------------------------------------------------------------------------*/
/* This is the externally visible state of the library.  The format is
 * almost the same as the ASCII file format.  The first part is exactly as
 * in the header 'M I L O A' after the format identifier string.
 */
struct aiger
{
  /* variable not literal index, e.g. maxlit = 2 * maxvar + 1 
   */
  aiger_variable maxvar;
  aiger_size num_inputs;
  aiger_size num_latches;
  aiger_size num_outputs;
  aiger_size num_ands;

  aiger_symbol *inputs;		/* [0..num_inputs[ */
  aiger_symbol *latches;	/* [0..num_latches[ */
  aiger_symbol *outputs;	/* [0..num_outputs[ */
  aiger_and *ands;		/* [0..num_ands[ */

  char **comments;		/* zero terminated */
};

typedef struct aiger_type aiger_type;

struct aiger_type
{
  unsigned input:1;
  unsigned latch:1;
  unsigned the_and:1;

  unsigned mark:1;
  unsigned onstack:1;

  /* Index int to 'public->{inputs,latches,ands}'.
   */
  aiger_size idx;

  aiger_size fanout;
};


/*------------------------------------------------------------------------*/
/* Version and CVS identifier.
 */
const char *aiger_id (void);
const char *aiger_version (void);

/*------------------------------------------------------------------------*/
/* You need to initialize the library first.  This generic initialization
 * function uses standard 'malloc' and 'free' from the standard library for
 * memory management.
 */
aiger *aiger_init (void);

/*------------------------------------------------------------------------*/
/* Same as previous initialization function except that a memory manager
 * from the client is used for memory allocation.  See the 'aiger_malloc'
 * and 'aiger_free' definitions above.
 */
aiger *aiger_init_mem (void *mem_mgr, aiger_malloc, aiger_free);

/*------------------------------------------------------------------------*/
/* Reset and delete the library.
 */
void aiger_reset (aiger *);

/*------------------------------------------------------------------------*/
/* Treat the literal 'lit' as input, output or latch respectively.  The
 * literal of latches and inputs can not be signed nor a constant (< 2).
 * You can not register latches nor inputs multiple times.  An input can not
 * be a latch.  The last argument is the symbolic name if non zero.
 * The same literal can of course be used for multiple outputs.
 */
void aiger_add_input (aiger *, aiger_literal lit, const char *);
void aiger_add_latch (aiger *, aiger_literal lit, aiger_literal next, const char *);
void aiger_add_output (aiger *, aiger_literal lit, const char *);

/*------------------------------------------------------------------------*/
/* Register an unsigned AND with AIGER.  The arguments are signed literals
 * as discussed above, e.g. the least significant bit stores the sign and
 * the remaining bit the variable index.  The 'lhs' has to be unsigned
 * (even).  It identifies the AND and can only be registered once.  After
 * registration an AND can be accessed through 'ands[aiger_lit2idx (lhs)]'.
 */
void aiger_add_and (aiger *, aiger_literal lhs, aiger_literal rhs0, aiger_literal rhs1);

/*------------------------------------------------------------------------*/
/* Add a line of comments.  The comment may not contain a new line character.
 */
void aiger_add_comment (aiger *, const char *comment_line);

/*------------------------------------------------------------------------*/
/* This checks the consistency for debugging and testing purposes.  In
 * particular, it is checked that all 'next' literals of latches, all
 * outputs, and all right hand sides of ANDs are defined, where defined
 * means that the corresponding literal is a constant 0 or 1, or defined as
 * an input, a latch, or AND gate.  Furthermore the definitions of ANDs are
 * checked to be non cyclic.  If a check fails a corresponding error message
 * is returned.
 */
const char *aiger_check (aiger *);

/*------------------------------------------------------------------------*/
/* These are the writer functions for AIGER.  They return zero on failure.
 * The assumptions on 'aiger_put' are the same as with 'fputc' from the
 * standard library (see the 'aiger_put' definition above).  Note, that
 * writing in binary mode triggers 'aig_reencode' and thus destroys the
 * original literal association and may even delete AND nodes.  See
 * 'aiger_reencode' for more details.
 */
int aiger_write_to_file (aiger *, aiger_mode, FILE *);
int aiger_write_to_string (aiger *, aiger_mode, char *str, size_t len);
int aiger_write_generic (aiger *, aiger_mode, void *state, aiger_put);

/*------------------------------------------------------------------------*/
/* The following function allows to write to a file.  The write mode is
 * determined from the suffix in the file name.  The mode used is ASCII for
 * a '.aag' suffix and binary mode otherwise.  In addition a '.gz' suffix can
 * be added which requests the file to written by piping it through 'gzip'.
 * This feature assumes that the 'gzip' program is in your path and can be
 * executed through 'popen'.  The return value is non zero on success.
 */
int aiger_open_and_write_to_file (aiger *, const char *file_name);

/*------------------------------------------------------------------------*/
/* The binary format reencodes all indices.  After normalization the input
 * indices come first followed by the latch and then the AND indices.  In
 * addition the indices will be topologically sorted to respect the
 * child/parent relation, e.g. child indices will always be smaller than
 * their parent indices.   This function can directly be called by the
 * client.  As a side effect, ANDs that are not in any cone of a next state
 * function nor in any cone of an output function are discarded.  The new
 * indices of ANDs start immediately after all input and latch indices.  The
 * data structures are updated accordingly including 'maxvar'.  The client
 * data within ANDs is reset to zero.
 */
int aiger_is_reencoded (aiger *);
void aiger_reencode (aiger *);


/*------------------------------------------------------------------------*/
/* This function computes the cone of influence (coi). The coi contains
 * those literals that may have an influence to one of the outputs.   A
 * variable 'v' is in the coi if the array returned as result is non zero at
 * position 'v'. All other variables can be considered redundant.  The array
 * returned is valid until the next call to this function and will be
 * deallocated on reset.
 */
const unsigned char * aiger_coi (aiger *);		/* [1..maxvar] */

/*------------------------------------------------------------------------*/
/* Read an AIG from a FILE, a string, or through a generic interface.  These
 * functions return a non zero error message if an error occurred and
 * otherwise 0.  The paramater 'aiger_get' has the same return values as
 * 'getc', e.g. it returns 'EOF' when done.  After an error occurred the
 * library becomes invalid.  Only 'aiger_reset' or 'aiger_error' can be
 * used.  The latter returns the previously returned error message.
 */
const char *aiger_read_from_file (aiger *, FILE *);
const char *aiger_read_from_string (aiger *, const char *str);
const char *aiger_read_generic (aiger *, void *state, aiger_get);

/*------------------------------------------------------------------------*/
/* Returns a previously generated error message if the library is in an
 * invalid state.  After this function returns a non zero error message,
 * only 'aiger_reset' can be called (beside 'aiger_error').  The library can
 * reach an invalid through a failed read attempt, or if 'aiger_check'
 * failed.
 */
const char *aiger_error (aiger *);

/*------------------------------------------------------------------------*/
/* Same semantics as with 'aiger_open_and_write_to_file' for reading.
 */
const char *aiger_open_and_read_from_file (aiger *, const char *);

/*------------------------------------------------------------------------*/
/* Write symbol table or the comments to a file.  Result is zero on failure.
 */
int aiger_write_symbols_to_file (aiger *, FILE * file);
int aiger_write_comments_to_file (aiger *, FILE * file);

/*------------------------------------------------------------------------*/
/* Remove symbols and comments.  The result is the number of symbols
 * and comments removed.
 */
aiger_size aiger_strip_symbols_and_comments (aiger *);

/*------------------------------------------------------------------------*/
/* If 'lit' is an input or a latch with a name, the symbolic name is
 * returned.   Note, that literals can be used for multiple outputs.
 * Therefore there is no way to associate a name with a literal itself.
 * Names for outputs are stored in the 'outputs' symbols and can only be
 * accesed through a linear traversal of the output symbols.
 */
const char *aiger_get_symbol (aiger *, aiger_literal lit);
aiger_size aiger_get_fanout(aiger *, aiger_literal lit);

/*------------------------------------------------------------------------*/
/* Check whether the given unsigned, e.g. even, literal was defined as
 * 'input', 'latch' or 'and'.  The command returns a zero pointer if the
 * literal was not defined as 'input', 'latch', or 'and' respectively.
 * Otherwise a pointer to the corresponding input or latch symbol is returned.
 * In the case of an 'and' the AND node is returned.  The returned symbol
 * if non zero is in the respective array of 'inputs', 'latches' and 'ands'.
 * It thus also allows to extract the position of an input or latch literal.
 * For outputs this is not possible, since the same literal may be used for
 * several outputs.
 */
aiger_symbol *aiger_is_input (aiger *, aiger_literal lit);
aiger_symbol *aiger_is_latch (aiger *, aiger_literal lit);
aiger_and *aiger_is_and (aiger *, aiger_literal lit);

aiger_type *
aiger_lit2type (aiger *pub, aiger_literal lit);
#endif
