#include <bitfunc.h>
#include <bitfunc/bitvec.h>
#define UNUSED(x) (void)(x)

int main(int argc, char **argv)
{
  UNUSED(argc), UNUSED(argv);
  bfman *BF = bfInit(AIG_MODE);
  bitvec *X = bfvInit(BF, 3);
  bitvec *Y = bfvInit(BF, 3);

  bitvec *Z = bfvAdd0(BF, X, Y);
  bfPrintAIG_dot(BF, "3-bit-adder.dot");
  literal *p;
  forBv (p, Z) {
    fprintf(stderr, "%i ", *p);
  }
  bfDestroy(BF);
  return 0;
}
