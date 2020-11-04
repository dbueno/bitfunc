/*12:*/
#line 266 "funcsat.w"

#ifndef funcsat_internal_h_included
#define funcsat_internal_h_included
#include <funcsat/vec.h> 
#include <funcsat/intvec.h> 
#include <funcsat/fibheap.h> 

/*149:*/
#line 4321 "funcsat.w"


#ifndef NDEBUG
#define assertExit(code, cond) \
  if (!(cond)) exit(code);
#else
#define assertExit(code, cond)
#endif

/*:149*//*150:*/
#line 4335 "funcsat.w"

#ifdef FUNCSAT_LOG
#ifndef SWIG
DECLARE_HASHTABLE(fsLogMapInsert,fsLogMapSearch,fsLogMapRemove,char,int)
#endif

#define fs_ifdbg(f, label, level)                                        \
  if ((f)->conf->logSyms &&                                             \
      fsLogMapSearch(f->conf->logSyms, (void *) (label)) &&             \
      (level) <= *fsLogMapSearch((f)->conf->logSyms, (void *) (label)))

#else

#define fs_ifdbg(f, label, level) if (false)

#endif

static inline int fslog(const struct funcsat*,const char*label,
int level,const char*format,...);
static inline int dopen(const struct funcsat*,const char*label);
static inline int dclose(const struct funcsat*,const char*label);

/*:150*/
#line 273 "funcsat.w"

/*26:*/
#line 864 "funcsat.w"

typedef struct head_tail
{
clause*hd;
clause*tl;
}head_tail;

/*:26*//*34:*/
#line 1041 "funcsat.w"

struct watchlist_elt
{
literal lit;
clause*cls;
};


/*:34*//*35:*/
#line 1058 "funcsat.w"

#ifndef WATCHLIST_HEAD_SIZE_MAX
#  define WATCHLIST_HEAD_SIZE_MAX 4
#endif

struct watchlist
{
uint32_t size;
uint32_t capacity;

struct watchlist_elt elts[WATCHLIST_HEAD_SIZE_MAX];
struct watchlist_elt*rest;
};

/*:35*//*37:*/
#line 1086 "funcsat.w"

typedef struct all_watches
{
struct watchlist*wlist;
unsigned size;
unsigned capacity;
}all_watches;

/*:37*//*64:*/
#line 1556 "funcsat.w"

struct reason_info
{
uintptr_t ty;
clause*cls;
};
#include "funcsat/vec_reason_info.h"


/*:64*//*85:*/
#line 1942 "funcsat.w"

struct bh_node
{
variable var;
double act;
};

/*:85*//*142:*/
#line 4020 "funcsat.w"

#ifndef CLAUSE_HEAD_SIZE
#  define CLAUSE_HEAD_SIZE 4    
#endif

#ifndef CLAUSE_BLOCK_SIZE
#  define CLAUSE_BLOCK_SIZE 8
#endif

enum clause_state{
CLAUSE_WATCHED,
CLAUSE_JAILED,
CLAUSE_UNIT,
};

typedef struct clause_head
{
literal headlits[CLAUSE_HEAD_SIZE];

uint32_t is_learned:1;
uint32_t is_reason:1;
uint32_t is_bigger:1;
uint32_t where:2;
uint32_t lbd_score_or_activity:27;

}clause_head;

typedef struct clause_block
{
literal blocklits[CLAUSE_BLOCK_SIZE];
unsigned next_block;
}clause_block;

/*:142*/
#line 274 "funcsat.w"

/*18:*/
#line 425 "funcsat.w"

struct funcsat_config;
struct clause_head;

struct funcsat
{
struct funcsat_config*conf;

clause assumptions;

funcsat_result lastResult;

unsigned decisionLevel;

unsigned propq;




clause trail;

posvec model;


clause phase;


clause level;


posvec decisions;


struct vec_uintptr*reason;




vector reason_hooks;



struct vec_reason_info*reason_infos;


uintptr_t reason_infos_freelist;

head_tail*unit_facts;

unsigned unit_facts_size;
unsigned unit_facts_capacity;

head_tail*jail;



all_watches watches;


vector origClauses;
vector learnedClauses;
unsigned numVars;

#if 0
unsigned freelist;
struct clause_head*pool;
#endif

clause*conflictClause;



posvec litPos;

clause uipClause;

vector subsumed;



unsigned lbdCount;
posvec lbdLevels;


double claDecay;
double claInc;
double learnedSizeFactor;




double maxLearned;
double learnedSizeAdjustConfl;
uint64_t learnedSizeAdjustCnt;
double learnedSizeAdjustInc;
double learnedSizeInc;



posvec seen;
intvec analyseToClear;
posvec analyseStack;
posvec allLevels;



double varInc;
double varDecay;

struct bh_node*binvar_heap;
unsigned binvar_heap_size;
unsigned*binvar_pos;


int64_t lrestart;
uint64_t lubycnt;
uint64_t lubymaxdelta;
bool waslubymaxdelta;


double pct;
double pctdepth;
int correction;



struct drand48_data*rand;



uint64_t numSolves;


uint64_t numOrigClauses;


uint64_t numLearnedClauses;


uint64_t numSweeps;


uint64_t numLearnedDeleted;


uint64_t numLiteralsDeleted;



uint64_t numProps;
uint64_t numUnitFactProps;

uint64_t numJails;


uint64_t numConflicts;


uint64_t numRestarts;


uint64_t numDecisions;


uint64_t numSubsumptions;



uint64_t numSubsumedOrigClauses;


uint64_t numSubsumptionUips;
};


/*:18*/
#line 275 "funcsat.w"

/*25:*/
#line 849 "funcsat.w"



void fs_clause_print(funcsat*f,FILE*out,clause*c);
void dimacsPrintClauses(FILE*f,vector*);




/*:25*//*28:*/
#line 905 "funcsat.w"

static inline void head_tail_clear(head_tail*ht);
static inline bool head_tail_is_empty(head_tail*ht);
static inline void head_tail_append(head_tail*ht1,head_tail*ht2);
static inline void head_tail_add(head_tail*ht1,clause*c);


/*:28*//*31:*/
#line 964 "funcsat.w"

extern void head_tail_print(funcsat*f,FILE*out,head_tail*l);


/*:31*//*32:*/
#line 970 "funcsat.w"

bool bcp(funcsat*f);

/*:32*//*40:*/
#line 1116 "funcsat.w"

static inline bool watchlist_is_elt_in_head(struct watchlist_elt*elt,struct watchlist*wl)
{
return(elt>=wl->elts&&elt<(wl)->elts+WATCHLIST_HEAD_SIZE_MAX);
}
static inline uint32_t watchlist_head_size(struct watchlist*wl)
{
return((wl)->size> WATCHLIST_HEAD_SIZE_MAX?WATCHLIST_HEAD_SIZE_MAX:(wl)->size);
}
static inline struct watchlist_elt*watchlist_head_size_ptr(struct watchlist*wl)
{
return((wl)->elts+watchlist_head_size(wl));
}
static inline uint32_t watchlist_rest_size(struct watchlist*wl)
{
return((wl)->size> WATCHLIST_HEAD_SIZE_MAX?(wl)->size-WATCHLIST_HEAD_SIZE_MAX:0);
}
static inline struct watchlist_elt*watchlist_rest_size_ptr(struct watchlist*wl)
{
return((wl)->rest+watchlist_rest_size(wl));
}


/*:40*//*41:*/
#line 1141 "funcsat.w"

static inline void all_watches_init(funcsat*f);
static inline void all_watches_destroy(funcsat*f);

/*:41*//*46:*/
#line 1199 "funcsat.w"

static int compare_pointer(const void*x,const void*y);


/*:46*//*49:*/
#line 1275 "funcsat.w"

static inline void watchlist_check(funcsat*f,literal l);
static inline void watches_check(funcsat*f);


/*:49*//*58:*/
#line 1458 "funcsat.w"

static inline void watch_l0(funcsat*f,clause*c);
static inline void watch_l1(funcsat*f,clause*c);

/*:58*//*61:*/
#line 1504 "funcsat.w"

static inline void addWatch(funcsat*f,clause*c);
static inline void addWatchUnchecked(funcsat*f,clause*c);
static inline void makeWatchable(funcsat*f,clause*c);

/*:61*//*78:*/
#line 1745 "funcsat.w"


void trailPush(funcsat*f,literal p,uintptr_t reason_info_idx);
literal trailPop(funcsat*f,head_tail*facts);
static inline literal trailPeek(funcsat*f);
static inline uintptr_t reason_info_mk(funcsat*f,clause*cls);

/*:78*//*81:*/
#line 1870 "funcsat.w"


literal funcsatMakeDecision(funcsat*,void*);


/*:81*//*84:*/
#line 1917 "funcsat.w"

static inline int activity_compare(double x,double y);


/*:84*//*90:*/
#line 2023 "funcsat.w"


static inline unsigned bh_var2pos(funcsat*f,variable v);
static inline bool bh_is_in_heap(funcsat*f,variable v);
static inline bool bh_node_is_in_heap(funcsat*f,struct bh_node*);
static inline double*bh_var2act(funcsat*f,variable v);
static inline struct bh_node*bh_top(funcsat*f);
static inline struct bh_node*bh_bottom(funcsat*f);
static inline bool bh_is_top(funcsat*f,struct bh_node*v);
static inline struct bh_node*bh_left(funcsat*f,struct bh_node*v);
static inline struct bh_node*bh_right(funcsat*f,struct bh_node*v);
static inline struct bh_node*bh_parent(funcsat*f,struct bh_node*v);
static inline unsigned bh_size(funcsat*f);
static inline variable bh_pop(funcsat*f);
static inline void bh_insert(funcsat*f,variable v);


/*:90*//*94:*/
#line 2128 "funcsat.w"

static inline void bh_increase_activity(funcsat*f,unsigned node_pos,double new_act);

/*:94*//*96:*/
#line 2158 "funcsat.w"

static void bh_check(funcsat*f);

/*:96*//*102:*/
#line 2230 "funcsat.w"

static void bh_print(funcsat*f,const char*path,struct bh_node*r);
/*:102*//*114:*/
#line 2645 "funcsat.w"

static void print_dot_impl_graph(funcsat*f,clause*cc);

/*:114*//*116:*/
#line 2722 "funcsat.w"

bool findUips(funcsat*f,unsigned c,head_tail*facts,literal*uipLit);
bool propagateFacts(funcsat*f,head_tail*facts,literal uipLit);

static unsigned resetLevelCount(funcsat*f,unsigned c,head_tail*facts);

static void checkSubsumption(
funcsat*f,
literal p,clause*learn,clause*reason,
bool learnIsUip);

/*:116*//*121:*/
#line 2865 "funcsat.w"

static inline mbool tentativeValue(funcsat*f,literal p);


/*:121*//*125:*/
#line 2893 "funcsat.w"

void minimizeUip(funcsat*f,clause*learned);












variable resolve(funcsat*f,literal p,clause*learn,clause*pReason);











variable resolveWithPos(funcsat*f,literal p,clause*learn,clause*pReason);


/*:125*//*127:*/
#line 3019 "funcsat.w"

static void noBumpOriginal(funcsat*f,clause*c);
void myDecayAfterConflict(funcsat*f);
void lbdDecayAfterConflict(funcsat*f);

/*:127*//*136:*/
#line 3309 "funcsat.w"

static void restore_facts(funcsat*f,head_tail*facts);

/*:136*//*141:*/
#line 3975 "funcsat.w"

extern mbool funcsatValue(funcsat*f,literal p);

/*:141*//*157:*/
#line 5413 "funcsat.w"


static void finishSolving(funcsat*func);
static bool bcpAndJail(funcsat*f);


bool funcsatLubyRestart(funcsat*f,void*p);
bool funcsatNoRestart(funcsat*,void*);
bool funcsatInnerOuter(funcsat*f,void*p);
bool funcsatMinisatRestart(funcsat*f,void*p);
void parseDimacsCnf(const char*path,funcsat*f);




void undoAssumptions(funcsat*func,clause*assumptions);

funcsat_result startSolving(funcsat*f);






funcsat_result addClause(funcsat*f,clause*c);






bool analyze_conflict(funcsat*func);










void backtrack(funcsat*func,variable newDecisionLevel,head_tail*facts,bool isRestart);






static void fs_print_state(funcsat*,FILE*);
void funcsatPrintConfig(FILE*f,funcsat*);


bool funcsatIsResourceLimitHit(funcsat*,void*);
funcsat_result funcsatPreprocessNewClause(funcsat*,void*,clause*);
funcsat_result funcsatPreprocessBeforeSolve(funcsat*,void*);
variable funcsatLearnClauses(funcsat*,void*);

int varOrderCompare(fibkey*,fibkey*);
double funcsatDefaultStaticActivity(variable*v);
void lbdBumpActivity(funcsat*f,clause*c);

void bumpReasonByActivity(funcsat*f,clause*c);
void bumpLearnedByActivity(funcsat*f,clause*c);
void bumpReasonByLbd(funcsat*f,clause*c);
void bumpLearnedByLbd(funcsat*f,clause*c);
void bumpOriginal(funcsat*f,clause*c);
void bumpUnitClauseByActivity(funcsat*f,clause*c);

void singlesPrint(FILE*stream,clause*begin);


bool watcherFind(clause*c,clause**watches,uint8_t w);
void watcherPrint(FILE*stream,clause*c,uint8_t w);
void singlesPrint(FILE*stream,clause*begin);
void binWatcherPrint(FILE*stream,funcsat*f);





bool isUnitClause(funcsat*f,clause*c);






literal levelOf(funcsat*f,variable v);

variable fs_lit2var(literal l);



literal fs_var2lit(variable v);


unsigned fs_lit2idx(literal l);

bool isDecision(funcsat*,variable);












void sortClause(clause*c);




literal findLiteral(literal l,clause*);



literal findVariable(variable l,clause*);

unsigned int fsLitHash(void*);
int fsLitEq(void*,void*);
int litCompare(const void*l1,const void*l2);
typedef struct litpos
{
clause*c;
posvec*ix;
}litpos;



/*:157*//*159:*/
#line 5615 "funcsat.w"

static inline bool hasLitPos(funcsat*f,literal l);
static inline clause*getReason(funcsat*f,literal l);
static inline void spliceUnitFact(funcsat*f,literal l,clause*c);
static inline unsigned getLitPos(funcsat*f,literal l);


/*:159*/
#line 276 "funcsat.w"


#endif

/*:12*/
