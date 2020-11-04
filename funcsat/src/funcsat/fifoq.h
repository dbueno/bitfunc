#ifndef fifoq_h_included
#define fifoq_h_included

#include "funcsat/vec.h"

#define NewFifoQTy(fifoTy, vecTy, eltTy)                        \
  typedef struct                                                \
  {                                                             \
    vecTy ## _t t;                     /* top in order */       \
    vecTy ## _t r;                     /* rest in rev-order */  \
  } fifoTy ## _t;                                               \
  void fifoTy ## Init(fifoTy ## _t *q, size_t hint);            \
  void fifoTy ## Destroy(fifoTy ## _t *q);                      \
  void fifoTy ## Clear(fifoTy ## _t *q);                        \
  void fifoTy ## Push(fifoTy ## _t *q, eltTy x);                \
  eltTy fifoTy ## Pop(fifoTy ## _t *q);                         \
  eltTy fifoTy ## Peek(fifoTy ## _t *q);                        \
  uint64_t fifoTy ## Size(fifoTy ## _t *q);

#define NewFifoQ(fifoTy, vecTy, eltTy)                  \
  void fifoTy ## Init(fifoTy ## _t *q, size_t hint)     \
  {                                                     \
    vecTy ## Init(&q->t, (hint/2)+1);                   \
    vecTy ## Init(&q->r, (hint/2)+1);                   \
  }                                                     \
                                                        \
  void fifoTy ## Destroy(fifoTy ## _t *q)               \
  {                                                     \
    vecTy ## Destroy(&q->t);                            \
    vecTy ## Destroy(&q->r);                            \
  }                                                     \
                                                        \
  void fifoTy ## Clear(fifoTy ## _t *q)                 \
  {                                                     \
    vecTy ## Clear(&q->t);                              \
    vecTy ## Clear(&q->r);                              \
  }                                                     \
                                                        \
  void fifoTy ## Push(fifoTy ## _t *q, eltTy x)         \
  {                                                     \
    vecTy ## Push(&q->r, x);                            \
  }                                                     \
                                                        \
  eltTy fifoTy ## Pop(fifoTy ## _t *q)                  \
  {                                                     \
    eltTy elt = fifoTy ## Peek(q);                      \
    vecTy ## Pop(&q->t);                                \
    return elt;                                         \
  }                                                     \
                                                        \
  eltTy fifoTy ## Peek(fifoTy ## _t *q)                 \
  {                                                     \
    if (0 == q->t.size) {                               \
      while (0 != q->r.size) {                          \
        eltTy x = vecTy ## Pop(&q->r);                  \
        vecTy ## Push(&q->t, x);                        \
      }                                                 \
    }                                                   \
    assert(q->t.size > 0 && "empty queue");             \
    return vecTy ## Peek(&q->t);                        \
  }                                                     \
                                                        \
  uint64_t fifoTy ## Size(fifoTy ## _t *q)              \
  {                                                     \
    return q->t.size + q->r.size;                       \
  }


/* NewFifoQTy(fifoi, veci, int64_t); */

#endif
