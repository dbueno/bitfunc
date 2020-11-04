#include "funcsat/config.h"

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef __USE_POSIX199309
#define __USE_POSIX199309
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <signal.h>
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif
#include <time.h>
#include <libgen.h>
#include <limits.h>

#include "funcsat.h"
#include "funcsat/system.h"
#include "funcsat/types.h"
#include "funcsat/internal.h"
#include "funcsat/restart.h"
#include "funcsat/debug.h"
#include "funcsat/vecgen.h"
#include "funcsat/hashtable.h"
#include "funcsat/frontend.h"

VecGenTy(vecc, char);
VecGen(vecc, char);

int64_t readHeader(int (*getChar)(void *), void *, funcsat *func);
void readClauses(
  int (*getChar)(void *), void *, funcsat *func, uint64_t numClauses);


int fgetcWrapper(void *stream)
{
  return fgetc((FILE *) stream);
}

void parseDimacsCnf(const char *path, funcsat *func)
{
  struct stat buf;
  if (-1 == stat(path, &buf)) perror("stat"), exit(1);
  if (!S_ISREG(buf.st_mode)) {
    fprintf(stderr, "Error: '%s' not a regular file\n", path);
    exit(1);
  }
  int (*getChar)(void *);
  void *stream;
  const char *opener;
  bool isGz = 0 == strcmp(".gz", path + (strlen(path)-strlen(".gz")));
  if (isGz) {
#ifdef HAVE_LIBZ
    /* fprintf(stderr, "c found .gz file\n"); */
    getChar = (int(*)(void *))gzgetc;
    stream = gzopen(path, "r");
    opener = "gzopen";
#else
    fprintf(stderr, "cannot read gzipp'd file\n");
    exit(1);
#endif
  } else {
    getChar = fgetcWrapper;
    stream = fopen(path, "r");
    opener = "fopen";
  }
  if (!stream) {
    perror(opener);
    exit(1);
  }

  uint64_t numClauses = (uint64_t) readHeader(getChar, stream, func);
  readClauses(getChar, stream, func, numClauses);

  if (isGz) {
#ifdef HAVE_LIBZ
    if (Z_OK != gzclose(stream)) perror("gzclose"), exit(1);
#else
    assert(0 && "no libz and this shouldn't happen");
#endif
  } else {
    if (0 != fclose(stream)) perror("fclose"), exit(1);
  }
}


static literal readLiteral(
  int (*getChar)(void *stream),
  void *stream,
  vecc_t *tmp,
  uint64_t numClauses);
int64_t readHeader(int (*getChar)(void *stream), void *stream, funcsat *func)
{
  char c;
Comments:
  while (isspace(c = getChar(stream))); /* skip leading spaces */
  if ('c' == c) {
    while ('\n' != (c = getChar(stream)));
  }

  if ('p' != c) {
    goto Comments;
  }
  while ('c' != (c = getChar(stream)));
  assert(c == 'c');
  c = getChar(stream);
  assert(c == 'n');
  c = getChar(stream);
  assert(c == 'f');

  vecc_t tmp;
  veccInit(&tmp, 4);
  readLiteral(getChar, stream, &tmp, 0);
  unsigned numClauses = (unsigned) readLiteral(getChar, stream, &tmp, 0);
  veccDestroy(&tmp);
  /* fprintf(stderr, "c read header 'p cnf %u %u'\n", numVariables, numClauses); */
  return (int) numClauses;
}


void readClauses(
  int (*getChar)(void *stream),
  void *stream,
  funcsat *func,
  uint64_t numClauses)
{
  clause *clause;
  vecc_t tmp;
  veccInit(&tmp, 4);
  if (numClauses > 0) {
    do {
      clause = clauseAlloc(5);

      /* literal literal = readLiteral(getChar, stream); */
      literal literal = readLiteral(getChar, stream, &tmp, numClauses);

      while (literal != 0) {
        clausePush(clause, literal);
        literal = readLiteral(getChar, stream, &tmp, numClauses);
      }

      /* dimacsPrintClause(stderr, clause); */
      /* fprintf(stderr, "\n"); */
      if (FS_UNSAT == funcsatAddClause(func, clause)) {
        fprintf(stdout, "c trivially unsat\n");
        exit(EXIT_SUCCESS);
      }
    } while (--numClauses > 0);
  }
  veccDestroy(&tmp);
}

static literal readLiteral(
  int (*getChar)(void *stream),
  void *stream,
  vecc_t *tmp,
  uint64_t numClauses)
{
  char c;
  bool begun;
  veccClear(tmp);
  literal literal;
  begun = false;
  while (1) {
    c = getChar(stream);
    /* fprintf(stderr, "%c", c); */
    if (isspace(c) || EOF == c) {
      if (begun) {
        break;
      } else if (EOF == c) {
        fprintf(stderr, "readLiteral error: too few clauses (after %" PRIu64 " clauses)\n", numClauses);
        exit(1);
      } else {
        continue;
      }
    }
    begun = true;
    veccPush(tmp, c);
  }
  veccPush(tmp, '\0');
  literal = strtol((char *) tmp->data, NULL, 10);
  return literal;
}


