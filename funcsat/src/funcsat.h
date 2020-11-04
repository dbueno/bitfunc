/*11:*/
#line 252 "funcsat.w"

#ifndef funcsat_h_included
#define funcsat_h_included
#include <stdio.h> 
#include <stdlib.h> 

/*13:*/
#line 282 "funcsat.w"

#include "funcsat/system.h"
#include "funcsat/posvec.h"
#include "funcsat/hashtable.h"


/*:13*//*17:*/
#line 413 "funcsat.w"

typedef enum
{
FS_UNKNOWN= 0,
FS_SAT= 10,
FS_UNSAT= 20
}funcsat_result;

/*:17*//*23:*/
#line 789 "funcsat.w"

typedef struct clause
{
literal*data;

uint32_t size;

uint32_t capacity;

uint32_t isLearnt:1;
uint32_t isReason:1;
uint32_t is_watched:1;
uint32_t lbdScore:29;

double activity;

struct clause*nx;

}clause;

/*:23*//*63:*/
#line 1545 "funcsat.w"

enum reason_ty
{
REASON_CLS_TY
};

/*:63*//*148:*/
#line 4219 "funcsat.w"

typedef struct funcsat funcsat;
typedef struct funcsat_config
{
void*user;

char*name;


struct hashtable*logSyms;
posvec logStack;
bool printLogLabel;
FILE*debugStream;

bool usePhaseSaving;

bool useSelfSubsumingResolution;

bool minimizeLearnedClauses;

uint32_t numUipsToLearn;


bool gc;

unsigned maxJailDecisionLevel;



bool(*isTimeToRestart)(funcsat*,void*);






bool(*isResourceLimitHit)(funcsat*,void*);






funcsat_result(*preprocessNewClause)(funcsat*,void*,clause*);




funcsat_result(*preprocessBeforeSolve)(funcsat*,void*);




double(*getInitialActivity)(variable*);




void(*bumpOriginal)(funcsat*,clause*orig);





void(*bumpReason)(funcsat*,clause*rsn);




void(*bumpLearned)(funcsat*,clause*learned);




void(*bumpUnitClause)(funcsat*,clause*uc);




void(*decayAfterConflict)(funcsat*f);




void(*sweepClauses)(funcsat*,void*);




}funcsat_config;





/*:148*/
#line 258 "funcsat.w"


/*15:*/
#line 326 "funcsat.w"


#define CallocX(ptr, n, size)                   \
  do {                                          \
    ptr =  calloc(n, size);                      \
    if (!ptr) perror("calloc"), abort();        \
  } while (0);

#define MallocX(ptr, n, size)                   \
  do {                                          \
    ptr =  malloc((n) * (size));                 \
    if (!ptr) perror("malloc"), abort();        \
  } while (0);

#define MallocTyX(ty, ptr, n, size)             \
  do {                                          \
    ptr =  (ty) malloc((n)*(size));              \
    if (!ptr) perror("malloc"), abort();        \
  } while (0);

#define ReallocX(ptr, n, size)                                          \
  do {                                                                  \
    void *tmp_funcsat_ptr__;                                            \
    tmp_funcsat_ptr__ =  realloc(ptr, (n)*(size));                       \
    if (!tmp_funcsat_ptr__)  free(ptr), perror("realloc"), exit(1);     \
    ptr =  tmp_funcsat_ptr__;                                            \
  } while (0);


/*:15*//*24:*/
#line 811 "funcsat.w"

clause*clauseAlloc(uint32_t capacity);

void clauseInit(clause*v,uint32_t capacity);

void clauseDestroy(clause*);



void clauseFree(clause*);

void clauseClear(clause*v);



void clausePush(clause*v,literal data);



void clausePushAt(clause*v,literal data,uint32_t i);
void clauseGrowTo(clause*v,uint32_t newCapacity);
literal clausePop(clause*v);
literal clausePopAt(clause*v,uint32_t i);
literal clausePeek(clause*v);

void clauseCopy(clause*dst,clause*src);



void dimacsPrintClause(FILE*f,clause*);

#define forClause(elt, vec) for (elt =  (vec)->data; elt != (vec)->data + (vec)->size; elt++)
#define for_clause(elt, cl) for (elt =  (cl)->data; elt != (cl)->data + (cl)->size; elt++)



/*:24*//*68:*/
#line 1597 "funcsat.w"

void funcsatAddReasonHook(funcsat*f,uintptr_t ty,clause*(*hook)(funcsat*f,literal l));

/*:68*//*144:*/
#line 4113 "funcsat.w"

struct funcsat_config;
funcsat*funcsatInit(funcsat_config*conf);

funcsat_config*funcsatConfigInit(void*userData);

void funcsatConfigDestroy(funcsat_config*);

void funcsatResize(funcsat*f,variable numVars);

void funcsatDestroy(funcsat*);

funcsat_result funcsatAddClause(funcsat*func,clause*clause);

funcsat_result funcsatPushAssumption(funcsat*f,literal p);

funcsat_result funcsatPushAssumptions(funcsat*f,clause*c);

void funcsatPopAssumptions(funcsat*f,unsigned num);

funcsat_result funcsatSolve(funcsat*func);

unsigned funcsatNumClauses(funcsat*func);

unsigned funcsatNumVars(funcsat*func);

void funcsatPrintStats(FILE*stream,funcsat*f);

void funcsatPrintColumnStats(FILE*stream,funcsat*f);

void funcsatClearStats(funcsat*f);

void funcsatBumpLitPriority(funcsat*f,literal p);

void funcsatPrintCnf(FILE*stream,funcsat*f,bool learned);

funcsat_result funcsatResult(funcsat*f);

clause*funcsatSolToClause(funcsat*f);

int funcsatSolCount(funcsat*f,clause subset,clause*lastSolution);

void funcsatReset(funcsat*f);

void funcsatSetupActivityGc(funcsat_config*);
void funcsatSetupLbdGc(funcsat_config*);

bool funcsatDebug(funcsat*f,char*label,int level);

clause*funcsatRemoveClause(funcsat*f,clause*c);


/*:144*//*145:*/
#line 4167 "funcsat.w"

char*funcsatResultAsString(funcsat_result result);


/*:145*//*146:*/
#line 4174 "funcsat.w"

void fs_vig_print(funcsat*f,const char*path);

/*:146*//*160:*/
#line 5623 "funcsat.w"

funcsat_result fs_parse_dimacs_solution(funcsat*f,FILE*solutionFile);


/*:160*/
#line 260 "funcsat.w"


#endif

/*:11*/
