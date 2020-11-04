#define Unassigned (-1)  \

#define for_head_tail(ht,prev,curr,next)  \
for(prev= NULL,curr= (ht) ->hd,((curr) ?next= (curr) ->nx:0) ; \
(curr) ; \
((curr) ->nx?prev= (curr) :0) ,curr= (next) ,((curr) ?next= (curr) ->nx:0) )  \

#define head_tail_iter_rm(ht,prev,curr,next)  \
if((ht) ->hd==(curr) ) { \
(ht) ->hd= (next) ; \
} \
if((ht) ->tl==(curr) ) { \
(ht) ->tl= (prev) ; \
} \
if(prev) { \
(prev) ->nx= (next) ; \
} \
(curr) ->nx= NULL; \

#define watchlist_next_elt(elt,wl)  \
((elt) +1==(wl) ->elts+WATCHLIST_HEAD_SIZE_MAX?elt= (wl) ->rest:(elt) ++) 
#define for_watchlist(elt,dump,wl)  \
for(elt= dump= wl->elts; \
(watchlist_is_elt_in_head(elt,wl)  \
?elt<watchlist_head_size_ptr(wl)  \
:elt<watchlist_rest_size_ptr(wl) ) ; \
watchlist_next_elt(elt,wl) )  \

#define for_watchlist_continue(elt,dump,wl)  \
for(; \
(watchlist_is_elt_in_head(elt,wl)  \
?elt<watchlist_head_size_ptr(wl)  \
:elt<watchlist_rest_size_ptr(wl) ) ; \
watchlist_next_elt(elt,wl) )  \

#define NEWLINE(s) fprintf((!(s) ?stderr:(s) ) ,"\n")  \

#define NO_REASON UINTPTR_MAX \

#define reason_info_ptr(f,i) (&((f) ->reason_infos->data[i]) )  \

#define reason_info_idx(f,r) ((r) -(f) ->reason_infos->data)  \

#define fs_dbgout(f) (f) ->conf->debugStream \

#define LBD_SCORE_MAX 1000000
#define otherWatchIdx(watchIdx) ((watchIdx) ==0?1:0)  \

#define forEachWatchedClause(c,p,wi,nx,end,foundEnd)  \
if(c)  \
for(wi= (c->data[0]==p?0:1) ,end= c->prev[wi],nx= c->next[wi],foundEnd= false; \
!foundEnd; \
foundEnd= (c==end) ,c= nx,wi= (c->data[0]==p?0:1) ,nx= c->next[wi]) 
#define forEachClause(c,nx,end,foundEnd)  \
if(c)  \
for(end= c->prev[0],nx= c->next[0],foundEnd= false; \
!foundEnd; \
foundEnd= (c==end) ,c= nx,nx= c->next[0])  \
 \
 \
 \
 \
 \

/*14:*/
#line 295 "funcsat.w"

#include "funcsat/config.h"

#include <stdio.h> 
#include <stdlib.h> 
#include <stdarg.h> 
#include <assert.h> 
#include <string.h> 
#include <float.h> 
#include <math.h> 
#include <sys/resource.h> 
#include <sys/stat.h> 
#include <inttypes.h> 
#include <errno.h> 
#include <funcsat/hashtable.h> 
#include <funcsat/system.h> 
#include <zlib.h> 
#include <ctype.h> 


#include "funcsat.h"
#include "funcsat_internal.h"
#include "funcsat/vec_uintptr.h"

#define UNUSED(x) (void)(x)
/*16:*/
#line 366 "funcsat.w"

funcsat_result funcsatSolve(funcsat*f)
{
if(FS_UNSAT==(f->lastResult= startSolving(f)))goto Done;

if(!bcpAndJail(f))goto Unsat;

while(!f->conf->isResourceLimitHit(f,f->conf->user)){
if(!bcp(f)){
if(0==f->decisionLevel)goto Unsat;
if(!analyze_conflict(f))goto Unsat;
continue;
}

#if 0
if(f->conf->gc)f->conf->sweepClauses(f,f->conf->user);
#endif

if(f->trail.size!=f->numVars&&
f->conf->isTimeToRestart(f,f->conf->user)){
fslog(f,"solve",1,"restarting\n");
++f->numRestarts;
backtrack(f,0,NULL,true);
continue;
}

if(!funcsatMakeDecision(f,f->conf->user)){
f->lastResult= FS_SAT;
goto Done;
}
}

Unsat:
f->lastResult= FS_UNSAT;

Done:
fslog(f,"solve",1,"instance is %s\n",funcsatResultAsString(f->lastResult));
assert(f->lastResult!=FS_SAT||f->trail.size==f->numVars);
finishSolving(f);
return f->lastResult;
}

/*:16*//*19:*/
#line 603 "funcsat.w"

funcsat_result funcsatAddClause(funcsat*f,clause*c)
{
funcsatReset(f);
variable k;
variable maxVar= 0;

for(k= 0;k<c->size;k++){
variable v= fs_lit2var(c->data[k]);
maxVar= v> maxVar?v:maxVar;
}
funcsatResize(f,maxVar);

if(c->size> 1){
unsigned size= c->size;
sortClause(c);
literal*i,*j,*end;

for(i= j= (literal*)c->data,end= i+c->size;i!=end;i++){
literal p= *i,q= *j;
if(i!=j){
if(p==q){
size--;
continue;
}else if(p==-q){
clauseDestroy(c);
goto Done;
}else*(++j)= p;
}
}
c->size= size;
}

fs_ifdbg(f,"solve",2){
fslog(f,"solve",2,"original clause: ");
dimacsPrintClause(fs_dbgout(f),c);
fprintf(fs_dbgout(f),"\n");
}

c->lbdScore= LBD_SCORE_MAX;
c->isLearnt= false;
c->isReason= false;
f->conf->bumpOriginal(f,c);
funcsat_result clauseResult= addClause(f,c);
if(f->lastResult!=FS_UNSAT){
f->lastResult= clauseResult;
}
vectorPush(&f->origClauses,c);
++f->numOrigClauses;
Done:
return f->lastResult;
}

funcsat_result addClause(funcsat*f,clause*c)
{
funcsat_result result= FS_UNKNOWN;
assert(f->decisionLevel==0);
c->isLearnt= false;
if(c->size==0){
f->conflictClause= c;
result= FS_UNSAT;
}else if(c->size==1){
mbool val= funcsatValue(f,c->data[0]);
if(val==false){
f->conflictClause= c;
result= FS_UNSAT;
}else{
if(val==unknown){
trailPush(f,c->data[0],reason_info_mk(f,c));
head_tail_add(&f->unit_facts[fs_lit2var(c->data[0])],c);
}
}
}else addWatch(f,c);
return result;
}


/*:19*//*20:*/
#line 708 "funcsat.w"

funcsat_result funcsatPushAssumption(funcsat*f,literal p)
{
if(p==0){

clausePush(&f->assumptions,0);
return FS_UNKNOWN;
}

f->conf->minimizeLearnedClauses= false;
backtrack(f,0UL,NULL,true);

funcsatResize(f,fs_lit2var(p));
if(funcsatValue(f,p)==false){
return FS_UNSAT;
}else if(funcsatValue(f,p)==unknown){

clausePush(&f->assumptions,p);
trailPush(f,p,NO_REASON);
}else{
clausePush(&f->assumptions,0);
}
return FS_UNKNOWN;
}

funcsat_result funcsatPushAssumptions(funcsat*f,clause*c){
for(unsigned i= 0;i<c->size;i++){
if(funcsatPushAssumption(f,c->data[i])==FS_UNSAT){
funcsatPopAssumptions(f,i);
return FS_UNSAT;
}
}
return FS_UNKNOWN;
}

/*:20*//*21:*/
#line 745 "funcsat.w"

void funcsatPopAssumptions(funcsat*f,unsigned num){

head_tail facts;
head_tail_clear(&facts);

assert(num<=f->assumptions.size);

backtrack(f,0,NULL,true);

for(unsigned i= 0;i<num;i++){
literal p= clausePop(&f->assumptions);
if(p==0)return;

literal t= trailPop(f,&facts);

while(p!=t){
t= trailPop(f,&facts);
}
}

restore_facts(f,&facts);
}

/*:21*//*22:*/
#line 772 "funcsat.w"


void funcsatReset(funcsat*f)
{
f->conflictClause= NULL;
backtrack(f,0UL,NULL,true);
f->propq= 0;
f->lastResult= FS_UNKNOWN;
}





/*:22*//*27:*/
#line 872 "funcsat.w"

static inline void head_tail_clear(head_tail*ht){
ht->hd= ht->tl= NULL;
}

static inline bool head_tail_is_empty(head_tail*ht){
return ht->hd==NULL&&ht->tl==NULL;
}

static inline void head_tail_append(head_tail*ht1,head_tail*ht2){

if(!head_tail_is_empty(ht2)){
if(head_tail_is_empty(ht1)){
ht1->hd= ht2->hd;
}else{
assert(ht1->hd);
assert(!ht1->tl->nx);
ht1->tl->nx= ht2->hd;
}
ht1->tl= ht2->tl;
}
}


static inline void head_tail_add(head_tail*ht1,clause*c){

(c)->nx= ht1->hd;
ht1->hd= (c);
if(!ht1->tl){
ht1->tl= (c);}
}

/*:27*//*30:*/
#line 944 "funcsat.w"

inline void head_tail_print(funcsat*f,FILE*out,head_tail*l)
{
if(!out)out= stderr;
if(l->hd){
clause*p,*c,*nx;
for_head_tail(l,p,c,nx){
if(!c->nx&&l->tl!=c){
fprintf(out,"warning: tl is not last clause\n");
}
fs_clause_print(f,out,c);
fprintf(out,"\n");
}
}else if(l->tl){
fprintf(out,"warning: hd unset but tl set!\n");
}
}

/*:30*//*33:*/
#line 983 "funcsat.w"

bool bcp(funcsat*f)
{
bool isConsistent= true;

/*50:*/
#line 1283 "funcsat.w"

while(f->propq<f->trail.size){
literal p= f->trail.data[f->propq];
#ifndef NDEBUG
watchlist_check(f,p);
#endif
fs_ifdbg(f,"bcp",3){fslog(f,"bcp",3,"bcp on %i\n",p);}
const literal false_lit= -p;

struct watchlist*wl= &f->watches.wlist[fs_lit2idx(p)];
struct watchlist_elt*elt,*dump;
uint32_t new_size= 0;

++f->numProps;
dopen(f,"bcp");

for_watchlist(elt,dump,wl){
clause*c= elt->cls;
literal otherlit;
mbool litval;
fs_ifdbg(f,"bcp",9){
fslog(f,"bcp",9,"visit ");
fs_clause_print(f,fs_dbgout(f),c);
fprintf(fs_dbgout(f)," %p\n",c);
}
assert(false_lit==c->data[0]||false_lit==c->data[1]);
assert(!c->nx);
/*51:*/
#line 1336 "funcsat.w"

if(funcsatValue(f,elt->lit)==true)goto watch_continue;



/*:51*/
#line 1310 "funcsat.w"

/*52:*/
#line 1344 "funcsat.w"

if(c->data[0]==false_lit){
literal tmp= c->data[0];
elt->lit= c->data[0]= c->data[1],c->data[1]= tmp;
assert(c->data[1]==false_lit);
fs_ifdbg(f,"bcp",10){
fslog(f,"bcp",9,"swapped ");
fs_clause_print(f,fs_dbgout(f),c),fprintf(fs_dbgout(f),"\n");}
}
assert(c->data[1]==false_lit);


/*:52*/
#line 1311 "funcsat.w"

/*53:*/
#line 1361 "funcsat.w"

for(literal*l= c->data+2;l!=c->data+c->size;l++){
mbool v= funcsatValue(f,*l);
if(v!=false){
c->data[1]= *l,*l= false_lit;
watch_l1(f,c);
fs_ifdbg(f,"bcp",9){
fslog(f,"bcp",9,"moved ");
fs_clause_print(f,fs_dbgout(f),c),NEWLINE(fs_dbgout(f));
}
elt->cls= NULL;elt->lit= 0;
goto skip_watchelt_copy;
}
}

/*:53*/
#line 1312 "funcsat.w"


otherlit= c->data[0];
litval= funcsatValue(f,otherlit);
if(litval==true)goto watch_continue;
if(litval==false){
/*54:*/
#line 1392 "funcsat.w"

isConsistent= false;
f->conflictClause= c;
for_watchlist_continue(elt,dump,wl){
*dump= *elt,watchlist_next_elt(dump,wl);
new_size++;
}
wl->size= new_size;
dclose(f,"bcp");
goto bcp_conflict;

/*:54*/
#line 1318 "funcsat.w"

}else{
/*55:*/
#line 1405 "funcsat.w"

fslog(f,"bcp",2," => %i (%s:%d)\n",otherlit,__FILE__,__LINE__);
trailPush(f,otherlit,reason_info_mk(f,c));
f->conf->bumpUnitClause(f,c);

/*:55*/
#line 1320 "funcsat.w"

}
watch_continue:
*dump= *elt,watchlist_next_elt(dump,wl);
new_size++;
skip_watchelt_copy:;
}
f->propq++;
wl->size= new_size;
dclose(f,"bcp");
}

/*:50*/
#line 988 "funcsat.w"


bcp_conflict:
return isConsistent;
}

/*:33*//*38:*/
#line 1096 "funcsat.w"

static inline void all_watches_init(funcsat*f)
{
CallocX(f->watches.wlist,1<<7,sizeof(*f->watches.wlist));
f->watches.size= 2;
f->watches.capacity= 1<<7;
}

/*:38*//*39:*/
#line 1106 "funcsat.w"

static inline void all_watches_destroy(funcsat*f)
{
free(f->watches.wlist);
}

/*:39*//*45:*/
#line 1187 "funcsat.w"

static int compare_pointer(const void*x,const void*y)
{
uintptr_t xp= (uintptr_t)*(clause**)x;
uintptr_t yp= (uintptr_t)*(clause**)y;
if(xp<yp)return-1;
else if(xp> yp)return 1;
else return 0;
}


/*:45*//*47:*/
#line 1204 "funcsat.w"

static inline void watchlist_check(funcsat*f,literal l)
{
literal false_lit= -l;

struct watchlist*wl= &f->watches.wlist[fs_lit2idx(l)];
struct watchlist_elt*elt,*dump;

vector*clauses= vectorAlloc(wl->size);
for_watchlist(elt,dump,wl){
clause*c= elt->cls;
vectorPush(clauses,c);
literal*chkl;bool chk_in_cls= false;
for_clause(chkl,c){
if(*chkl==elt->lit){
chk_in_cls= true;
break;
}
}
assert(chk_in_cls&&"watched lit not in clause");

assert((c->data[0]==false_lit||c->data[1]==false_lit)&&
"watched lit not in first 2");

uint32_t num_not_false= 0;
for(unsigned i= 0;i<c->size;i++){
if(tentativeValue(f,c->data[i])!=false){
num_not_false++;
if(funcsatValue(f,c->data[i])==true){
num_not_false= 0;
break;
}
}
}
if(num_not_false>=1){
assert((tentativeValue(f,c->data[0])!=false||
tentativeValue(f,c->data[1])!=false)&&
"watching bad literals");
if(num_not_false>=2){
assert(tentativeValue(f,c->data[0])!=false&&
tentativeValue(f,c->data[1])!=false&&
"watching bad literals");
}
}
}

qsort(clauses->data,clauses->size,sizeof(clause*),compare_pointer);

for(unsigned i= 0,j= 1;j<clauses->size;i++,j++){

assert(clauses->data[i]!=clauses->data[j]&&"duplicate clause");
}
vectorDestroy(clauses),free(clauses);
}



/*:47*//*48:*/
#line 1262 "funcsat.w"


static inline void watches_check(funcsat*f)
{
for(variable v= 1;v<=f->numVars;v++){
literal pos= (literal)v;
literal neg= -(literal)v;
watchlist_check(f,pos);
watchlist_check(f,neg);
}
}

/*:48*//*56:*/
#line 1412 "funcsat.w"

static inline struct watchlist_elt*watch_lit(funcsat*f,struct watchlist*wl,clause*c)
{
UNUSED(f);
struct watchlist_elt*ret;
if(watchlist_head_size(wl)<WATCHLIST_HEAD_SIZE_MAX){
assert(watchlist_rest_size(wl)==0);
ret= &wl->elts[watchlist_head_size(wl)];
wl->elts[watchlist_head_size(wl)].cls= c;
}else{
assert(watchlist_head_size(wl)>=WATCHLIST_HEAD_SIZE_MAX);
/*57:*/
#line 1445 "funcsat.w"

if(wl->capacity> 0){
if(watchlist_rest_size(wl)>=wl->capacity){
ReallocX(wl->rest,wl->capacity*2,sizeof(*wl->rest));
wl->capacity*= 2;
}
}else{
CallocX(wl->rest,8,sizeof(*wl->rest));
wl->capacity= 8;
}

/*:57*/
#line 1423 "funcsat.w"

ret= &wl->rest[watchlist_rest_size(wl)];
wl->rest[watchlist_rest_size(wl)].cls= c;
}
wl->size++;
return ret;
}
static inline void watch_l0(funcsat*f,clause*c)
{
struct watchlist*wl= &f->watches.wlist[fs_lit2idx(-c->data[0])];
struct watchlist_elt*elt= watch_lit(f,wl,c);
elt->lit= (c->size==2?c->data[1]:c->data[0]);
}
static inline void watch_l1(funcsat*f,clause*c)
{
struct watchlist*wl= &f->watches.wlist[fs_lit2idx(-c->data[1])];
struct watchlist_elt*elt= watch_lit(f,wl,c);
elt->lit= (c->size==2?c->data[1]:c->data[0]);
}

/*:56*//*59:*/
#line 1468 "funcsat.w"

static inline void addWatch(funcsat*f,clause*c)
{
makeWatchable(f,c);
addWatchUnchecked(f,c);
}

static inline void addWatchUnchecked(funcsat*f,clause*c)
{
fslog(f,"bcp",1,"watching %li and %li in ",c->data[0],c->data[1]);
fs_ifdbg(f,"bcp",1){fs_clause_print(f,fs_dbgout(f),c),NEWLINE(fs_dbgout(f));}
assert(c->size> 1);
watch_l0(f,c);
watch_l1(f,c);
c->is_watched= true;
}

/*:59*//*60:*/
#line 1489 "funcsat.w"

static inline void makeWatchable(funcsat*f,clause*c)
{
for(variable i= 0,j= 0;i<c->size&&j<2;i++){
mbool v= funcsatValue(f,c->data[i]);
if(v!=false&&i!=j){
literal tmp= c->data[j];
c->data[j]= c->data[i],c->data[i]= tmp;
j++;
}
}
}

/*:60*//*62:*/
#line 1511 "funcsat.w"

static void fs_watches_print(funcsat*f,FILE*out,literal p)
{
if(!out)out= stderr;
fprintf(out,"watcher list for %i:\n",p);
struct watchlist*wl= &f->watches.wlist[fs_lit2idx(p)];
struct watchlist_elt*elt,*dump;
for_watchlist(elt,dump,wl){
struct clause*c= elt->cls;
if(c){
fprintf(out,"[%i",elt->lit);
bool in_clause= false;
literal*q;
for_clause(q,c){
if(*q==elt->lit){
in_clause= true;break;
}
}
if(!in_clause){
fprintf(out," oh noes!");
}
fprintf(out,"] ");
fs_clause_print(f,out,c),NEWLINE(out);
}else{
fprintf(out,"[EMPTY SLOT]\n");
}
}
}


/*:62*//*65:*/
#line 1568 "funcsat.w"

static inline clause*getReason(funcsat*f,literal l)
{
uintptr_t reason_idx= f->reason->data[fs_lit2var(l)];
if(reason_idx!=NO_REASON){
struct reason_info*r= reason_info_ptr(f,reason_idx);
if(r->ty==REASON_CLS_TY||r->cls)return r->cls;
clause*(*get_reason_clause)(funcsat*,literal)= f->reason_hooks.data[r->ty];
return r->cls= get_reason_clause(f,l);
}else
return NULL;
}


/*:65*//*67:*/
#line 1590 "funcsat.w"

void funcsatAddReasonHook(funcsat*f,uintptr_t ty,clause*(*hook)(funcsat*f,literal l))
{
vectorPushAt(&f->reason_hooks,hook,ty);
}

/*:67*//*69:*/
#line 1606 "funcsat.w"

static inline uintptr_t reason_info_mk(funcsat*f,clause*cls)
{
assert(f->reason_infos_freelist<f->reason_infos->size);
uintptr_t ret= f->reason_infos_freelist;
struct reason_info*r= reason_info_ptr(f,ret);
f->reason_infos_freelist= r->ty;

r->ty= REASON_CLS_TY;
r->cls= cls;
return ret;
}

/*:69*//*70:*/
#line 1624 "funcsat.w"

static inline void reason_info_release(funcsat*f,uintptr_t ri)
{
struct reason_info*r= reason_info_ptr(f,ri);
r->ty= f->reason_infos_freelist;
f->reason_infos_freelist= ri;
}

/*:70*//*75:*/
#line 1662 "funcsat.w"

void trailPush(funcsat*f,literal p,uintptr_t reason_info_idx)
{
variable v= fs_lit2var(p);
#ifndef NDEBUG
if(f->model.data[v]<f->trail.size){
assert(f->trail.data[f->model.data[v]]!=p);
}
#endif
clausePush(&f->trail,p);
f->model.data[v]= f->trail.size-(unsigned)1;
f->phase.data[v]= p;
f->level.data[v]= (literal)f->decisionLevel;

if(reason_info_idx!=NO_REASON){
struct reason_info*r= reason_info_ptr(f,reason_info_idx);
if(r->ty==REASON_CLS_TY){
r->cls->isReason= true;
}
f->reason->data[v]= reason_info_idx;
}
}

/*:75*//*76:*/
#line 1692 "funcsat.w"

literal trailPop(funcsat*f,head_tail*facts)
{
literal p= clausePop(&f->trail);
variable v= fs_lit2var(p);
uintptr_t reason_ix;
if(f->unit_facts[v].hd&&facts){
head_tail_append(facts,&f->unit_facts[v]);
head_tail_clear(&f->unit_facts[v]);
}

#if 0
exonerateClauses(f,v);
#endif

if(f->decisions.data[v]!=0){
f->decisionLevel--;
f->decisions.data[v]= 0;
}


f->level.data[v]= Unassigned;
reason_ix= f->reason->data[v];
if(reason_ix!=NO_REASON){
struct reason_info*r= reason_info_ptr(f,reason_ix);
if(r->ty==REASON_CLS_TY){
r->cls->isReason= false;
}
reason_info_release(f,reason_ix);
f->reason->data[v]= NO_REASON;
}
if(f->propq>=f->trail.size){
f->propq= f->trail.size;
}
if(!bh_is_in_heap(f,v)){
#ifndef NDEBUG
bh_check(f);
#endif
bh_insert(f,v);
}
return p;
}

/*:76*//*77:*/
#line 1737 "funcsat.w"

static inline literal trailPeek(funcsat*f)
{
literal p= clausePeek(&f->trail);
return p;
}

/*:77*//*79:*/
#line 1765 "funcsat.w"

static void jailClause(funcsat*f,literal trueLit,clause*c)
{
UNUSED(f),UNUSED(trueLit),UNUSED(c);
#if 0
++f->numJails;
dopen(f,"jail");
assert(funcsatValue(f,trueLit)==true);
const variable trueVar= fs_lit2var(trueLit);
dmsg(f,"jail",7,false,"jailed for %u: ",trueVar);
clauseUnSpliceWatch((clause**)&watches->data[fs_lit2idx(-c->data[0])],c,0);
clause*cell= &f->jail.data[trueVar];
clauseSplice1(c,&cell);
dclose(f,"jail");
assert(!c->is_watched);
#endif
}

static void exonerateClauses(funcsat*f,variable v)
{
clause*p,*c,*nx;
for_head_tail(&f->jail[v],p,c,nx){
c->nx= NULL;
addWatchUnchecked(f,c);
}
head_tail tmp;
head_tail_clear(&tmp);
memcpy(&f->jail[v],&tmp,sizeof(tmp));
}

/*:79*//*80:*/
#line 1841 "funcsat.w"

literal funcsatMakeDecision(funcsat*f,void*p)
{
UNUSED(p);
literal l= 0;
while(bh_size(f)> 0){
fslog(f,"decide",5,"extracting\n");
variable v= bh_pop(f);
#ifndef NDEBUG
bh_check(f);
#endif
fslog(f,"decide",5,"extracted %u\n",v);
literal p= -fs_var2lit(v);
if(funcsatValue(f,p)==unknown){
if(f->conf->usePhaseSaving)l= f->phase.data[v];
else l= p;
++f->numDecisions,f->pctdepth/= 2.0,f->correction= 1;
trailPush(f,l,NO_REASON);
f->level.data[v]= (int)++f->decisionLevel;
f->decisions.data[v]= f->decisionLevel;
fslog(f,"solve",2,"branched on %i\n",l);
break;
}
}
assert(l!=0||bh_size(f)==0);
return l;
}

/*:80*//*82:*/
#line 1879 "funcsat.w"

void varBumpScore(funcsat*f,variable v)
{
double*activity_v= bh_var2act(f,v);
double origActivity,activity;
origActivity= activity= *activity_v;
if((activity+= f->varInc)> 1e100){
for(variable j= 1;j<=f->numVars;j++){
double*m= bh_var2act(f,j);
fslog(f,"decide",5,"old activity %f, rescaling\n",*m);
*m*= 1e-100;
}
double oldVarInc= f->varInc;
f->varInc*= 1e-100;
fslog(f,"decide",1,"setting varInc from %f to %f\n",oldVarInc,f->varInc);
activity*= 1e-100;
}
if(bh_is_in_heap(f,v)){
bh_increase_activity(f,v,activity);
}else{
*activity_v= activity;
}
fslog(f,"decide",5,"bumped %u from %.30f to %.30f\n",v,origActivity,*activity_v);
}

/*:82*//*83:*/
#line 1908 "funcsat.w"

static inline int activity_compare(double x,double y)
{
if(x> y)return 1;
else if(x<y)return-1;
else return 0;
}

/*:83*//*86:*/
#line 1951 "funcsat.w"

static inline unsigned bh_var2pos(funcsat*f,variable v)
{
return f->binvar_pos[v];
}
static inline bool bh_is_in_heap(funcsat*f,variable v)
{
assert(bh_var2pos(f,v)> 0);
return bh_var2pos(f,v)<=f->binvar_heap_size;
}
static inline bool bh_node_is_in_heap(funcsat*f,struct bh_node*n)
{
assert(n>=f->binvar_heap);
return(unsigned)(n-f->binvar_heap)<=f->binvar_heap_size;
}

/*:86*//*87:*/
#line 1970 "funcsat.w"


static inline double*bh_var2act(funcsat*f,variable v)
{
return&f->binvar_heap[bh_var2pos(f,v)].act;
}

/*:87*//*88:*/
#line 1981 "funcsat.w"

static inline struct bh_node*bh_top(funcsat*f)
{
return f->binvar_heap+1;
}
static inline struct bh_node*bh_bottom(funcsat*f)
{
return f->binvar_heap+f->binvar_heap_size;
}
static inline bool bh_is_top(funcsat*f,struct bh_node*v)
{
return bh_top(f)==v;
}
static inline struct bh_node*bh_left(funcsat*f,struct bh_node*v)
{
return f->binvar_heap+(2*(v-f->binvar_heap));
}
static inline struct bh_node*bh_right(funcsat*f,struct bh_node*v)
{
return f->binvar_heap+(2*(v-f->binvar_heap)+1);
}
static inline struct bh_node*bh_parent(funcsat*f,struct bh_node*v)
{
return f->binvar_heap+((v-f->binvar_heap)/2);
}
static inline unsigned bh_size(funcsat*f)
{
return f->binvar_heap_size;
}

/*:88*//*89:*/
#line 2015 "funcsat.w"

static inline struct bh_node*bh_node_get(funcsat*f,variable v)
{
return f->binvar_heap+f->binvar_pos[v];
}


/*:89*//*91:*/
#line 2047 "funcsat.w"

static inline void bh_swap(funcsat*f,struct bh_node**x,struct bh_node**y)
{
struct bh_node tmp= **x,*tmpp= *x;
**x= **y,**y= tmp;
*x= *y,*y= tmpp;
f->binvar_pos[(*x)->var]= *x-f->binvar_heap;
f->binvar_pos[(*y)->var]= *y-f->binvar_heap;
}

static inline void bh_bubble_up(funcsat*f,struct bh_node*e)
{
while(!bh_is_top(f,e)){
struct bh_node*p= bh_parent(f,e);
if(activity_compare(p->act,e->act)<0){
bh_swap(f,&p,&e);
}else
break;
}
}

static inline void bh_insert(funcsat*f,variable v)
{
assert(bh_size(f)+1<=f->numVars);
assert(bh_var2pos(f,v)> f->binvar_heap_size);
struct bh_node*node= &f->binvar_heap[bh_var2pos(f,v)];
assert(node->var==v);
f->binvar_heap_size++;
struct bh_node*last= &f->binvar_heap[f->binvar_heap_size];
bh_swap(f,&node,&last);
bh_bubble_up(f,node);
assert(f->binvar_heap[bh_var2pos(f,v)].var==v);
}


/*:91*//*92:*/
#line 2085 "funcsat.w"

static inline void bh_bubble_down(funcsat*f,struct bh_node*e)
{
struct bh_node*l,*r;
goto bh_bd_begin;
while(bh_node_is_in_heap(f,l)){
if(bh_node_is_in_heap(f,r)){
if(activity_compare(l->act,r->act)<0)
l= r;
}
if(activity_compare(e->act,l->act)<0){
bh_swap(f,&e,&l);
}else
break;
bh_bd_begin:
l= bh_left(f,e),r= bh_right(f,e);
}
}

static inline variable bh_pop(funcsat*f)
{
assert(f->binvar_heap_size> 0);
struct bh_node*top= bh_top(f);
struct bh_node*bot= bh_bottom(f);
bh_swap(f,&top,&bot);
f->binvar_heap_size--;
bh_bubble_down(f,bh_top(f));
return top->var;
}

/*:92*//*93:*/
#line 2116 "funcsat.w"

static inline void bh_increase_activity(funcsat*f,variable v,double act_new)
{
double*act_curr= bh_var2act(f,v);
struct bh_node*n= bh_node_get(f,v);
assert(n->var==v);
assert(*act_curr<=act_new);
*act_curr= act_new;
bh_bubble_up(f,n);
}

/*:93*//*95:*/
#line 2132 "funcsat.w"

static void bh_check_node(funcsat*f,struct bh_node*x)
{
struct bh_node*l= bh_left(f,x),*r= bh_right(f,x);
if(bh_node_is_in_heap(f,l)){
assert(activity_compare(l->act,x->act)<=0);
bh_check_node(f,l);
}
if(bh_node_is_in_heap(f,r)){
assert(activity_compare(r->act,x->act)<=0);
bh_check_node(f,r);
}
}

static void bh_check(funcsat*f)
{
struct bh_node*root= bh_top(f);
if(bh_node_is_in_heap(f,root)){
bh_check_node(f,root);
}
for(unsigned i= 1;i<f->numVars;i++){
assert(bh_node_get(f,i)->var==i);
}
}

/*:95*//*101:*/
#line 2191 "funcsat.w"

static void bh_padding(funcsat*f,const char*s,int x)
{
while(x--> 0){
fprintf(fs_dbgout(f),"%s",s);
}
}

static bool bh_print_levels(funcsat*f,FILE*dotfile,struct bh_node*r,int level)
{
assert(r);
if(bh_node_is_in_heap(f,r)){
bool lf,ri;
lf= bh_print_levels(f,dotfile,bh_left(f,r),level+1);
ri= bh_print_levels(f,dotfile,bh_right(f,r),level+1);
if(lf)fprintf(dotfile,"%u -> %u [label=\"L\"];\n",r->var,bh_left(f,r)->var);
if(ri)fprintf(dotfile,"%u -> %u [label=\"R\"];\n",r->var,bh_right(f,r)->var);
fprintf(dotfile,"%u [label=\"%u%s, %.1f\"];\n",r->var,r->var,
(funcsatValue(f,r->var)==true?"T":
(funcsatValue(f,r->var)==false?"F":"?")),r->act);
return true;
}else{
return false;
}
}

static void bh_print(funcsat*f,const char*path,struct bh_node*r)
{
if(!r)r= bh_top(f);
FILE*dotfile;
if(NULL==(dotfile= fopen(path,"w")))perror("fopen"),exit(1);
fprintf(dotfile,"digraph G {\n");
bh_print_levels(f,dotfile,r,0);
fprintf(dotfile,"}\n");
if(0!=fclose(dotfile))perror("fclose");
fprintf(fs_dbgout(f),"\n");
}

/*:101*//*103:*/
#line 2245 "funcsat.w"

bool analyze_conflict(funcsat*f)
{
++f->numConflicts;
#ifndef NDEBUG
#if 0
print_dot_impl_graph(f,f->conflictClause);
#endif
#endif

f->uipClause.size= 0;
clauseCopy(&f->uipClause,f->conflictClause);

head_tail facts;
head_tail_clear(&facts);
literal uipLit= 0;
variable i,c= 0;
literal*it;



for(i= 0,c= 0;i<f->uipClause.size;i++){
variable v= fs_lit2var(f->uipClause.data[i]);
f->litPos.data[v]= i;
if((unsigned)levelOf(f,v)==f->decisionLevel)c++;
}
int64_t entrydl= (int64_t)f->decisionLevel;


#ifndef NDEBUG
watches_check(f);
#endif
bool isUnsat= findUips(f,c,&facts,&uipLit);

if(entrydl<=(int64_t)f->decisionLevel+(int64_t)f->correction)f->pct+= f->pctdepth;
for(int64_t i= (int64_t)entrydl;
i> (int64_t)f->decisionLevel+(int64_t)f->correction;
i--){
f->pct+= f->pctdepth;
f->pctdepth*= 2.0;
}
f->correction= 0;
for(i= 0;i<f->uipClause.size;i++){
variable v= fs_lit2var(f->uipClause.data[i]);
f->litPos.data[v]= UINT_MAX;
}
if(isUnsat){
fslog(f,"solve",1,"findUips returned isUnsat\n");
f->conflictClause= NULL;
clause*prev,*curr,*next;
for_head_tail(&facts,prev,curr,next){
clauseDestroy(curr);
}
return false;
}

f->conf->decayAfterConflict(f);
f->conflictClause= NULL;
#ifndef NDEBUG
watches_check(f);
#endif
if(!propagateFacts(f,&facts,uipLit)){
return analyze_conflict(f);
}else{
return true;
}
}

/*:103*//*104:*/
#line 2316 "funcsat.w"

bool findUips(funcsat*f,unsigned c,head_tail*facts,literal*uipLit)
{
bool more= true;
uint32_t numUipsLearned= 0;
literal p;
fs_ifdbg(f,"findUips",1){
fslog(f,"findUips",1,"conflict@%u (#%"PRIu64") with ",
f->decisionLevel,f->numConflicts);
fs_clause_print(f,fs_dbgout(f),&f->uipClause);
fprintf(fs_dbgout(f),"\n");
}
dopen(f,"findUips");
do{
clause*reason= NULL;
do{

/*105:*/
#line 2380 "funcsat.w"

while(!hasLitPos(f,p= trailPeek(f))){
trailPop(f,facts);
}

/*:105*/
#line 2333 "funcsat.w"

reason= getReason(f,p);
/*106:*/
#line 2388 "funcsat.w"

fs_ifdbg(f,"findUips",6){
fslog(f,"findUips",6,"resolving ");
fs_clause_print(f,fs_dbgout(f),&f->uipClause);
fprintf(fs_dbgout(f)," with ");
fs_clause_print(f,fs_dbgout(f),reason);
fprintf(fs_dbgout(f),"\n");
}
trailPop(f,facts);
assert(reason);
f->conf->bumpReason(f,reason);
variable num= resolveWithPos(f,p,&f->uipClause,reason);
if(f->uipClause.size==0)return true;

/*:106*/
#line 2335 "funcsat.w"

/*107:*/
#line 2406 "funcsat.w"

c= c-1+num;
c= resetLevelCount(f,c,facts);


/*:107*/
#line 2336 "funcsat.w"

if(f->conf->useSelfSubsumingResolution){


checkSubsumption(f,p,&f->uipClause,reason,!(c> 1));
}
}while(c> 1);
++numUipsLearned;
assert(c==1||f->decisionLevel==0);

/*108:*/
#line 2415 "funcsat.w"

if(f->decisionLevel==0)return true;

/*:108*/
#line 2346 "funcsat.w"




p= trailPeek(f);
while(!hasLitPos(f,p)){
trailPop(f,facts);
p= trailPeek(f);
}
more= getReason(f,p)!=NULL;
assert(f->decisionLevel!=VARIABLE_MAX);


/*109:*/
#line 2422 "funcsat.w"


clause*newUip= clauseAlloc((&f->uipClause)->size);

clauseCopy(newUip,&f->uipClause);
fs_ifdbg(f,"findUips",5){
fslog(f,"findUips",5,"found raw UIP: ");
fs_clause_print(f,fs_dbgout(f),newUip);
fprintf(fs_dbgout(f),"\n");
}
newUip->isLearnt= true;

spliceUnitFact(f,p,newUip);
vectorPush(&f->learnedClauses,newUip);
++f->numLearnedClauses;

/*110:*/
#line 2453 "funcsat.w"

literal watch2= 0,watch2_level= -1;
unsigned pPos= getLitPos(f,p);
literal tmp= newUip->data[0];
newUip->data[0]= -p;
newUip->data[pPos]= tmp;
if(f->conf->minimizeLearnedClauses)minimizeUip(f,newUip);
for(variable i= 1;i<newUip->size;i++){
literal lev= levelOf(f,fs_lit2var(newUip->data[i]));
if(watch2_level<lev){
watch2_level= lev;
watch2= (literal)i;
}
if(lev==(literal)f->decisionLevel)break;
}
if(watch2_level!=-1){
tmp= newUip->data[1];
newUip->data[1]= newUip->data[watch2];
newUip->data[watch2]= tmp;
}


/*:110*/
#line 2438 "funcsat.w"

f->conf->bumpLearned(f,newUip);
fs_ifdbg(f,"findUips",5){
fslog(f,"findUips",5,"found min UIP: ");
fs_clause_print(f,fs_dbgout(f),newUip);
fprintf(fs_dbgout(f),"\n");
}

/*:109*/
#line 2359 "funcsat.w"

}while(more&&numUipsLearned<f->conf->numUipsToLearn);
bool foundDecision= isDecision(f,fs_lit2var(p));
*uipLit= -trailPop(f,facts);



while(!foundDecision){
foundDecision= isDecision(f,fs_lit2var(trailPeek(f)));
trailPop(f,facts);
}
fslog(f,"findUips",5,"learned %"PRIu32" UIPs\n",numUipsLearned);
dclose(f,"findUips");
fslog(f,"findUips",5,"done\n");

/*112:*/
#line 2506 "funcsat.w"

#if 0
if(f->conf->useSelfSubsumingResolution){
clause**it;
forVector(clause**,it,&f->subsumed){
bool subsumedByUip= (bool)*it;
++it;

fs_ifdbg(f,"subsumption",1){
fprintf(dbgout(f),"removing clause due to subsumption: ");
dimacsPrintClause(dbgout(f),*it);
fprintf(dbgout(f),"\n");
}

if((*it)->isLearnt){
clause*removedClause= funcsatRemoveClause(f,*it);
assert(removedClause);
if(subsumedByUip)clauseFree(removedClause);
else{
removedClause->data[0]= removedClause->data[--removedClause->size];
vectorPush(&f->learnedClauses,removedClause);
fs_ifdbg(f,"subsumption",1){
fprintf(dbgout(f),"new adjusted clause: ");
dimacsPrintClause(dbgout(f),removedClause);
fprintf(dbgout(f),"\n");
}
if(removedClause->is_watched){
addWatch(f,removedClause);
}
}
}
}
f->subsumed.size= 0;
}
#endif

/*:112*/
#line 2374 "funcsat.w"


return f->uipClause.size==0||c==0;
}

/*:104*//*111:*/
#line 2478 "funcsat.w"

static unsigned resetLevelCount(funcsat*f,unsigned c,head_tail*facts)
{



while(c==0&&f->decisionLevel!=0){
literal p= trailPeek(f);
if(hasLitPos(f,p)){


variable i;
for(i= 0,c= 0;i<f->uipClause.size;i++){
if((unsigned)levelOf(f,fs_lit2var(f->uipClause.data[i]))==f->decisionLevel){
c++;
}
}
}else trailPop(f,facts);
}
return c;
}



/*:111*//*113:*/
#line 2543 "funcsat.w"

static void cleanSeen(funcsat*f,variable top)
{
while(f->analyseToClear.size> top){
variable v= fs_lit2var(intvecPop(&f->analyseToClear));
f->seen.data[v]= false;
}
}

static bool isAssumption(funcsat*f,variable v)
{
literal*it;
forClause(it,&f->assumptions){
if(fs_lit2var(*it)==v){
return true;
}
}
return false;
}




static bool litRedundant(funcsat*f,literal q0)
{
if(levelOf(f,fs_lit2var(q0))==0)return false;
if(!getReason(f,q0))return false;
assert(!isAssumption(f,fs_lit2var(q0)));

posvecClear(&f->analyseStack);
posvecPush(&f->analyseStack,fs_lit2var(q0));
variable top= f->analyseToClear.size;

while(f->analyseStack.size> 0){
variable p= posvecPop(&f->analyseStack);
clause*c= getReason(f,(literal)p);
variable i;

for(i= 1;i<c->size;i++){
literal qi= c->data[i];
variable v= fs_lit2var(qi);
literal lev= levelOf(f,v);
if(!f->seen.data[v]&&lev> 0){
if(getReason(f,qi)&&f->allLevels.data[lev]){
posvecPush(&f->analyseStack,v);
intvecPush(&f->analyseToClear,qi);
f->seen.data[v]= true;
}else{
cleanSeen(f,top);
return false;
}
}
}
}
return true;
}

void minimizeUip(funcsat*f,clause*learned)
{

variable i,j;
for(i= 0;i<learned->size;i++){
literal l= levelOf(f,fs_lit2var(learned->data[i]));
assert(l!=Unassigned);
f->allLevels.data[l]= true;
}

intvecClear(&f->analyseToClear);
literal*it;
forClause(it,learned){
intvecPush(&f->analyseToClear,*it);
}


for(i= 0;i<learned->size;i++){
f->seen.data[fs_lit2var(learned->data[i])]= true;
}



for(i= 1,j= 1;i<learned->size;i++){
literal p= learned->data[i];
if(!litRedundant(f,p))learned->data[j++]= p;
else{
assert(!isAssumption(f,fs_lit2var(p)));
fslog(f,"minimizeUip",5,"deleted %i\n",p);
++f->numLiteralsDeleted;
}
}
learned->size-= i-j;


while(f->analyseToClear.size> 0){
literal l= intvecPop(&f->analyseToClear);
variable v= fs_lit2var(l);
f->seen.data[v]= false;
f->allLevels.data[levelOf(f,v)]= false;
}
}


/*:113*//*115:*/
#line 2651 "funcsat.w"

static char*dot_lit2label(literal p)
{
static char buf[64];
sprintf(buf,"lit%i",fs_lit2var(p));
return buf;
}

static void print_dot_impl_graph_rec(funcsat*f,FILE*dotfile,bool*seen,literal p)
{
if(seen[fs_lit2var(p)]==false){
fprintf(dotfile,"%s ",dot_lit2label(p));
fprintf(dotfile,"[label=\"%i @ %u%s\"];\n",
(funcsatValue(f,p)==false?-p:p),
levelOf(f,fs_lit2var(p)),
(funcsatValue(f,p)==unknown?"*":""));
seen[fs_lit2var(p)]= true;
clause*r= getReason(f,p);
if(r){
literal*q;
fprintf(dotfile,"/* reason for %i: ",p);
fs_clause_print(f,dotfile,r);
fprintf(dotfile,"*/\n");
for_clause(q,r){
print_dot_impl_graph_rec(f,dotfile,seen,*q);
if(*q!=-p){
fprintf(dotfile,"%s",dot_lit2label(*q));
fprintf(dotfile," -> ");
fprintf(dotfile,"%s;\n",dot_lit2label(p));
}
}
}else{
fprintf(dotfile,"/* no reason for %i */\n",p);
}
}
}

static void print_dot_impl_graph(funcsat*f,clause*cc)
{
literal*p,*q;
bool*seen;
CallocX(seen,f->numVars+1,sizeof(*seen));
FILE*dotfile= fopen("g.dot","w");

fprintf(dotfile,"digraph G {\n");
fprintf(dotfile,"/* conflict clause: ");
fs_clause_print(f,dotfile,cc);
fprintf(dotfile,"*/\n");
for_clause(p,cc){
print_dot_impl_graph_rec(f,dotfile,seen,*p);
}
#ifdef EDGES_FOR_CLAUSE
q= NULL;
for_clause(p,cc){
if(q){
fprintf(dotfile,"%s",dot_lit2label(*p));
fprintf(dotfile," -> ");
fprintf(dotfile,"%s [color=\"red\"];\n",dot_lit2label(*q));
}
q= p;
}
#endif
fprintf(dotfile,"\n}");
free(seen);
fclose(dotfile);
}


/*:115*//*118:*/
#line 2750 "funcsat.w"

bool propagateFacts(funcsat*f,head_tail*facts,literal uipLit)
{
bool isConsistent= true;
variable uipVar= fs_lit2var(uipLit);
unsigned cnt= 0;
dopen(f,"bcp");

clause*prev,*curr,*next;
for_head_tail(facts,prev,curr,next){
++f->numUnitFactProps,++cnt;
/*119:*/
#line 2790 "funcsat.w"

fs_ifdbg(f,"bcp",5){
fslog(f,"bcp",5,"");
fs_clause_print(f,fs_dbgout(f),curr);
}

if(curr->size==0)goto Conflict;
else if(curr->size==1){
literal p= curr->data[0];
mbool val= funcsatValue(f,p);
if(val==false)goto Conflict;
else if(val==unknown){
trailPush(f,p,reason_info_mk(f,curr));
fslog(f,"bcp",1," => %i\n",p);
}
continue;
}

literal p= curr->data[0],q= curr->data[1];
assert(p!=0&&q!=0&&"unset literals");
assert(p!=q&&"did not find distinct literals");
mbool vp= tentativeValue(f,p),vq= tentativeValue(f,q);



if(vp==true&&vq==true){
head_tail_iter_rm(facts,prev,curr,next);
addWatchUnchecked(f,curr);
fslog(f,"bcp",5," => watched\n");
}else if(vp==true||vq==true){
;
fslog(f,"bcp",5," => unmolested\n");
}else if(vp==unknown&&vq==unknown){
head_tail_iter_rm(facts,prev,curr,next);
addWatchUnchecked(f,curr);
fslog(f,"bcp",5," => watched\n");
}else if(vp==unknown){
if(funcsatValue(f,p)==false)goto Conflict;
assert(curr->data[0]==p);
trailPush(f,p,reason_info_mk(f,curr));
f->conf->bumpUnitClause(f,curr);
fslog(f,"bcp",1," => %i\n",p);
}else if(vq==unknown){
if(funcsatValue(f,q)==false)goto Conflict;
assert(curr->data[1]==q);
curr->data[0]= q,curr->data[1]= p;
trailPush(f,q,reason_info_mk(f,curr));
fslog(f,"bcp",1," => %i\n",q);
f->conf->bumpUnitClause(f,curr);
}else{
Conflict:
isConsistent= false;
f->conflictClause= curr;
fslog(f,"bcp",1," => X\n");
}

/*:119*/
#line 2761 "funcsat.w"

if(!isConsistent)break;
}

dclose(f,"bcp");
fslog(f,"bcp",1,"propagated %u facts\n",cnt);
if(f->conflictClause){
if(funcsatValue(f,uipLit)==unknown){
if(f->decisionLevel> 0){
uipVar= fs_lit2var(f->trail.data[f->trail.size-1]);
}else goto ReallyDone;
}
}
#if 0
assert(!head||funcsatValue(f,fs_var2lit(uipVar))!=unknown);
assert(clauseIsAlone(&f->unitFacts.data[uipVar],0));
#endif

if(levelOf(f,uipVar)!=0){
head_tail_append(&f->unit_facts[uipVar],facts);
}

ReallyDone:
return isConsistent;
}

/*:118*//*120:*/
#line 2853 "funcsat.w"

static inline mbool tentativeValue(funcsat*f,literal p)
{
variable v= fs_lit2var(p);
literal*valLoc= &f->trail.data[f->model.data[v]];
bool isTentative= valLoc>=f->trail.data+f->propq;
if(levelOf(f,v)!=Unassigned&&!isTentative)return p==*valLoc;
else return unknown;
}

/*:120*//*126:*/
#line 2924 "funcsat.w"

void incLubyRestart(funcsat*f,bool skip);

bool funcsatNoRestart(funcsat*f,void*p){
UNUSED(f),UNUSED(p);
return false;
}

bool funcsatLubyRestart(funcsat*f,void*p)
{
UNUSED(p);
if((int)f->numConflicts>=f->lrestart&&f->decisionLevel> 2){
incLubyRestart(f,false);
return true;
}
return false;
}

bool funcsatInnerOuter(funcsat*f,void*p)
{
UNUSED(p);
static uint64_t inner= 100UL,outer= 100UL,conflicts= 1000UL;
if(f->numConflicts>=conflicts){
conflicts+= inner;
if(inner>=outer){
outer*= 1.1;
inner= 100UL;
}else{
inner*= 1.1;
}
return true;
}
return false;
}

bool funcsatMinisatRestart(funcsat*f,void*p)
{
UNUSED(p);
static uint64_t cutoff= 100UL;
if(f->numConflicts>=cutoff){
cutoff*= 1.5;
return true;
}
return false;
}







int64_t luby(int64_t i)
{
int64_t k;
for(k= 1;k<(int64_t)sizeof(k);k++)
if(i==(1<<k)-1)
return 1<<(k-1);

for(k= 1;;k++)
if((1<<(k-1))<=i&&i<(1<<k)-1)
return luby(i-(1<<(k-1))+1);
}

void incLubyRestart(funcsat*f,bool skip)
{
UNUSED(skip);
uint64_t delta;


if(f->lubycnt> 250){
f->lubycnt= 0;
f->lubymaxdelta= 0;
f->waslubymaxdelta= false;
}
delta= 100*(uint64_t)luby((int64_t)++f->lubycnt);
f->lrestart= (int64_t)(f->numConflicts+delta);






if(delta> f->lubymaxdelta){
f->lubymaxdelta= delta;
f->waslubymaxdelta= 1;
}else{
f->waslubymaxdelta= 0;
}
}


/*:126*//*147:*/
#line 4179 "funcsat.w"

void fs_vig_print(funcsat*f,const char*path)
{
FILE*dot;
char buf[256];
if(NULL==(dot= fopen(path,"w")))perror("fopen"),exit(1);
fprintf(dot,"graph G {\n");

fprintf(dot,"// variables\n");
for(variable i= 1;i<=f->numVars;i++){
fprintf(dot,"%u;\n",i);
}

fprintf(dot,"// clauses\n");
unsigned cnt= 0;
for(unsigned i= 0;i<f->origClauses.size;i++){
char*bufptr= buf;
clause*c= f->origClauses.data[i];
literal*p;
for_clause(p,c){
bufptr+= sprintf(bufptr,"%i ",*p);
}
fprintf(dot,"clause%u [shape=box,label=\"%s\",fillcolor=black,shape=box];\n",
cnt,buf);
for_clause(p,c){
fprintf(dot,"clause%u -- %u [%s];\n",
cnt,
fs_lit2var(*p),
(*p<0?"color=red":"color=green"));
}
cnt++;
}

fprintf(dot,"}\n");
if(0!=fclose(dot))perror("fclose");

}


/*:147*//*151:*/
#line 4359 "funcsat.w"

DEFINE_HASHTABLE(fsLogMapInsert,fsLogMapSearch,fsLogMapRemove,char,int)

int fslog(const funcsat*f,const char*label,int msgLevel,const char*format,...)
{
int pr= 0;
int*logLevel;
va_list ap;

if(f->conf->logSyms&&(logLevel= hashtable_search(f->conf->logSyms,(void*)label))){
if(msgLevel<=*logLevel){
unsigned indent= posvecPeek(&f->conf->logStack),i;
for(i= 0;i<indent;i++)fprintf(f->conf->debugStream," ");
if(f->conf->printLogLabel)pr+= fprintf(f->conf->debugStream,"%s %d: ",label,msgLevel);
va_start(ap,format);
pr+= vfprintf(f->conf->debugStream,format,ap);
va_end(ap);
}
}
return pr;
}

int dopen(const funcsat*f,const char*label)
{
if(f->conf->logSyms&&hashtable_search(f->conf->logSyms,(void*)label)){
unsigned indent= posvecPeek(&f->conf->logStack)+2;
posvecPush(&f->conf->logStack,indent);
}
return 0;
}
int dclose(const funcsat*f,const char*label)
{
if(f->conf->logSyms&&hashtable_search(f->conf->logSyms,(void*)label)){
posvecPop(&f->conf->logStack);
}
return 0;
}


/*:151*//*152:*/
#line 4400 "funcsat.w"

int64_t readHeader(int(*getChar)(void*),void*,funcsat*func);
void readClauses(
int(*getChar)(void*),void*,funcsat*func,uint64_t numClauses);

#define VecGenTy(name, eltTy)                                           \
  typedef struct                                                        \
  {                                                                     \
    eltTy   *data;                                                      \
    size_t size;                                    \
    size_t capacity;                              \
  } name ## _t;                                                         \
  static void name ## Destroy(name ## _t *v);                           \
  static void name ## Init(name ## _t *v, size_t capacity);             \
  static void name ## Clear(name ## _t *v);                             \
  static void name ## Push(name ## _t *v, eltTy data);



#define VecGen(name, eltTy)                                     \
  static void name ## Init(name ## _t *v, size_t capacity)      \
  {                                                             \
    size_t c =  capacity >  0 ? capacity : 4;                     \
    CallocX(v->data, c, sizeof(*v->data));                      \
    v->size =  0;                                                \
    v->capacity =  c;                                            \
  }                                                             \
                                                                \
  static void name ## Destroy(name ## _t *v)                    \
  {                                                             \
    free(v->data);                                              \
    v->data =  NULL;                                             \
    v->size =  0;                                                \
    v->capacity =  0;                                            \
  }                                                             \
                                                                \
  static void name ## Clear(name ## _t *v)                      \
  {                                                             \
    v->size =  0;                                                \
  }                                                             \
                                                                \
  static void name ## Push(name ## _t *v, eltTy data)           \
  {                                                             \
    if (v->capacity == v->size) {                               \
      ReallocX(v->data, v->capacity*2+1, sizeof(eltTy));        \
      v->capacity =  v->capacity*2+1;                            \
    }                                                           \
    v->data[v->size++] =  data;                                  \
  }


VecGenTy(vecc,char);
VecGen(vecc,char);

int fgetcWrapper(void*stream)
{
return fgetc((FILE*)stream);
}

void parseDimacsCnf(const char*path,funcsat*func)
{
struct stat buf;
if(-1==stat(path,&buf))perror("stat"),exit(1);
if(!S_ISREG(buf.st_mode)){
fprintf(stderr,"Error: '%s' not a regular file\n",path);
exit(1);
}
int(*getChar)(void*);
void*stream;
const char*opener;
bool isGz= 0==strcmp(".gz",path+(strlen(path)-strlen(".gz")));
if(isGz){
#ifdef HAVE_LIBZ
#  if 0
fprintf(stderr,"c found .gz file\n");
#  endif
getChar= (int(*)(void*))gzgetc;
stream= gzopen(path,"r");
opener= "gzopen";
#else
fprintf(stderr,"cannot read gzipp'd file\n");
exit(1);
#endif
}else{
getChar= fgetcWrapper;
stream= fopen(path,"r");
opener= "fopen";
}
if(!stream){
perror(opener);
exit(1);
}

uint64_t numClauses= (uint64_t)readHeader(getChar,stream,func);
readClauses(getChar,stream,func,numClauses);

if(isGz){
#ifdef HAVE_LIBZ
if(Z_OK!=gzclose(stream))perror("gzclose"),exit(1);
#else
assert(0&&"no libz and this shouldn't happen");
#endif
}else{
if(0!=fclose(stream))perror("fclose"),exit(1);
}
}


static literal readLiteral(
int(*getChar)(void*stream),
void*stream,
vecc_t*tmp,
uint64_t numClauses);
int64_t readHeader(int(*getChar)(void*stream),void*stream,funcsat*func)
{
UNUSED(func);
char c;
Comments:
while(isspace(c= getChar(stream)));
if('c'==c){
while('\n'!=(c= getChar(stream)));
}

if('p'!=c){
goto Comments;
}
while('c'!=(c= getChar(stream)));
assert(c=='c');
c= getChar(stream);
assert(c=='n');
c= getChar(stream);
assert(c=='f');

vecc_t tmp;
veccInit(&tmp,4);
readLiteral(getChar,stream,&tmp,0);
unsigned numClauses= (unsigned)readLiteral(getChar,stream,&tmp,0);
veccDestroy(&tmp);
#if 0
fprintf(stderr,"c read header 'p cnf %u %u'\n",numVariables,numClauses);
#endif
return(int)numClauses;
}


void readClauses(
int(*getChar)(void*stream),
void*stream,
funcsat*func,
uint64_t numClauses)
{
clause*clause;
vecc_t tmp;
veccInit(&tmp,4);
if(numClauses> 0){
do{
clause= clauseAlloc(5);

#if 0
literal literal= readLiteral(getChar,stream);
#endif
literal literal= readLiteral(getChar,stream,&tmp,numClauses);

while(literal!=0){
clausePush(clause,literal);
literal= readLiteral(getChar,stream,&tmp,numClauses);
}
#if 0
dimacsPrintClause(stderr,clause);
fprintf(stderr,"\n");
#endif
if(FS_UNSAT==funcsatAddClause(func,clause)){
fprintf(stdout,"c trivially unsat\n");
exit(EXIT_SUCCESS);
}
}while(--numClauses> 0);
}
veccDestroy(&tmp);
}

static literal readLiteral(
int(*getChar)(void*stream),
void*stream,
vecc_t*tmp,
uint64_t numClauses)
{
char c;
bool begun;
veccClear(tmp);
literal literal;
begun= false;
while(1){
c= getChar(stream);
if(isspace(c)||EOF==c){
if(begun){
break;
}else if(EOF==c){
fprintf(stderr,"readLiteral error: too few clauses (after %"
PRIu64" clauses)\n",numClauses);
exit(1);
}else{
continue;
}
}
begun= true;
veccPush(tmp,c);
}
veccPush(tmp,'\0');
literal= strtol((char*)tmp->data,NULL,10);
return literal;
}





void lbdSweep(funcsat*f,void*p);
void claActivitySweep(funcsat*f,void*p);


void varBumpScore(funcsat*f,variable v);


static variable computeLbdScore(funcsat*f,clause*c);


/*:152*//*153:*/
#line 4627 "funcsat.w"









static void swapInMaxLevelLit(funcsat*f,clause*reason,unsigned swapIdx,unsigned startIdx)
{
literal secondWatch= 0,swLevel= -1;

for(variable i= startIdx;i<reason->size;i++){
literal lev= levelOf(f,fs_lit2var(reason->data[i]));
fslog(f,"subsumption",9,"level of %i = %u\n",
reason->data[i],levelOf(f,fs_lit2var(reason->data[i])));
if(swLevel<lev)swLevel= lev,secondWatch= (literal)i;

if(lev==fs_var2lit(f->decisionLevel))break;
}
if(swLevel!=-1){
literal tmp= reason->data[swapIdx];
reason->data[swapIdx]= reason->data[secondWatch],reason->data[secondWatch]= tmp;
}
}


static void checkSubsumption(
funcsat*f,literal p,clause*learn,clause*reason,bool learnIsUip)
{
UNUSED(p);


if(learn->size==reason->size-1&&
reason->size> 1&&
learn->size> 2){

++f->numSubsumptions;

fs_ifdbg(f,"subsumption",1){
FILE*out= f->conf->debugStream;
dimacsPrintClause(out,learn);
fprintf(out," subsumes ");
dimacsPrintClause(out,reason);
fprintf(out,"\n");
}

assert(!reason->isReason);

if(learnIsUip)vectorPush(&f->subsumed,(void*)1);
else vectorPush(&f->subsumed,(void*)0);
vectorPush(&f->subsumed,reason);
}
}





static variable computeLbdScore(funcsat*f,clause*c)
{


uint16_t score= 0;
f->lbdCount++;

for(variable i= 0;i<c->size;i++){
literal level= levelOf(f,fs_lit2var(c->data[i]));
assert(level!=Unassigned);
if(f->lbdLevels.data[level]!=f->lbdCount){
f->lbdLevels.data[level]= f->lbdCount;
score++;
}

if(score> 16)break;
}
assert(c->size==0||score!=0);
return score;
}

static int compareByLbdRev(const void*cl1,const void*cl2);










void lbdSweep(funcsat*f,void*user)
{
UNUSED(user);
static uint64_t num= 0,lastNumConflicts= 0,lastNumLearnedClauses= 0;
uint64_t diffConfl= f->numConflicts-lastNumConflicts;
uint64_t diffLearn= f->numLearnedClauses-lastNumLearnedClauses;
uint64_t avg= diffConfl==0?1:Max(1,diffLearn/diffConfl);
uint64_t next= ((uint64_t)20000/avg)+((uint64_t)500/avg)*num;

assert(next> 0);
if(diffConfl> next){
++f->numSweeps;


qsort(f->learnedClauses.data,f->learnedClauses.size,sizeof(clause*),compareByLbdRev);

fs_ifdbg(f,"sweep",5){
uint64_t dupCount= 0;
double lastActivity= 0.f;
FILE*out= f->conf->debugStream;
fprintf(out,"sorted:\n");
clause**it;
forVectorRev(clause**,it,&f->learnedClauses){
if((*it)->activity==lastActivity){
dupCount++;
}else{
if(dupCount> 0){
fprintf(out,"(repeats %"PRIu64" times) %f ",dupCount,(*it)->activity);
}else{
fprintf(out,"%f ",(*it)->activity);
}
dupCount= 0;
lastActivity= (*it)->activity;
}
}
if(dupCount> 0){
fprintf(out,"%f (repeats %"PRIu64" times) ",lastActivity,dupCount);
}

fprintf(out,"done\n");
}



for(unsigned i= 0;i<f->learnedClauses.size;i++){
clause*c= f->learnedClauses.data[i];
}

uint64_t numDeleted= 0;
const uint64_t max= f->learnedClauses.size/2;
fslog(f,"sweep",1,"deleting at most %"PRIu64
" clauses (of %u)\n",max,f->learnedClauses.size);
for(variable i= 0;i<f->learnedClauses.size&&f->learnedClauses.size> max;i++){
clause*c= f->learnedClauses.data[i];
if(c->lbdScore==2)continue;

if(c->size> 2){
clause*rmClause= funcsatRemoveClause(f,c);
if(rmClause){
clauseFree(c);
--f->numLearnedClauses,++f->numLearnedDeleted;
numDeleted++;
}
}
}
fslog(f,"sweep",1,">>> deleted %"PRIu64" clauses\n",numDeleted);

num++;
lastNumLearnedClauses= f->numLearnedClauses;
lastNumConflicts= f->numConflicts;
}
}



static int compareByActivityRev(const void*cl1,const void*cl2);



void claActivitySweep(funcsat*f,void*user)
{
UNUSED(user);
static uint64_t num= 0,lastNumConflicts= 0,lastNumLearnedClauses= 0;
#if 0
const uint64_t diffConfl= f->numConflicts-lastNumConflicts;
const uint64_t diffLearn= f->numLearnedClauses-lastNumLearnedClauses;
uint64_t size= f->learnedClauses.size;
double extraLim= f->claInc/(size*1.f);
#endif

if(f->numLearnedClauses*1.f>=f->maxLearned){
++f->numSweeps;
uint64_t numDeleted= 0;


qsort(f->learnedClauses.data,f->learnedClauses.size,sizeof(clause*),compareByActivityRev);

fs_ifdbg(f,"sweep",5){
uint64_t dupCount= 0;
int32_t lastLbd= LBD_SCORE_MAX;
FILE*out= f->conf->debugStream;
fprintf(out,"sorted:\n");
clause**it;
forVectorRev(clause**,it,&f->learnedClauses){
if((*it)->lbdScore==lastLbd){
dupCount++;
}else{
if(dupCount> 0){
fprintf(out,"(repeats %"PRIu64" times) %d ",dupCount,(*it)->lbdScore);
}else{
fprintf(out,"%d ",(*it)->lbdScore);
}
dupCount= 0;
lastLbd= (*it)->lbdScore;
}
}
if(dupCount> 0){
fprintf(out,"%d (repeats %"PRIu64" times) ",lastLbd,dupCount);
}

fprintf(out,"done\n");
}



for(unsigned i= 0;i<f->learnedClauses.size;i++){
clause*c= f->learnedClauses.data[i];
}


#if 0
for(variable i= 0;i<f->learnedClauses.size;i++){
clause*c= f->learnedClauses.data[i];
if(c->activity==0.f){
clause*rmClause= funcsatRemoveClause(f,c);
if(rmClause){
clauseFree(c);
--f->numLearnedClauses,++f->numLearnedDeleted,++numDeleted;
}
}else break;
}
#endif

const uint64_t max= f->learnedClauses.size/2;
fslog(f,"sweep",1,"deleting at most %"PRIu64
" clauses (of %u)\n",max,f->learnedClauses.size);
for(variable i= 0;i<f->learnedClauses.size&&f->learnedClauses.size> max;i++){
clause*c= f->learnedClauses.data[i];
if(c->size> 2){
clause*rmClause= funcsatRemoveClause(f,c);
if(rmClause){
clauseFree(c);
--f->numLearnedClauses,++f->numLearnedDeleted,++numDeleted;
}
}
}
fslog(f,"sweep",1,">>> deleted %"PRIu64" clauses\n",numDeleted);

num++;
lastNumLearnedClauses= f->numLearnedClauses;
lastNumConflicts= f->numConflicts;
}
}

static void varDecayActivity(funcsat*f);
static void claDecayActivity(funcsat*f);
static void bumpClauseByActivity(funcsat*f,clause*c)
{

clause**cIt;
if((c->activity+= f->claInc)> 1e20){
fslog(f,"sweep",5,"rescale for activity %f\n",c->activity);

forVector(clause**,cIt,&f->learnedClauses){
double oldActivity= (*cIt)->activity;
(*cIt)->activity*= 1e-20;
fslog(f,"sweep",5,"setting activity from %f to %f\n",oldActivity,(*cIt)->activity);
}
double oldClaInc= f->claInc;
f->claInc*= 1e-20;
fslog(f,"sweep",5,"setting claInc from %f to %f\n",oldClaInc,f->claInc);
}
}

static void varDecayActivity(funcsat*f)
{
f->varInc*= (1/f->varDecay);
}
static void claDecayActivity(funcsat*f)
{
double oldClaInc= f->claInc;
f->claInc*= (1/f->claDecay);
fslog(f,"sweep",9,"decaying claInc from %f to %f\n",oldClaInc,f->claInc);
}


void bumpReasonByActivity(funcsat*f,clause*c)
{
bumpClauseByActivity(f,c);


literal*it;
forClause(it,c){
varBumpScore(f,fs_lit2var(*it));
}
}

void bumpLearnedByActivity(funcsat*f,clause*c)
{
bumpClauseByActivity(f,c);


literal*it;
forClause(it,c){
varBumpScore(f,fs_lit2var(*it));
varBumpScore(f,fs_lit2var(*it));
}

if(--f->learnedSizeAdjustCnt==0){
f->learnedSizeAdjustConfl*= f->learnedSizeAdjustInc;
f->learnedSizeAdjustCnt= (uint64_t)f->learnedSizeAdjustConfl;
f->maxLearned*= f->learnedSizeInc;
fslog(f,"sweep",1,"update: ml %f\n",f->maxLearned);

}

varDecayActivity(f);
claDecayActivity(f);
}

/*:153*//*155:*/
#line 5110 "funcsat.w"

unsigned int fsLitHash(void*lIn)
{

literal l= *(literal*)lIn;
return(unsigned int)l;
}
int fsLitEq(void*a,void*b){
literal p= *(literal*)a,q= *(literal*)b;
return p==q;
}
unsigned int fsVarHash(void*lIn)
{

literal l= *(literal*)lIn;
return(unsigned int)fs_lit2var(l);
}
int fsVarEq(void*a,void*b){
literal p= *(literal*)a,q= *(literal*)b;
return fs_lit2var(p)==fs_lit2var(q);
}



int litCompare(const void*l1,const void*l2)
{
literal x= *(literal*)l1,y= *(literal*)l2;
if(fs_lit2var(x)!=fs_lit2var(y)){
return fs_lit2var(x)<fs_lit2var(y)?-1:1;
}else{
if(x==y){
return 0;
}else{
return x<y?-1:1;
}
}
}


#if 0
int clauseCompare(const void*cp1,const void*cp2)
{
const clause*c= *(clause**)cp1,*d= *(clause**)cp2;
if(c->size!=d->size){
return c->size<d->size?-1:1;
}else{

uint32_t i;
for(i= 0;i<c->size;i++){
int ret;
if(0!=(ret= litCompare(&c->data[i],&d->data[i]))){
return ret;
}
}
return 0;
}
}
#endif


void sortClause(clause*clause)
{
qsort(clause->data,clause->size,sizeof(*clause->data),litCompare);
}


literal findLiteral(literal p,clause*clause)
{
literal min= 0,max= clause->size-1,mid= -1;
int res= -1;
while(!(res==0||min> max)){
mid= min+((max-min)/2);
res= litCompare(&p,&clause->data[mid]);
if(res> 0){
min= mid+1;
}else{
max= mid-1;
}
}
return res==0?mid:-1;
}

literal findVariable(variable v,clause*clause)
{
literal min= 0,max= clause->size-1,mid= -1;
int res= -1;
while(!(res==0||min> max)){
mid= min+((max-min)/2);
if(v==fs_lit2var(clause->data[mid])){
res= 0;
}else{
literal p= fs_var2lit(v);
res= litCompare(&p,&clause->data[mid]);
}
if(res> 0){
min= mid+1;
}else{
max= mid-1;
}
}
return res==0?mid:-1;
}




clause*clauseAlloc(uint32_t capacity)
{
clause*c;
MallocX(c,1,sizeof(*c));
clauseInit(c,capacity);
return c;
}

void clauseInit(clause*v,uint32_t capacity)
{
uint32_t c= capacity> 0?capacity:4;
CallocX(v->data,c,sizeof(*v->data));
v->size= 0;
v->capacity= c;
v->isLearnt= false;
v->lbdScore= LBD_SCORE_MAX;
v->nx= NULL;
v->is_watched= false;
v->isReason= false;
v->activity= 0.f;
}

void clauseFree(clause*v)
{
clauseDestroy(v);
free(v);
}

void clauseDestroy(clause*v)
{
free(v->data);
v->data= NULL;
v->size= 0;
v->capacity= 0;
v->isLearnt= false;
v->lbdScore= LBD_SCORE_MAX;
}

void clauseClear(clause*v)
{
v->size= 0;
v->lbdScore= LBD_SCORE_MAX;
v->isLearnt= false;
}

void clausePush(clause*v,literal data)
{
if(v->capacity<=v->size){
while(v->capacity<=v->size){
v->capacity= v->capacity*2+1;
}
ReallocX(v->data,v->capacity,sizeof(*v->data));
}
v->data[v->size++]= data;
}

void clausePushAt(clause*v,literal data,uint32_t i)
{
uint32_t j;
assert(i<=v->size);
if(v->capacity<=v->size){
while(v->capacity<=v->size){
v->capacity= v->capacity*2+1;
}
ReallocX(v->data,v->capacity,sizeof(*v->data));
}
v->size++;
for(j= v->size-(uint32_t)1;j> i;j--){
v->data[j]= v->data[j-1];
}
v->data[i]= data;
}


void clauseGrowTo(clause*v,uint32_t newCapacity)
{
if(v->capacity<newCapacity){
v->capacity= newCapacity;
ReallocX(v->data,v->capacity,sizeof(*v->data));
}
assert(v->capacity>=newCapacity);
}


literal clausePop(clause*v)
{
assert(v->size!=0);
return v->data[v->size---1];
}

literal clausePopAt(clause*v,uint32_t i)
{
uint32_t j;
assert(v->size!=0);
literal res= v->data[i];
for(j= i;j<v->size-(uint32_t)1;j++){
v->data[j]= v->data[j+1];
}
v->size--;
return res;
}

literal clausePeek(clause*v)
{
assert(v->size!=0);
return v->data[v->size-1];
}

void clauseSet(clause*v,uint32_t i,literal p)
{
v->data[i]= p;
}

void clauseCopy(clause*dst,clause*src)
{
uintptr_t i;
for(i= 0;i<src->size;i++){
clausePush(dst,src->data[i]);
}
dst->isLearnt= src->isLearnt;
dst->lbdScore= src->lbdScore;
}



/*:155*//*156:*/
#line 5343 "funcsat.w"






clause*funcsatSolToClause(funcsat*f){
clause*c= clauseAlloc(f->trail.size);
for(unsigned i= 0;i<f->trail.size;i++)
clausePush(c,-f->trail.data[i]);
return c;
}

funcsat_result funcsatFindAnotherSolution(funcsat*f){
clause*cur_sol= funcsatSolToClause(f);
funcsat_result res= funcsatAddClause(f,cur_sol);
if(res==FS_UNSAT)return FS_UNSAT;
res= funcsatSolve(f);
return res;
}

int funcsatSolCount(funcsat*f,clause subset,clause*lastSolution)
{
assert(f->assumptions.size==0);
int count= 0;
for(unsigned i= 0;i<subset.size;i++){
funcsatResize(f,fs_lit2var(subset.data[i]));
}

clause assumptions;
clauseInit(&assumptions,subset.size);

unsigned twopn= (unsigned)round(pow(2.,(double)subset.size));
fslog(f,"countsol",1,"%u incremental problems to solve\n",twopn);
for(unsigned i= 0;i<twopn;i++){
fslog(f,"countsol",2,"%u: ",i);
clauseClear(&assumptions);
clauseCopy(&assumptions,&subset);


unsigned n= i;
for(unsigned j= 0;j<subset.size;j++){

if((n%2)==0)assumptions.data[j]*= -1;
n>>= 1;
}

if(funcsatPushAssumptions(f,&assumptions)==FS_UNSAT){
continue;
}

if(FS_SAT==funcsatSolve(f)){
count++;
if(lastSolution){
clauseClear(lastSolution);
clauseCopy(lastSolution,&f->trail);
}
}

funcsatPopAssumptions(f,f->assumptions.size);
}

clauseDestroy(&assumptions);

return count;
}


/*:156*//*158:*/
#line 5549 "funcsat.w"


extern inline mbool funcsatValue(funcsat*f,literal p)
{
variable v= fs_lit2var(p);
if(f->level.data[v]==Unassigned)return unknown;
literal value= f->trail.data[f->model.data[v]];
return p==value;
}

static inline struct litpos*buildLitPos(clause*c)
{
literal*lIt;
litpos*p;
CallocX(p,1,sizeof(*p));
p->c= c;
p->ix= posvecAlloc(c->size);
p->ix->size= c->size;
unsigned*uIt;
forPosVec(uIt,p->ix)*uIt= UINT_MAX;
forClause(lIt,p->c){
p->ix->data[fs_lit2idx(*lIt)]= (unsigned)(lIt-p->c->data);
}
return p;
}

static inline literal*clauseLitPos(litpos*pos,literal l)
{
return pos->c->data+pos->ix->data[fs_lit2idx(l)];
}

static inline bool clauseContains(litpos*pos,literal l)
{
return pos->ix->data[fs_lit2idx(l)]!=UINT_MAX;
}



static inline unsigned getLitPos(funcsat*f,literal l)
{
return f->litPos.data[fs_lit2var(l)];
}

static inline void setLitPos(funcsat*f,literal l,unsigned pos)
{
f->litPos.data[fs_lit2var(l)]= pos;
}


static inline void resetLitPos(funcsat*f,literal l,unsigned pos)
{
UNUSED(pos);
setLitPos(f,l,UINT_MAX);
}

static inline bool hasLitPos(funcsat*f,literal l)
{
return getLitPos(f,l)!=UINT_MAX;
}

static inline void spliceUnitFact(funcsat*f,literal l,clause*c)
{
head_tail_add(&f->unit_facts[fs_lit2var(l)],c);
}

/*:158*//*161:*/
#line 5628 "funcsat.w"


static char parse_read_char(funcsat*f,FILE*solutionFile){
UNUSED(f);
char c;
c= fgetc(solutionFile);
#if 0
bf_log(b,"bf",8,"Read character '%c'\n",c);
#endif
return c;
}

static const char*s_SATISFIABLE= "SATISFIABLE\n";
static const char*s_UNSATISFIABLE= "UNSATISFIABLE\n";

funcsat_result fs_parse_dimacs_solution(funcsat*f,FILE*solutionFile)
{
char c;
literal var;
bool truth;
funcsat_result result= FS_UNKNOWN;
bool have_var= false;
const char*cur;
state_new_line:
while(true){
c= parse_read_char(f,solutionFile);
switch(c){
case EOF:
goto state_eof;
case'c':
goto state_comment;
case's':
goto state_satisfiablility;
case'v':
goto state_variables;
default:
fslog(f,"fs",1,"unknown line type '%c' in solution file\n",c);
goto state_error;
}
}
state_comment:
while(true){
c= parse_read_char(f,solutionFile);
switch(c){
case EOF:
goto state_eof;
case'\n':
goto state_new_line;
default:
break;
}
}
state_satisfiablility:
cur= NULL;
funcsat_result pending= FS_UNKNOWN;
while(true){
c= parse_read_char(f,solutionFile);
switch(c){
case' ':
case'\t':
continue;
case EOF:
goto state_eof;
default:
ungetc(c,solutionFile);
break;
}
break;
}
while(true){
c= parse_read_char(f,solutionFile);
if(cur==NULL){
switch(c){
case'S':
cur= &s_SATISFIABLE[1];
pending= FS_SAT;
break;
case'U':
cur= &s_UNSATISFIABLE[1];
pending= FS_UNSAT;
break;
case EOF:
goto state_eof;
default:
fslog(f,"fs",1,"unknown satisfiability\n");
goto state_error;
}
}else{
if(c==EOF){
goto state_eof;
}else if(c!=*cur){
fslog(f,"fs",1,"reading satisfiability, got '%c', expected '%c'\n",c,*cur);
goto state_error;
}
if(c=='\n'){
result= pending;
goto state_new_line;
}
++cur;
}
}
state_variables:
while(true){
c= parse_read_char(f,solutionFile);
switch(c){
case'\n':
goto state_new_line;
case' ':
case'\t':
break;
case EOF:
goto state_eof;
default:
ungetc(c,solutionFile);
goto state_sign;
}
}
state_sign:
have_var= false;
truth= true;
c= parse_read_char(f,solutionFile);
if(c=='-'){
truth= false;
}else{
ungetc(c,solutionFile);
}
var= 0;
goto state_variable;
state_variable:
while(true){
c= parse_read_char(f,solutionFile);
switch(c){
case'0':
case'1':
case'2':
case'3':
case'4':
case'5':
case'6':
case'7':
case'8':
case'9':
have_var= true;
var= var*10+(c-'0');
break;
default:
ungetc(c,solutionFile);
goto state_record_variable;
}
}
state_record_variable:
if(have_var){
if(var==0){










goto state_exit;
}else{
variable v= fs_lit2var(var);
funcsatPushAssumption(f,(truth?(literal)v:-(literal)v));
}
goto state_variables;
}else{
abort();
fslog(f,"fs",1,"expected variable, but didn't get one\n");
goto state_error;
}
state_eof:
if(ferror(solutionFile)){
abort();
fslog(f,"fs",1,"IO error reading solution file: %s",strerror(errno));
goto state_error;
}else{
goto state_exit;
}
state_error:
result= FS_UNKNOWN;
goto state_exit;
state_exit:
return result;
}
/*:161*/
#line 320 "funcsat.w"




/*:14*//*29:*/
#line 939 "funcsat.w"



/*:29*//*128:*/
#line 3025 "funcsat.w"


funcsat_config funcsatDefaultConfig= {
.user= NULL,
.name= NULL,
.usePhaseSaving= false,
.useSelfSubsumingResolution= false,
.minimizeLearnedClauses= true,
.numUipsToLearn= UINT_MAX,
.gc= true,
.maxJailDecisionLevel= 0,
.logSyms= NULL,
.printLogLabel= true,
.debugStream= NULL,
.isTimeToRestart= funcsatLubyRestart,
.isResourceLimitHit= funcsatIsResourceLimitHit,
.preprocessNewClause= funcsatPreprocessNewClause,
.preprocessBeforeSolve= funcsatPreprocessBeforeSolve,
.getInitialActivity= funcsatDefaultStaticActivity,
.sweepClauses= claActivitySweep,
.bumpOriginal= bumpOriginal,
.bumpReason= bumpReasonByActivity,
.bumpLearned= bumpLearnedByActivity,
.bumpUnitClause= bumpUnitClauseByActivity,
.decayAfterConflict= myDecayAfterConflict,
};

/*:128*//*130:*/
#line 3068 "funcsat.w"


funcsat*funcsatInit(funcsat_config*conf)
{
funcsat*f;
CallocX(f,1,sizeof(*f));
f->conf= conf;
f->conflictClause= NULL;
f->propq= 0;
/*129:*/
#line 3054 "funcsat.w"

f->varInc= 1.f;
f->varDecay= 0.95f;
f->claInc= 1.f;
f->claDecay= 0.9999f;
f->learnedSizeFactor= 1.f/3.f;
f->learnedSizeAdjustConfl= 25000;
f->learnedSizeAdjustCnt= 25000;
f->maxLearned= 20000.f;
f->learnedSizeAdjustInc= 1.5f;
f->learnedSizeInc= 1.1f;


/*:129*/
#line 3077 "funcsat.w"

fslog(f,"sweep",1,"set maxLearned to %f\n",f->maxLearned);
fslog(f,"sweep",1,"set 1/f->claDecoy to %f\n",1.f/f->claDecay);
f->lrestart= 1;
f->lubycnt= 0;
f->lubymaxdelta= 0;
f->waslubymaxdelta= false;

f->numVars= 0;
/*66:*/
#line 1584 "funcsat.w"

f->reason= vec_uintptr_init(2);
vec_uintptr_push(f->reason,NO_REASON);
vectorInit(&f->reason_hooks,2);

/*:66*//*71:*/
#line 1634 "funcsat.w"

f->reason_infos_freelist= UINT_MAX;
f->reason_infos= vec_reason_info_init(2);
f->reason_infos->size= 0;


/*:71*//*97:*/
#line 2162 "funcsat.w"

CallocX(f->binvar_heap,2,sizeof(*f->binvar_heap));
CallocX(f->binvar_pos,2,sizeof(*f->binvar_pos));
f->binvar_heap_size= 0;


/*:97*//*122:*/
#line 2871 "funcsat.w"

CallocX(f->unit_facts,2,sizeof(*f->unit_facts));
f->unit_facts_size= 1;
f->unit_facts_capacity= 2;


/*:122*/
#line 3086 "funcsat.w"

clauseInit(&f->assumptions,0);
posvecInit(&f->model,2);
posvecPush(&f->model,0);
clauseInit(&f->phase,2);
clausePush(&f->phase,0);
clauseInit(&f->level,2);
clausePush(&f->level,Unassigned);
posvecInit(&f->decisions,2);
posvecPush(&f->decisions,0);
clauseInit(&f->trail,2);
all_watches_init(f);
vectorInit(&f->origClauses,2);
vectorInit(&f->learnedClauses,2);
posvecInit(&f->litPos,2);
posvecPush(&f->litPos,0);
clauseInit(&f->uipClause,100);
posvecInit(&f->lbdLevels,100);
posvecPush(&f->lbdLevels,0);
posvecInit(&f->seen,2);
posvecPush(&f->seen,false);
intvecInit(&f->analyseToClear,2);
intvecPush(&f->analyseToClear,0);
posvecInit(&f->analyseStack,2);
posvecPush(&f->analyseStack,0);
posvecInit(&f->allLevels,2);
posvecPush(&f->allLevels,false);
vectorInit(&f->subsumed,10);

return f;
}



/*:130*//*131:*/
#line 3121 "funcsat.w"

funcsat_config*funcsatConfigInit(void*userData)
{
UNUSED(userData);
funcsat_config*conf= malloc(sizeof(*conf));
memcpy(conf,&funcsatDefaultConfig,sizeof(*conf));
#ifdef FUNCSAT_LOG
conf->logSyms= create_hashtable(16,hashString,stringEqual);
posvecInit(&conf->logStack,2);
posvecPush(&conf->logStack,0);
conf->debugStream= stderr;
#endif
return conf;
}

void funcsatConfigDestroy(funcsat_config*conf)
{
#ifdef FUNCSAT_LOG
hashtable_destroy(conf->logSyms,true,true);
posvecDestroy(&conf->logStack);
#endif
free(conf);
}

/*:131*//*132:*/
#line 3148 "funcsat.w"


void funcsatResize(funcsat*f,variable numVars)
{
assert(f->decisionLevel==0);
if(numVars> f->numVars){
const variable old= f->numVars,new= numVars;
f->numVars= new;
variable i;

clauseGrowTo(&f->uipClause,numVars);
intvecGrowTo(&f->analyseToClear,numVars);
posvecGrowTo(&f->analyseStack,numVars);
/*72:*/
#line 1641 "funcsat.w"

vec_reason_info_grow_to(f->reason_infos,numVars);
for(uintptr_t i= f->reason_infos->size;i<numVars;i++){
reason_info_release(f,i);
}
f->reason_infos->size= numVars;

/*:72*//*99:*/
#line 2176 "funcsat.w"

ReallocX(f->binvar_heap,numVars+1,sizeof(*f->binvar_heap));
ReallocX(f->binvar_pos,numVars+1,sizeof(*f->binvar_pos));


/*:99*/
#line 3161 "funcsat.w"

for(i= old;i<new;i++){
variable v= i+1;
/*42:*/
#line 1147 "funcsat.w"

if(f->watches.capacity<=f->watches.size+2){
while(f->watches.capacity<=f->watches.size){
f->watches.capacity= f->watches.capacity*2+2;
}
ReallocX(f->watches.wlist,f->watches.capacity,sizeof(*f->watches.wlist));
}

/*43:*/
#line 1163 "funcsat.w"

f->watches.wlist[f->watches.size].size= 0;
f->watches.wlist[f->watches.size].capacity= 0;
for(uint32_t i= 0;i<WATCHLIST_HEAD_SIZE_MAX;i++){
f->watches.wlist[f->watches.size].elts[i].lit= 0;
f->watches.wlist[f->watches.size].elts[i].cls= NULL;
}
f->watches.wlist[f->watches.size].rest= NULL;
assert(0==watchlist_head_size(&f->watches.wlist[f->watches.size]));
assert(0==watchlist_rest_size(&f->watches.wlist[f->watches.size]));


/*:43*/
#line 1155 "funcsat.w"

f->watches.size++;
/*43:*/
#line 1163 "funcsat.w"

f->watches.wlist[f->watches.size].size= 0;
f->watches.wlist[f->watches.size].capacity= 0;
for(uint32_t i= 0;i<WATCHLIST_HEAD_SIZE_MAX;i++){
f->watches.wlist[f->watches.size].elts[i].lit= 0;
f->watches.wlist[f->watches.size].elts[i].cls= NULL;
}
f->watches.wlist[f->watches.size].rest= NULL;
assert(0==watchlist_head_size(&f->watches.wlist[f->watches.size]));
assert(0==watchlist_rest_size(&f->watches.wlist[f->watches.size]));


/*:43*/
#line 1157 "funcsat.w"

f->watches.size++;
assert(f->watches.size<=f->watches.capacity);

/*:42*//*73:*/
#line 1649 "funcsat.w"

vec_uintptr_push(f->reason,NO_REASON);

/*:73*//*100:*/
#line 2182 "funcsat.w"

f->binvar_heap[v].var= v;
f->binvar_heap[v].act= f->conf->getInitialActivity(&v);
f->binvar_pos[v]= v;
bh_insert(f,v);
assert(f->binvar_pos[v]!=0);


/*:100*//*123:*/
#line 2878 "funcsat.w"

if(f->unit_facts_size>=f->unit_facts_capacity){
ReallocX(f->unit_facts,f->unit_facts_capacity*2,sizeof(*f->unit_facts));
f->unit_facts_capacity*= 2;
}
head_tail_clear(&f->unit_facts[f->unit_facts_size]);
f->unit_facts_size++;


/*:123*/
#line 3164 "funcsat.w"

#if 0
literal l= fs_var2lit(v);
#endif
posvecPush(&f->model,UINT_MAX);
clausePush(&f->phase,-fs_var2lit(v));
clausePush(&f->level,Unassigned);
posvecPush(&f->decisions,0);
posvecPush(&f->litPos,UINT_MAX);
posvecPush(&f->lbdLevels,0);
posvecPush(&f->seen,false);
posvecPush(&f->allLevels,false);
}
unsigned highestIdx= fs_lit2idx(-(literal)numVars)+1;
assert(f->model.size==numVars+1);
assert(!f->conf->usePhaseSaving||f->phase.size==numVars+1);
assert(f->level.size==numVars+1);
assert(f->decisions.size==numVars+1);
assert(f->reason->size==numVars+1);
assert(f->unit_facts_size==numVars+1);
assert(f->uipClause.capacity>=numVars);
assert(f->allLevels.size==numVars+1);
assert(f->watches.size==highestIdx);

if(numVars> f->trail.capacity){
ReallocX(f->trail.data,numVars,sizeof(*f->trail.data));
f->trail.capacity= numVars;
}
}
}

/*:132*//*133:*/
#line 3197 "funcsat.w"

void funcsatDestroy(funcsat*f)
{
literal i;
while(f->trail.size> 0)trailPop(f,NULL);
/*44:*/
#line 1177 "funcsat.w"


for(variable i= 1;i<=f->numVars;i++){
free(f->watches.wlist[fs_lit2idx((literal)i)].rest);
free(f->watches.wlist[fs_lit2idx(-(literal)i)].rest);
}
free(f->watches.wlist);


/*:44*//*74:*/
#line 1653 "funcsat.w"

vec_uintptr_destroy(f->reason);
vec_reason_info_destroy(f->reason_infos);
vectorDestroy(&f->reason_hooks);

/*:74*//*98:*/
#line 2169 "funcsat.w"

free(f->binvar_heap);
free(f->binvar_pos);
f->binvar_heap_size= 0;


/*:98*//*124:*/
#line 2888 "funcsat.w"

free(f->unit_facts);

/*:124*/
#line 3202 "funcsat.w"

clauseDestroy(&f->assumptions);
posvecDestroy(&f->model);
clauseDestroy(&f->phase);
clauseDestroy(&f->level);
posvecDestroy(&f->decisions);
clauseDestroy(&f->trail);
posvecDestroy(&f->litPos);
clauseDestroy(&f->uipClause);
posvecDestroy(&f->analyseStack);
intvecDestroy(&f->analyseToClear);
posvecDestroy(&f->lbdLevels);
posvecDestroy(&f->seen);
posvecDestroy(&f->allLevels);
for(unsigned i= 0;i<f->origClauses.size;i++){
clause*c= f->origClauses.data[i];
clauseFree(c);
}
vectorDestroy(&f->origClauses);
for(unsigned i= 0;i<f->learnedClauses.size;i++){
clause*c= f->learnedClauses.data[i];
clauseFree(c);
}
vectorDestroy(&f->learnedClauses);
vectorDestroy(&f->subsumed);
free(f);
}


/*:133*//*134:*/
#line 3233 "funcsat.w"

void funcsatSetupActivityGc(funcsat_config*conf)
{
conf->gc= true;
conf->sweepClauses= claActivitySweep;
conf->bumpReason= bumpReasonByActivity;
conf->bumpLearned= bumpLearnedByActivity;
conf->decayAfterConflict= myDecayAfterConflict;
}

void funcsatSetupLbdGc(funcsat_config*conf)
{
conf->gc= true;
conf->sweepClauses= lbdSweep;
conf->bumpReason= bumpReasonByLbd;
conf->bumpLearned= bumpLearnedByLbd;
conf->decayAfterConflict= lbdDecayAfterConflict;
}

funcsat_result funcsatResult(funcsat*f)
{
return f->lastResult;
}







/*:134*//*135:*/
#line 3264 "funcsat.w"

void backtrack(funcsat*f,variable newLevel,head_tail*facts,bool isRestart)
{
head_tail restartFacts;
head_tail_clear(&restartFacts);
if(isRestart){
assert(newLevel==0UL);
assert(!facts);
facts= &restartFacts;
}
while(f->decisionLevel!=newLevel){
trailPop(f,facts);
}
if(isRestart){
f->pct= 0.,f->pctdepth= 100.,f->correction= 1;
f->propq= 0;
}
}

static void restore_facts(funcsat*f,head_tail*facts)
{
clause*prev,*curr,*next;
for_head_tail(facts,prev,curr,next){

assert(curr->size>=1);

if(curr->size==1){
literal p= curr->data[0];
mbool val= funcsatValue(f,p);
assert(val!=false);
if(val==unknown){
trailPush(f,p,reason_info_mk(f,curr));
fslog(f,"bcp",5," => %i\n",p);
}
}else{
head_tail_iter_rm(facts,prev,curr,next);
addWatchUnchecked(f,curr);
#ifndef NDEBUG
watches_check(f);
#endif
}
}
}

/*:135*//*137:*/
#line 3314 "funcsat.w"


funcsat_result startSolving(funcsat*f)
{
f->numSolves++;
if(f->conflictClause){
if(f->conflictClause->size==0){
return FS_UNSAT;
}else{
f->conflictClause= NULL;
}
}

backtrack(f,0UL,NULL,true);
f->lastResult= FS_UNKNOWN;

assert(f->decisionLevel==0);
return FS_UNKNOWN;
}





static void finishSolving(funcsat*f)
{
UNUSED(f);
}




/*:137*//*138:*/
#line 3348 "funcsat.w"



static bool bcpAndJail(funcsat*f)
{
if(!bcp(f)){
fslog(f,"solve",2,"returning false at toplevel\n");
return false;
}

fslog(f,"solve",1,"bcpAndJail trailsize is %u\n",f->trail.size);

clause**cIt;
vector*watches;
uint64_t numJails= 0;

#if 0
forVector(clause**,cIt,&f->origClauses){
if((*cIt)->is_watched){
literal*lIt;
bool allFalse= true;
forClause(lIt,*cIt){
mbool value= funcsatValue(f,*lIt);
if(value==false)continue;
allFalse= false;
if(value==true){

clause**w0= (clause**)&watches->data[fs_lit2idx(-(*cIt)->data[0])];

clause**w1= (clause**)&watches->data[fs_lit2idx(-(*cIt)->data[1])];


clauseUnSpliceWatch(w1,*cIt,1);
jailClause(f,*lIt,*cIt);
numJails++;
break;
}
}
if(allFalse){
fslog(f,"solve",2,"returning false at toplevel\n");
return false;
}
}
}
#endif

fslog(f,"solve",2,"jailed %"PRIu64" clauses at toplevel\n",numJails);
return true;
}


bool funcsatIsResourceLimitHit(funcsat*f,void*p)
{
UNUSED(f),UNUSED(p);
return false;
}
funcsat_result funcsatPreprocessNewClause(funcsat*f,void*p,clause*c)
{
UNUSED(p),UNUSED(c);
return(f->lastResult= FS_UNKNOWN);
}
funcsat_result funcsatPreprocessBeforeSolve(funcsat*f,void*p)
{
UNUSED(p);
return(f->lastResult= FS_UNKNOWN);
}


/*:138*//*139:*/
#line 3418 "funcsat.w"

void funcsatPrintStats(FILE*stream,funcsat*f)
{
fprintf(stream,"c %"PRIu64" decisions\n",f->numDecisions);
fprintf(stream,"c %"PRIu64" propagations (%"PRIu64" unit)\n",
f->numProps+f->numUnitFactProps,
f->numUnitFactProps);
fprintf(stream,"c %"PRIu64" jailed clauses\n",f->numJails);
fprintf(stream,"c %"PRIu64" conflicts\n",f->numConflicts);
fprintf(stream,"c %"PRIu64" learned clauses\n",f->numLearnedClauses);
fprintf(stream,"c %"PRIu64" learned clauses removed\n",f->numLearnedDeleted);
fprintf(stream,"c %"PRIu64" learned clause deletion sweeps\n",f->numSweeps);
if(!f->conf->minimizeLearnedClauses){
fprintf(stream,"c (learned clause minimisation off)\n");
}else{
fprintf(stream,"c %"PRIu64" redundant learned clause literals\n",f->numLiteralsDeleted);
}
if(!f->conf->useSelfSubsumingResolution){
fprintf(stream,"c (on-the-fly self-subsuming resolution off)\n");
}else{
fprintf(stream,"c %"PRIu64" subsumptions\n",f->numSubsumptions);
fprintf(stream,"c - %"PRIu64" original clauses\n",f->numSubsumedOrigClauses);
fprintf(stream,"c - %"PRIu64" UIPs (%2.2lf%%)\n",f->numSubsumptionUips,
(double)f->numSubsumptionUips*100./(double)f->numSubsumptions);
}
fprintf(stream,"c %"PRIu64" restarts\n",f->numRestarts);
fprintf(stream,"c %d assumptions\n",f->assumptions.size);
fprintf(stream,"c %u original clauses\n",f->origClauses.size);
}

void funcsatPrintColumnStats(FILE*stream,funcsat*f)
{
struct rusage usage;
getrusage(RUSAGE_SELF,&usage);
double uTime= ((double)usage.ru_utime.tv_sec)+
((double)usage.ru_utime.tv_usec)/1000000;
double sTime= ((double)usage.ru_stime.tv_sec)+
((double)usage.ru_stime.tv_usec)/1000000;
fprintf(stream,"Name,NumDecisions,NumPropagations,NumUfPropagations,"
"NumLearnedClauses,NumLearnedClausesRemoved,"
"NumLearnedClauseSweeps,NumConflicts,NumSubsumptions,"
"NumSubsumedOrigClauses,NumSubsumedUips,NumRestarts,"
"UserTimeSeconds,SysTimeSeconds\n");
fprintf(stream,"%s,",f->conf->name);
fprintf(stream,"%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64
",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64
",%"PRIu64",%.2lf,%.2lf\n",
f->numDecisions,
f->numProps+f->numUnitFactProps,
f->numUnitFactProps,
f->numLearnedClauses,
f->numLearnedDeleted,
f->numSweeps,
f->numConflicts,
f->numSubsumptions,
f->numSubsumedOrigClauses,
f->numSubsumptionUips,
f->numRestarts,
uTime,
sTime);
}

void funcsatBumpLitPriority(funcsat*f,literal p)
{
varBumpScore(f,fs_lit2var(p));
}















literal levelOf(funcsat*f,variable v)
{
return f->level.data[v];
}


literal fs_var2lit(variable v)
{
literal result= (literal)v;
assert((variable)result==v);
return result;
}

variable fs_lit2var(literal l)
{
if(l<0){
l= -l;
}
return(variable)l;
}

unsigned fs_lit2idx(literal l)
{

variable v= fs_lit2var(l);
v<<= 1;
v|= (l<0);
return v;
}

bool isDecision(funcsat*f,variable v)
{
return 0!=f->decisions.data[v];
}




#if 0
void singlesPrint(FILE*stream,clause*begin)
{
clause*c= begin;
if(c){
do{
clause*next= c->next[0];
clause*prev= c->prev[0];
if(!next){
fprintf(stream,"next is NULL");
return;
}
if(!prev){
fprintf(stream,"prev is NULL");
return;
}
if(next->prev[0]!=c)fprintf(stream,"n*");
if(prev->next[0]!=c)fprintf(stream,"p*");
dimacsPrintClause(stream,c);
c= next;
if(c!=begin)fprintf(stream,", ");
}while(c!=begin);
}else{
fprintf(stream,"NULL");
}
}
#endif

#if 0
void watcherPrint(FILE*stream,clause*c,uint8_t w)
{
if(!c){
fprintf(stream,"EMPTY\n");
return;
}

clause*begin= c;
literal data= c->data[w];
fprintf(stream,"watcher list containing lit %i\n",c->data[w]);
do{
uint8_t i= c->data[0]==data?0:1;
clause*next= c->next[i];
if(!next){
fprintf(stream,"next is NULL\n");
return;
}
if(!(next->prev[next->data[0]==c->data[i]?0:1]==c)){
fprintf(stream,"*");
}
dimacsPrintClause(stream,c);
fprintf(stream,"\n");
c= next;
}while(c!=begin);
}



bool watcherFind(clause*c,clause**watches,uint8_t w)
{
clause*curr= *watches,*nx,*end;
uint8_t wi;
bool foundEnd;
forEachWatchedClause(curr,c->data[w],wi,nx,end,foundEnd){
if(curr==c)return true;
}
return false;
}

void binWatcherPrint(FILE*stream,funcsat*f)
{
variable v;
unsigned i;
for(v= 1;v<=f->numVars;v++){
binvec_t*bv= f->watchesBin.data[fs_lit2idx(fs_var2lit(v))];
if(bv->size> 0){
fprintf(stream,"%5ji -> ",fs_var2lit(v));
for(i= 0;i<bv->size;i++){
literal imp= bv->data[i].implied;
fprintf(stream,"%i",imp);
if(i+1!=bv->size)fprintf(stream,", ");
}
fprintf(stream,"\n");
}

bv= f->watchesBin.data[fs_lit2idx(-fs_var2lit(v))];
if(bv->size> 0){
fprintf(stream,"%5ji -> ",-fs_var2lit(v));
for(i= 0;i<bv->size;i++){
literal imp= bv->data[i].implied;
fprintf(stream,"%i",imp);
if(i+1!=bv->size)fprintf(stream,", ");
}
fprintf(stream,"\n");
}
}
}

#endif

unsigned funcsatNumClauses(funcsat*f)
{
return f->numOrigClauses;
}

unsigned funcsatNumVars(funcsat*f)
{
return f->numVars;
}

/*:139*//*140:*/
#line 3648 "funcsat.w"

void funcsatPrintHeuristicValues(FILE*p,funcsat*f)
{
for(variable i= 1;i<=f->numVars;i++){
double*value= bh_var2act(f,i);
fprintf(p,"%u = %4.2lf\n",i,*value);
}
fprintf(p,"\n");
}

static void fs_print_state(funcsat*f,FILE*p)
{
variable i;
literal*it;
if(!p)p= stderr;

fprintf(p,"assumptions: ");
forClause(it,&f->assumptions){
fprintf(p,"%i ",*it);
}
fprintf(p,"\n");

fprintf(p,"dl %u (%"PRIu64" X, %"PRIu64" d)\n",
f->decisionLevel,f->numConflicts,f->numDecisions);

fprintf(p,"trail: ");
for(i= 0;i<f->trail.size;i++){
fprintf(p,"%2i",f->trail.data[i]);
if(f->decisions.data[fs_lit2var(f->trail.data[i])]!=0){
fprintf(p,"@%u",f->decisions.data[fs_lit2var(f->trail.data[i])]);
}
if(!head_tail_is_empty(&f->unit_facts[fs_lit2var(f->trail.data[i])])){
fprintf(p,"*");
}
#if 0
if(!clauseIsAlone(&f->jail.data[fs_lit2var(f->trail.data[i])],0)){
fprintf(p,"!");
}
#endif
fprintf(p," ");
}
fprintf(p,"\n");

fprintf(p,"model: ");
for(i= 1;i<=f->numVars;i++){
if(levelOf(f,i)!=Unassigned){
fprintf(p,"%3i@%i ",f->trail.data[f->model.data[i]],levelOf(f,i));
}
}
fprintf(p,"\n");
fprintf(p,"pq: %u (-> %i)\n",f->propq,f->trail.data[f->propq]);
}

void funcsatPrintConfig(FILE*stream,funcsat*f)
{
funcsat_config*conf= f->conf;
if(NULL!=conf->user)
fprintf(stream,"Has user data\n");
if(NULL!=conf->name)
fprintf(stream,"Name: %s\n",conf->name);

conf->usePhaseSaving?
fprintf(stream,"phsv\t"):
fprintf(stream,"NO phsv\t");

conf->useSelfSubsumingResolution?
fprintf(stream,"ssr\t"):
fprintf(stream,"NO ssr\t");

conf->minimizeLearnedClauses?
fprintf(stream,"min\t"):
fprintf(stream,"NO min\t");


if(true==conf->gc){
if(lbdSweep==conf->sweepClauses){
fprintf(stream,"gc glucose\t");
}else if(claActivitySweep==conf->sweepClauses){
fprintf(stream,"gc minisat\t");
}else{
abort();
}
}else{
fprintf(stream,"NO gc\t\t");
assert(NULL==conf->sweepClauses);
}

if(funcsatLubyRestart==conf->isTimeToRestart){
fprintf(stream,"restart luby\t");
}else if(funcsatNoRestart==conf->isTimeToRestart){
fprintf(stream,"restart none\t");
}else if(funcsatInnerOuter==conf->isTimeToRestart){
fprintf(stream,"restart inout\t");
}else if(funcsatMinisatRestart==conf->isTimeToRestart){
fprintf(stream,"restart minisat\t");
}else{
abort();
}

fprintf(stream,"learn %"PRIu32" uips\t\t",conf->numUipsToLearn);


fprintf(stream,"Jail up to %u uips\n",
conf->maxJailDecisionLevel);
#if 0
if(funcsatIsResourceLimitHit==conf->isResourceLimitHit){
fprintf(stream,"  resource hit default\n");
}else{
abort();
}

if(funcsatPreprocessNewClause==conf->preprocessNewClause){
fprintf(stream,"  UNUSED preprocess new clause default\n");
}else{
abort();
}

if(funcsatPreprocessBeforeSolve==conf->preprocessBeforeSolve){
fprintf(stream,"  UNUSED preprocess before solve default\n");
}else{
abort();
}

if(funcsatDefaultStaticActivity==conf->getInitialActivity){
fprintf(stream,"  initial activity static (default)\n");
}else{
abort();
}
#endif
}

void funcsatPrintCnf(FILE*stream,funcsat*f,bool learned)
{
fprintf(stream,"c clauses: %u original",funcsatNumClauses(f));
if(learned){
fprintf(stream,", %u learned",f->learnedClauses.size);
}
fprintf(stream,"\n");

unsigned num_assumptions= 0;
for(unsigned i= 0;i<f->assumptions.size;i++){
if(f->assumptions.data[i]!=0)
num_assumptions++;
}

fprintf(stream,"c %u assumptions\n",num_assumptions);
unsigned numClauses= funcsatNumClauses(f)+
(learned?f->learnedClauses.size:0)+num_assumptions;
fprintf(stream,"p cnf %u %u\n",funcsatNumVars(f),numClauses);
for(unsigned i= 0;i<f->assumptions.size;i++){
if(f->assumptions.data[i]!=0)
fprintf(stream,"%i 0\n",f->assumptions.data[i]);
}
dimacsPrintClauses(stream,&f->origClauses);
if(learned){
fprintf(stream,"c learned\n");
dimacsPrintClauses(stream,&f->learnedClauses);
}
}





void dimacsPrintClauses(FILE*f,vector*clauses)
{
unsigned i;
for(i= 0;i<clauses->size;i++){
clause*c= clauses->data[i];
dimacsPrintClause(f,c);
fprintf(f,"\n");
}
}

void fs_clause_print(funcsat*f,FILE*out,clause*c)
{
if(!out)out= stderr;
literal*it;
forClause(it,c){
fprintf(out,"%i@%i%s ",*it,levelOf(f,fs_lit2var(*it)),
(funcsatValue(f,*it)==true?"T":
(funcsatValue(f,*it)==false?"F":"U")));
}
}

void dimacsPrintClause(FILE*f,clause*c)
{
if(!f)f= stderr;
if(!c){
fprintf(f,"(NULL CLAUSE)");
return;
}
variable j;
for(j= 0;j<c->size;j++){
literal lit= c->data[j];
fprintf(f,"%i ",lit);
}
fprintf(f,"0");
}

void funcsatClearStats(funcsat*f)
{
f->numSweeps= 0;
f->numLearnedDeleted= 0;
f->numLiteralsDeleted= 0;
f->numProps= 0;
f->numUnitFactProps= 0;
f->numConflicts= 0;
f->numRestarts= 0;
f->numDecisions= 0;
f->numSubsumptions= 0;
f->numSubsumedOrigClauses= 0;
f->numSubsumptionUips= 0;
}

char*funcsatResultAsString(funcsat_result result)
{
switch(result){
case FS_UNKNOWN:return"UNKNOWN";
case FS_SAT:return"SATISFIABLE";
case FS_UNSAT:return"UNSATISFIABLE";
default:abort();
}
}


bool isUnitClause(funcsat*f,clause*c)
{
variable i,numUnknowns= 0,numFalse= 0;
for(i= 0;i<c->size;i++){
if(funcsatValue(f,c->data[i])==unknown){
numUnknowns++;
}else if(funcsatValue(f,c->data[i])==false){
numFalse++;
}
}
return numUnknowns==1&&numFalse==c->size-(unsigned)1;
}


int varOrderCompare(fibkey*a,fibkey*b)
{
fibkey k= *a,l= *b;
if(k> l){
return-1;
}else if(k<l){
return 1;
}else{
return 0;
}
}


static void noBumpOriginal(funcsat*f,clause*c){
UNUSED(f),UNUSED(c);
}
void bumpOriginal(funcsat*f,clause*c)
{
literal*it;
double orig_varInc= f->varInc;
f->varInc+= 2.*(1./(double)c->size);
forClause(it,c){
varBumpScore(f,fs_lit2var(*it));
}
f->varInc= orig_varInc;
}



clause*funcsatRemoveClause(funcsat*f,clause*c){
UNUSED(f),UNUSED(c);return NULL;}
#if 0
{
assert(c->isLearnt);
if(c->isReason)return NULL;

if(c->is_watched){

clause**w0= (clause**)&f->watches.data[fs_lit2idx(-c->data[0])];
clause**w1= (clause**)&f->watches.data[fs_lit2idx(-c->data[1])];
clauseUnSpliceWatch(w0,c,0);
clauseUnSpliceWatch(w1,c,1);
}else{

clause*copy= c;
clauseUnSplice(&copy,0);
}

vector*clauses;
if(c->isLearnt){
clauses= &f->learnedClauses;
}else{
clauses= &f->origClauses;
}
clauses->size--;
return c;
}
#endif


extern int DebugSolverLoop;

static char*dups(const char*str)
{
char*res;
CallocX(res,strlen(str)+1,sizeof(*str));
strcpy(res,str);
return res;
}

bool funcsatDebug(funcsat*f,char*label,int level)
{
#ifdef FUNCSAT_LOG
int*levelp;
MallocX(levelp,1,sizeof(*levelp));
*levelp= level;
hashtable_insert(f->conf->logSyms,dups(label),levelp);
return true;
#else
UNUSED(f),UNUSED(label),UNUSED(level);
return false;
#endif
}



/*:140*//*154:*/
#line 4951 "funcsat.w"

void bumpLearnedByLbd(funcsat*f,clause*c)
{
c->lbdScore= computeLbdScore(f,c);


literal*it;
forClause(it,c){
varBumpScore(f,fs_lit2var(*it));
}
varDecayActivity(f);
}

void bumpReasonByLbd(funcsat*f,clause*c)
{
literal*it;
forClause(it,c){
variable v= fs_lit2var(*it);
clause*reason= getReason(f,(literal)v);
if(reason&&reason->lbdScore==2){
varBumpScore(f,fs_lit2var(reason->data[0]));
}
}


forClause(it,c){
varBumpScore(f,fs_lit2var(*it));
}
}

void lbdDecayAfterConflict(funcsat*f)
{
UNUSED(f);

}

double funcsatDefaultStaticActivity(variable*v)
{
UNUSED(v);
return 1.f;
}

void lbdBumpActivity(funcsat*f,clause*c)
{
for(variable i= 0;i<c->size;i++){
variable v= fs_lit2var(c->data[i]);
clause*reason= getReason(f,(literal)v);
if(reason&&reason->lbdScore==2){
varBumpScore(f,v);
}
}
}

void myDecayAfterConflict(funcsat*f)
{
varDecayActivity(f);
claDecayActivity(f);
}

void bumpUnitClauseByActivity(funcsat*f,clause*c)
{
bumpClauseByActivity(f,c);
assert(c->size> 0);
}

static void minimizeClauseMinisat1(funcsat*f,clause*learned)
{
unsigned i,j;
for(i= j= 1;i<learned->size;i++){
variable x= fs_lit2var(learned->data[i]);

if(getReason(f,(literal)x)==NULL)learned->data[j++]= learned->data[i];
else{
clause*c= getReason(f,learned->data[i]);
for(unsigned k= 1;k<c->size;k++){
#if 0
if(!seen[var(c[k])]&&level(var(c[k]))> 0){
learned[j++]= learned[i];
break;
}
#endif
}
}
}

}







variable resolveWithPos(funcsat*f,literal p,clause*learn,clause*pReason)
{
variable i,c= 0;

for(i= 0;i<pReason->size;i++){
literal q= pReason->data[i];
if(f->litPos.data[fs_lit2var(q)]==UINT_MAX){
f->litPos.data[fs_lit2var(q)]= learn->size;
clausePush(learn,pReason->data[i]);
if(levelOf(f,fs_lit2var(q))==(literal)f->decisionLevel)c++;
}
}

i= f->litPos.data[fs_lit2var(p)];
f->litPos.data[fs_lit2var(learn->data[learn->size-1])]= i;
assert(!isAssumption(f,fs_lit2var(learn->data[f->litPos.data[fs_lit2var(p)]])));
learn->data[f->litPos.data[fs_lit2var(p)]]= learn->data[--learn->size];
f->litPos.data[fs_lit2var(p)]= UINT_MAX;

return c;
}

static int compareByLbdRev(const void*cl1,const void*cl2)
{
clause*c1= *(clause**)cl1;
clause*c2= *(clause**)cl2;
uint64_t s1= c1->lbdScore,s2= c2->lbdScore;
if(s1==s2)return 0;
else if(s1> s2)return-1;
else return 1;
}

static int compareByActivityRev(const void*cl1,const void*cl2)
{
clause*c1= *(clause**)cl1;
clause*c2= *(clause**)cl2;
double s1= c1->activity,s2= c2->activity;
if(s1==s2)return 0;
else if(s1> s2)return-1;
else return 1;
}



/*:154*/
