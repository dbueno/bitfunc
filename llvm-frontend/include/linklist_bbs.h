#include "llvm/Instructions.h"
#ifndef linklist_bbs_h_included
#define linklist_bbs_h_included

struct lkl_bbs
{
  llvm::BasicBlock* data;
  struct lkl_bbs *next;
};

struct lkl_bbs *lkl_bbs_alloc();
struct lkl_bbs *lkl_bbs_init(llvm::BasicBlock* data);
void lkl_bbs_destroy(struct lkl_bbs *l);
void lkl_bbs_destroy_all(struct lkl_bbs *l);
struct lkl_bbs *lkl_bbs_cons(llvm::BasicBlock* data, struct lkl_bbs *next);

/* iterate over the linked list, possibly altering the list's structure (or
 * free'ing), the current node. */
#define for_lkl_bbs_mutable(node, nx, start) for (node = (start), nx = (node ? node->next : (struct lkl_bbs*)NULL); node; node = nx, (node ? nx = node->next : (struct lkl_bbs*)NULL))

/* iterate over the linked list. the caller must not modify the list's structure
 * through the current node */
#define for_lkl_bbs(node, start) for (node = (start); node; node = node->next)

#endif


