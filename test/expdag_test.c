#include <bitfunc/config.h>

#include <assert.h>
#include <bitfunc/expdag.h>
#include <stdlib.h>
#include <stdio.h>

#define CHECK_IS_ASSERT

#define ifok(x) if ((x) == 0)

/* test_ret should only be used to communicate the final test result. it will be
 * updated incrementally. */
#ifndef CHECK_IS_ASSERT
#define check(p) ifok (test_ret) test_ret = !(p);
#else
#define check assert
#endif

int test_expdag_constructors()
{
  bf_val *a, *b, *c, *x, *y, *z;
  bfman *BF;
  int test_ret = 0;

  BF = bfInit(AIG_MODE);

  x = bfo_uconstant(BF, 32, 0xfeed);
  check(x->sign == false);
  check(x->uconst == 0xfeed);
  check(x->ty == BF_VAL_CONST);
  check(x->kids == NULL);
  check(x->numkids == 0);
  bfo_consume_all(x);

  x = bfo_sconstant(BF, 32, -1);
  check(x->sign == true);
  check(x->sconst == -1);
  check(x->ty == BF_VAL_CONST);
  check(x->kids == NULL);
  check(x->numkids == 0);
  bfo_consume_all(x);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xdeef);
  z = bfo_eq(BF, x, y);
  check(z == bf_val_false);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_eq(BF, x, y);
  check(z == bf_val_true);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_ne(BF, x, y);
  check(z == bf_val_false);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_ult(BF, x, y);
  check(z->ty == BF_OP_ULT);
  check(z->kids[0] == x);
  check(z->kids[1] == y);
  check(z->width == x->width);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_ugt(BF, x, y);
  check(z->ty == BF_OP_UGT);
  check(z->kids[0] == x);
  check(z->kids[1] == y);
  check(z->width == x->width);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_slt(BF, x, y);
  check(z->ty == BF_OP_SLT);
  check(z->kids[0] == x);
  check(z->kids[1] == y);
  check(z->width == x->width);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_sgt(BF, x, y);
  check(z->ty == BF_OP_SGT);
  check(z->kids[0] == x);
  check(z->kids[1] == y);
  check(z->width == x->width);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_sle(BF, x, y);
  check(z->ty == BF_OP_SLE);
  check(z->kids[0] == x);
  check(z->kids[1] == y);
  check(z->width == x->width);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_sge(BF, x, y);
  check(z->ty == BF_OP_SGE);
  check(z->kids[0] == x);
  check(z->kids[1] == y);
  check(z->width == x->width);
  bfo_consume_all(z);

  x = bfo_uconstant(BF, 32, 0xfeed);
  y = bfo_uconstant(BF, 32, 0xfeed);
  z = bfo_shl(BF, x, y);
  check(z->ty == BF_OP_SHL);
  check(z->kids[0] == x);
  check(z->kids[1] == y);
  check(z->width == x->width);
  bfo_consume_all(z);

  bfDestroy(BF);
  return test_ret;
}

int main(int argc, char **argv)
{
  int test_ret = 0;

  ifok (test_ret) test_ret = test_expdag_constructors();

  fprintf(stderr, "test_ret is %d\n", test_ret);
  return test_ret;
}
