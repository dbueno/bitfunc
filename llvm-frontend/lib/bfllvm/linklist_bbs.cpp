#include "linklist_bbs.h"

struct lkl_bbs *lkl_bbs_alloc()
{
  struct lkl_bbs *r = (struct lkl_bbs*)calloc(1, sizeof(*r));
  return r;
}

struct lkl_bbs *lkl_bbs_init(llvm::BasicBlock* data)
{
  struct lkl_bbs *r = lkl_bbs_alloc();
  r->data = data;
  return r;
}

void lkl_bbs_destroy(struct lkl_bbs *l)
{
  free(l);
}

void lkl_bbs_destroy_all(struct lkl_bbs *l)
{
  struct lkl_bbs *n = l->next;
  lkl_bbs_destroy(l);
  if (n) lkl_bbs_destroy_all(n);
}

struct lkl_bbs *lkl_bbs_cons(llvm::BasicBlock* data, struct lkl_bbs *next)
{
  struct lkl_bbs *r = lkl_bbs_init(data);
  r->next = next;
  return r;
}

