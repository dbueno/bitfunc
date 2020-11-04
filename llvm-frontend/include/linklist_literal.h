#include "funcsat/system.h"
#ifndef linklist_literal_h_included
#define linklist_literal_h_included

struct lkl_literal
{
  literal data;
  struct lkl_literal *next;
};

struct lkl_literal *lkl_literal_alloc();
struct lkl_literal *lkl_literal_init(literal data);
void lkl_literal_destroy(struct lkl_literal *l);
void lkl_literal_destroy_all(struct lkl_literal *l);
struct lkl_literal *lkl_literal_cons(literal data, struct lkl_literal *next);

/* iterate over the linked list, possibly altering the list's structure (or
 * free'ing), the current node. */
#define for_lkl_literal_mutable(node, nx, start) for (node = (start), nx = (node ? node->next : (struct lkl_literal*)NULL); node; node = nx, (node ? nx = node->next : (struct lkl_literal*)NULL))

/* iterate over the linked list. the caller must not modify the list's structure
 * through the current node */
#define for_lkl_literal(node, start) for (node = (start); node; node = node->next)

#endif


