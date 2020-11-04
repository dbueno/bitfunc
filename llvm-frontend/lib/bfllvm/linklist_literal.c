#include "linklist_literal.h"

struct lkl_literal *lkl_literal_alloc()
{
  struct lkl_literal *r = (struct lkl_literal*)calloc(1, sizeof(*r));
  return r;
}

struct lkl_literal *lkl_literal_init(literal data)
{
  struct lkl_literal *r = lkl_literal_alloc();
  r->data = data;
  return r;
}

void lkl_literal_destroy(struct lkl_literal *l)
{
  free(l);
}

void lkl_literal_destroy_all(struct lkl_literal *l)
{
  struct lkl_literal *n = l->next;
  lkl_literal_destroy(l);
  if (n) lkl_literal_destroy_all(n);
}

struct lkl_literal *lkl_literal_cons(literal data, struct lkl_literal *next)
{
  struct lkl_literal *r = lkl_literal_init(data);
  r->next = next;
  return r;
}

