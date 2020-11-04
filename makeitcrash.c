/*
 * Copyright 2012 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
 * or on behalf of the U.S. Government. Export of this program may require a
 * license from the United States Government.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#include <bitfunc/bap.h> 
#include <bitfunc.h>
#include <funcsat/hashtable.h>
#include <funcsat/hashtable_itr.h>

int main(int argc, char **argv)
{
  char ilfilename[] = "makeitcrash_il_XXXXXX";
  char inputfilename[] = "makitcrash_in_XXXXXX";
  if (argc < 3) {
    fprintf(stderr, "specify a script to run and a seed file\n");
    exit(1);
  }

  int ilfile = mkstemp(ilfilename);
  if (ilfile == -1) {
    perror("Could not create temporary il file");
    exit(1);
  }

  int inputfile = mkstemp(inputfilename);
  if (inputfile == -1) {
    perror("Could not create temporary input file");
    exit(1);
  }

  size_t inputsize;
  struct stat input_stat;

  char buf[512];
  int seedfile = open(argv[2], O_RDONLY);
  if (seedfile == -1) {
    perror("Could not open seed file");
    exit(1);
  }
  if (fstat(seedfile, &input_stat) == -1) {
    perror("Could not stat seed file");
    exit(1);
  }
  inputsize = input_stat.st_size;
  ssize_t nr;
  while ((nr = read(seedfile, buf, 512)) != 0) {
    if (nr == -1) {
      perror("Could not read from seed file");
      exit(1);
    }
    ssize_t off = 0;
    while (off < nr) {
      ssize_t nw = write(inputfile, buf+off, nr-off);
      if (nw == -1) {
        perror("Could not write to input file");
        exit(1);
      }
      off += nw;
    }
  }

  uint8_t *inputmap = mmap(NULL, inputsize, PROT_READ | PROT_WRITE, MAP_SHARED, inputfile, 0);

  bfman *bf = bfInit(AIG_MODE);

  while (1) {
    pid_t child;
    if ((child = fork()) != 0) {
      int child_stat;
      waitpid(child, &child_stat, 0);
      if (WIFSIGNALED(child_stat) || (WIFEXITED(child_stat) && WEXITSTATUS(child_stat) != 0)) {
        break;
      }
    } else {
      execlp(argv[1], argv[1], inputfilename, ilfilename, NULL);
    }

    bfDestroy(bf);
    bf = bfInit(AIG_MODE);
    bap_machine *mach = bap_create_machine(bf);
    bap_execute_il(mach, ilfilename);
    if (!mach->has_error) {
      size_t nconds = mach->path_constraints->size;
      while (1) {
        printf("Resetting\n");
        bfPopAssumptions(bf, bf->assumps->size);
        printf("Sanity check..");
        fflush(stdout);
        for (size_t i = 0; i < nconds; ++i) {
          bfPushAssumption(mach->bf, mach->path_constraints->data[i]);
        }
        bfresult res = bfSolve(bf);
        switch (res) {
          case BF_SAT:
            printf("ok\n");
            break;
          case BF_UNSAT:
            printf("unsat oops\n");
            exit(1);
            break;
          default:
            printf("Unhandled solve result\n");
            exit(1);
        }
        bfPopAssumptions(bf, bf->assumps->size);
        size_t flipcond = random() % nconds;
        //flipcond = nconds - 1;
        for (size_t i = 0; i < nconds; ++i) {
          if (i == flipcond) {
            printf("Flipping condition %zu of %zu\n", i, nconds);
            bfPushAssumption(mach->bf, -mach->path_constraints->data[i]);
            break;
          } else {
            bfPushAssumption(mach->bf, mach->path_constraints->data[i]);
          }
        }
        res = bfSolve(bf);
        switch (res) {
          case BF_SAT:
            goto solved;
            break;
          case BF_UNSAT:
            printf("unsat\n");
            continue;
            break;
          default:
            printf("Unhandled solve result\n");
            exit(1);
        }
      }
solved:
      for (size_t i = 0; i < inputsize; i++) {
        struct hashtable_itr *i = hashtable_iterator(mach->bindings);
        do {
          size_t e;
          bap_val *v = (bap_val*)hashtable_iterator_value(i);
          char *name = (char*)hashtable_iterator_key(i);
          if (sscanf(name, "symb_%zu", &e) == 1) {
            inputmap[e-1] = (uint8_t)(bfvGet(bf, v->v.bv) & 0xff);
          }
        } while (hashtable_iterator_advance(i));
        if (msync(inputmap, inputsize, MS_SYNC) == -1) {
          perror("Syncing input file");
          exit(1);
        }
      }
    } else {
      break;
    }
    bap_destroy_machine(mach);
  }

  printf("Crashing input is in %s\n", inputfilename);
  printf("Final IL is in %s\n", ilfilename);
}
