#ifndef fibheap_internal_h_included
#define fibheap_internal_h_included

#include "funcsat/vec.h"
#include "funcsat/fibheap.h"

struct fibheap
{
  fibnode *min;
  int    num;
  /* int64_t    numMarked; */

  fibkey minKey;
  int (*compare)(fibkey *, fibkey *);


  uint64_t numDecrease;
  uint64_t numInsert;
  uint64_t numDelete;
  uint64_t numExtract;
};


/**
 * Splice the lists n and m, such that
 *
 *     |n->right == m && m->left == n|
 */
void fhsplice(fibnode *n, fibnode *m);

/**
 * Puts all the nodes of n, beginning with n and going to the right, in v.
 */
void toVector(fibnode *n, vector *v);

/**
 * @return True iff n has no other nodes in its list.
 */
bool isAlone(fibnode *n);
void unSplice(fibnode *n);

#endif
