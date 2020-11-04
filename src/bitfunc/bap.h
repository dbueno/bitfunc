#ifndef bitfunc_bap_h_included
#define bitfunc_bap_h_included

#include <funcsat/hashtable.h>

#include <bitfunc/mem.h>

enum {
  BAP_TYPE_FLAG_VALID  = 1,
};

enum {
  BAP_TYPE_KIND_BV,
  BAP_TYPE_KIND_MEM,
  BAP_TYPE_KIND_ARRAY,
  BAP_TYPE_KIND_STR,
};

typedef struct bap_type {
  uint8_t idx_width;
  uint8_t val_width;
  uint8_t kind:2;
  uint8_t flags:6;
} bap_type;

typedef struct bap_val
{
  size_t refcnt;
  union {
    bitvec *bv;
    memory *mem;
    char *str;
  } v;
  bap_type typ;
} bap_val;

typedef struct bap_lval
{
  char *name;
  bap_type typ;
} bap_lval;

typedef struct letctx
{
  char *name;
  bap_val *val;
} letctx;

DECLARE_HASHTABLE(bapSymbolInsert, bapSymbolSearch, bapSymbolRemove, char, bap_val)

typedef struct hashtable *symbol_table;

typedef struct bap_machine
{
  bfman *bf;
  symbol_table bindings;
  bool has_error;
  bool halted;
  bitvec *path_constraints;
} bap_machine;

bap_val *bap_val_new_bv(bitvec *bv);
bap_val *bap_val_new_array(memory *mem);
bap_val *bap_val_new_string(char *s);
bap_val *bap_val_new_bot(bap_type t);
bap_val *bap_val_ref(bap_machine *MS, bap_val *v);
bap_val *bap_val_unref(bap_machine *MS, bap_val *v);

bool bap_types_equal(bap_type *t1, bap_type *t2);
char *bap_describe_type(bap_type t);

void bap_set_binding(bap_machine *MS, char *name, bap_val *v);
void bap_clear_binding(bap_machine *MS, char *name);
bap_val *bap_lookup_ident(bap_machine *MS, char *name, bap_type t);

bap_machine *bap_create_machine(bfman *bf);
void bap_destroy_machine(bap_machine *mach);

bool bap_execute_il(bap_machine *mach, char *ilfile);

#endif /* bitfunc_bap_h_included */
