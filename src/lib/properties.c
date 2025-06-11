
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <ctype.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>

typedef struct hash_item hash_item;

typedef struct properties{
  item_type          type;
  char*              func;
  uint32_t           line;
  uint16_t           max_size;
  int                buckets;
  uint16_t           size;
  hash_item**        lists;
  char**             keys;
} properties;

struct hash_item{
  hash_item* next;
  char*      key;
  void*      item;
};

unsigned int string_hash(char* p)
{
  unsigned int h=0;
  while(*p) h=(h<<5)-h+tolower(*p++);
  return h;
}

#if defined(NRF5)
#define NUM_BUX 64
#else
#define NUM_BUX 256
#endif

#define WARN_SZLG(h,k) if((h)->size && !((h)->size % NUM_BUX)) log_write("{%s:%d %s %d/%d}\n", (h)->func, (h)->line, k, (h)->size, (h)->max_size)

properties* properties_new_(char* func, uint32_t line, uint16_t max_size){
  properties* op=(properties*)mem_alloc(sizeof(properties));
  if(!op) return 0;
  op->type=ITEM_PROPERTIES;
  op->func=func;
  op->line=line;
  op->max_size=max_size;
  op->buckets=NUM_BUX;
  op->size=0;
  op->lists=mem_alloc((op->buckets)*sizeof(hash_item*));
  op->keys=(char**)mem_alloc(max_size*sizeof(char*));
  if(!op->lists || !op->keys) return 0;
  int i; for(i=0; i< op->buckets; i++) op->lists[i]=0;
  return op;
}

bool properties_set(properties* op, char* key, void* i) {
  if(!(op && key && i)) return false;
  hash_item** lisp;
  lisp=&op->lists[string_hash(key) % op->buckets];
  while((*lisp) && strcmp((*lisp)->key, key)){
    lisp=&(*lisp)->next;
  }
  if(!(*lisp)){
    if(op->size==op->max_size) return false;
    (*lisp)=mem_alloc(sizeof(hash_item));
    if(!(*lisp)) return false;
    (*lisp)->key=mem_strdup(key);
    op->keys[op->size]=(*lisp)->key;
    (*lisp)->item=i;
    (*lisp)->next=0;
    op->size++;
    WARN_SZLG(op, key);
  }
  else{
    (*lisp)->item=i;
  }
  return true;
}

void* properties_get(properties* op, char* key)
{
  if(!(op && key)) return 0;
  hash_item* list;
  list=op->lists[string_hash(key) % op->buckets];
  while(list && strcmp(list->key,key)){
    list=list->next;
  }
  return list? list->item: 0;
}

char* properties_key_n(properties* op, uint16_t index) {
  if(!op) return 0;
  if(index<=0 || index>op->size) return 0;
  return op->keys[index-1];
}

void* properties_get_n(properties* op, uint16_t index) {
  if(!op) return 0;
  if(index<=0 || index>op->size) return 0;
  return properties_get(op, op->keys[index-1]);
}

void* properties_del_n(properties* op, uint16_t index) {
  if(!op) return 0;
  if(index<=0 || index>op->size) return 0;
  return properties_delete(op, op->keys[index-1]);
}

/** Use list_set_ins() on this property key. */
void properties_set_ins(properties* op, char* k, char* v){
  if(!op) return;
  list* li = (list*)properties_get(op,k);
  if(!li) properties_set(op,k,list_vals_new_from(v,16));
  else    list_vals_set_ins(li,v);
}

void* properties_delete(properties* op, char* key)
{
  if(!op) return 0;
  void* v=0;
  hash_item** lisp;
  lisp=&op->lists[string_hash(key) % op->buckets];
  while((*lisp) && strcmp((*lisp)->key,key)){
    lisp=&(*lisp)->next;
  }
  if((*lisp)){
    int j;
    for(j=0; j<op->size; j++) if(!strcmp(op->keys[j], key)) break;
    mem_freestr(op->keys[j]);
    for(; j<op->size-1; j++) op->keys[j]=op->keys[j+1];
    hash_item* next=(*lisp)->next;
    v=(*lisp)->item;
    mem_free((*lisp));
    (*lisp)=next;
    op->size--;
    return v;
  }
  return 0;
}

void properties_clear(properties* op, bool free_items)
{
  if(!op) return;
  int sz=op->size;
  for(int j=0; j<sz; j++){
    void* v=properties_delete(op, op->keys[0]);
    if(free_items) item_free((item*)v);
  }
}

void properties_free(properties* op, bool free_items)
{
  if(!op) return;
  properties_clear(op, free_items);
  mem_free(op->keys);
  mem_free(op->lists);
  mem_free(op);
}

uint16_t properties_size(properties* op)
{
  if(!op) return 0;
  return op->size;
}

#define PROP_TO_TXT_CHK if(ln>=s){ *b = 0; return b; }

char* properties_to_text(properties* op, char* b, uint16_t s) {
  int ln=0;
  if(!op){
    ln+=snprintf(b+ln, s-ln, "{ }\n");                                     PROP_TO_TXT_CHK
    return b;
  }
  ln+=snprintf(b+ln, s-ln, "{\n");                                         PROP_TO_TXT_CHK
  for(uint16_t j=0; j<op->size; j++){
    ln+=snprintf(b+ln, s-ln, "  ");                                        PROP_TO_TXT_CHK
    ln+=snprintf(b+ln, s-ln, "%s", op->keys[j]);                           PROP_TO_TXT_CHK
    ln+=snprintf(b+ln, s-ln, ": ");                                        PROP_TO_TXT_CHK
    ln+=strlen(item_to_text(properties_get(op, op->keys[j]), b+ln, s-ln)); PROP_TO_TXT_CHK
    ln+=snprintf(b+ln, s-ln, "\n");                                        PROP_TO_TXT_CHK
  }
  ln+=snprintf(b+ln, s-ln, "}\n");                                         PROP_TO_TXT_CHK
  return b;
}

void properties_log(properties* op) {
  char buf[4096]; // REVISIT!
  log_write("%s:%u\n%s\n", op->func, (uint16_t)op->line, properties_to_text(op,buf,4096));
}


