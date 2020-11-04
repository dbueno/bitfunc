#include <funcsat/config.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include <funcsat/fibheap.h>
#include <funcsat/fibheap_internal.h>
#include "genrand.h"

#define UNUSED(x) (void)(x)

static int compareDouble(double *i, double *j);

/* generates at most ub nodes. */
static fibnode *genNodes(uint32_t ub)
{
  long int x = randomPosNum(ub);
  fibnode *n = fibheapMkNode(0.f, 0);
  for (unsigned i = 1; i < (unsigned) x; i++) {
    fhsplice(n, fibheapMkNode((double) i, i));
  }
  return n;
}

static fibnode *genRandNodes(uint32_t ub)
{
  long int x = randomPosNum(ub);
  double d = randomDouble();
  fibnode *n = fibheapMkNode(d, 0);
  for (unsigned i = 1; i < (unsigned) x; i++) {
    fhsplice(n, fibheapMkNode(randomDouble(), i));
  }
  return n;
}


static bool testCompareDouble()
{
  double x, y;

  x = 0.f, y = 0.f;
  assert(compareDouble(&x, &y) == 0);
  x = 1.f, y = 0.f;
  assert(compareDouble(&x, &y) == 1);
  x = 0.f, y = 1.f;
  assert(compareDouble(&x, &y) == -1);
  x = -0.f, y = -0.f;
  assert(compareDouble(&x, &y) == 0);
  x = -1.f, y = -0.f;
  assert(compareDouble(&x, &y) == -1);
  x = -0.f, y = -1.f;
  assert(compareDouble(&x, &y) == 1);

  for (unsigned i = 0; i < 100; i++) {
    long int p = randomPosNum(1000), q = randomPosNum(1000);
    long int sign = randomPosNum(2);
    x = p * (sign == 1 ? 1.f : -1.f), y = q * (sign == 1 ? 1.f : -1.f);
    int ret = 20;
    if (x < y) ret = -1;
    else if (x == y) ret = 0;
    else if (x > y) ret = 1;
    assert(ret != 20);
    assert(compareDouble(&x, &y) == ret);
  }

  return true;
}


static bool testSplice()
{
  fibnode *m = fibheapMkNode(1.f, 1);
  assert(!m->isInHeap);
  fibnode *n = fibheapMkNode(2.f, 2);
  fibnode *o = fibheapMkNode(3.f, 3);
  fibnode *p = fibheapMkNode(4.f, 4);

  assert(m->left == m->right);

  fhsplice(m, n);
  /* m <--> n */
  assert(m->left == n);
  assert(m->right == n);
  assert(n->left == m);
  assert(n->right == m);

  fhsplice(n, o);
  /* m <--> n <--> o */
  assert(m->right == n);
  assert(m->left == o);
  assert(n->right == o);
  assert(n->left == m);
  assert(o->right == m);
  assert(o->left == n);

  fhsplice(o, p);
  /* m <--> n <--> o <--> p */
  assert(m->right == n);
  assert(m->left == p);
  assert(n->right == o);
  assert(n->left == m);
  assert(o->right == p);
  assert(o->left == n);
  assert(p->right == m);
  assert(p->left == o);

  fibheapFreeNodes(m);

  return true;
}


static bool testToVector()
{
  vector v;
  int times = 100, numNodes = 30;
  vectorInit(&v, (unsigned)numNodes);
  while (times-- > 0) {
    fibnode *n = genNodes((uint32_t)numNodes), *start = n;
    int i = 0;
    vectorClear(&v);
    toVector(n, &v);

    do {
      assert(v.data[i++] == n);
      n = n->right;
    } while (n != start);
    fibheapFreeNodes(n);
  }
  vectorDestroy(&v);

  return true;
}


static bool testUnSplice()
{
  vector v;
  int times = 100, numNodes = 30;
  vectorInit(&v, (unsigned)numNodes);
  while (times-- > 0) {
    fibnode *n = genNodes((uint32_t)numNodes);
    fibnode *start = n->right;
    vectorClear(&v);
    if (mumble()) {
      n = n->left;
    }
    unSplice(n);
    assert(isAlone(n));

    /* make sure n is not in start assuming there were at least 2 nodes to begin
     * with */
    if (n != start) {
      fibheapFreeNodes(n);
      n = start;
      toVector(n, &v);
      int i = 0;
      do {
        assert(v.data[i++] == n);
        n = n->right;
      } while (n != start);
      fibheapFreeNodes(start);
    } else {
      fibheapFreeNodes(n);
    }
  }
  vectorDestroy(&v);

  return true;
}


static bool testInsertExtract1()
{
  fibheap *h = fibheapInit(compareDouble, 0.f);
  fibnode *n = fibheapMkNode(1.f, 1);
  fibnode *m = fibheapMkNode(2.f, 2);
  fibnode *o = fibheapMkNode(0.5f, 3);
  fibnode *out;
  assert(!n->isInHeap);
  assert(!m->isInHeap);
  assert(!o->isInHeap);
  fibheapInsert(h,n);
  assert(n->isInHeap);
  fibheapInsert(h,m);
  assert(m->isInHeap);
  fibheapInsert(h,o);
  assert(o->isInHeap);
  out = fibheapExtractMin(h);
  assert(o == out);
  assert(!o->isInHeap);
  free(o);
  out = fibheapExtractMin(h);
  assert(n == out);
  assert(!n->isInHeap);
  free(n);
  out = fibheapExtractMin(h);
  assert(m == out);
  assert(!m->isInHeap);
  free(m);
  free(h);

  return true;
}


static bool testDecreaseKey1()
{
  fibheap *h = fibheapInit(compareDouble, 0.f);
  fibnode *n = fibheapMkNode(1.f, 1);
  fibnode *m = fibheapMkNode(2.f, 2);
  fibnode *o = fibheapMkNode(0.5f, 3);
  fibnode *out;
  assert(!n->isInHeap);
  assert(!m->isInHeap);
  assert(!o->isInHeap);
  assert(fibheapNum(h) == 0);
  fibheapInsert(h,n);
  assert(fibheapNum(h) == 1);
  fibheapInsert(h,m);
  assert(fibheapNum(h) == 2);
  fibheapInsert(h,o);
  assert(fibheapNum(h) == 3);
  fibheapDecreaseKey(h, n, 0.3f);
  out = fibheapExtractMin(h);
  assert(fibheapNum(h) == 2);
  assert(n == out);
  assert(!n->isInHeap);
  free(n);
  out = fibheapExtractMin(h);
  assert(fibheapNum(h) == 1);
  assert(o == out);
  assert(!o->isInHeap);
  free(o);
  out = fibheapExtractMin(h);
  assert(fibheapNum(h) == 0);
  assert(m == out);
  assert(!m->isInHeap);
  free(m);
  free(h);

  return true;
}


/* makes sure that whenever you decrease the key to the top, you get that node
 * first. */
static bool testHeap1()
{
  vector v;
  fibheap *h = fibheapInit(compareDouble, -100.f);
  int times = 100, numNodes = 1000;
  vectorInit(&v, (unsigned)numNodes);
  while (times-- > 0) {
    toVector(genNodes((uint32_t)numNodes), &v);
    int i;
    for (i = 0; i < (int)v.size; i++) {
      fibnode *n = v.data[i];
      unSplice(n);
      fibheapInsert(h, n);
      assert(n->isInHeap);
    }
    while (v.size > 0) {
      fibnode *n = vectorPop(&v), *o;
      fibheapDecreaseKey(h, n, -100.f);
      o = fibheapExtractMin(h);
      assert(n == o);
      assert(!o->isInHeap);
      free(o);
    }
  }
  vectorDestroy(&v);
  free(h);

  return true;
}


static bool testHeap2()
{
  vector v;
  fibheap *h = fibheapInit(compareDouble, 0.f);
  int times = 100, numNodes = 1000;
  vectorInit(&v, (unsigned)numNodes);
  while (times-- > 0) {
    toVector(genRandNodes((uint32_t)numNodes), &v);
    /* fprintf(stderr, "gen %u nodes\n", v.size); */
    int mysize = 0;
    while (v.size > 0) {
      fibnode *n = vectorPop(&v);
      unSplice(n);
      assert(n->key >= 0.f);
      fibheapInsert(h,n);
      assert(fibheapNum(h) == ++mysize);
    }

    fibkey last = -1.f;
    while (fibheapNum(h) > 0) {
      /* fprintf(stderr, "fibheapNum(h) = %i\n", fibheapNum(h)); */
      fibnode *n = fibheapExtractMin(h);
      assert(last <= n->key);
      last = n->key;
      free(n);
    }
  }

  vectorDestroy(&v);
  free(h);

  return true;
}


int main(int argc, char **argv)
{
  UNUSED(argc), UNUSED(argv);
  bool passed = true;
  srandom(1);

  printf("  * testCompareDouble\n");
  passed = passed && testCompareDouble();
  printf("  * testSplice\n");
  passed = passed && testSplice();
  printf("  * testToVector\n");
  passed = passed && testToVector();
  printf("  * testUnSplice\n");
  passed = passed && testUnSplice();
  printf("  * testInsertExtract1\n");
  passed = passed && testInsertExtract1();
  printf("  * testDecreaseKey1\n");
  passed = passed && testDecreaseKey1();
  printf("  * testHeap1\n");
  passed = passed && testHeap1();
  printf("  * testHeap2\n");
  passed = passed && testHeap2();

  return passed ? 0 : 1;
}




static int compareDouble(double *i, double *j)
{
  if (*i < *j) {
    return -1;
  } else if (*i > *j) {
    return 1;
  } else {
    return 0;
  }
}
