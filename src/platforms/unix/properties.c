
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <ctype.h>
#include <onex-kernel/log.h>

typedef struct hash_item hash_item;

typedef struct properties{
  item_type          type;
  uint8_t            max_size;
  struct properties* next;
  char*              name;
  int                buckets;
  int                size;
  hash_item**        lists;
  char**             keys;
  uint8_t            i;
  int                ignorecase;
} properties;

struct hash_item{
  hash_item* next;
  char*      key;
  item*      item;
};

unsigned int string_hash(char* p)
{
  unsigned int h=0;
  while(*p) h=(h<<5)-h+tolower(*p++);
  return h;
}

#define STRCMPCASE(op, listkey, key) ((op)->ignorecase? strcasecmp((listkey), (key)): strcmp((listkey), (key)))
#define BX 30
#define WARN_SIZE(h) if((h)->size && !((h)->size % BX)) log_write("%s# %d", (h)->name, (h)->size);

properties* properties_new(uint8_t max_size)
{
  if(max_size > 128) return 0;
  char* name=""; int ignorecase=false;
  properties* op=(properties*)malloc(sizeof(properties));
  op->type=ITEM_PROPERTIES;
  op->max_size=max_size;
  op->name=name;
  op->buckets=BX;
  op->size=0;
  op->lists=malloc((op->buckets)*sizeof(hash_item*));
  op->keys=(char**)calloc(max_size,sizeof(char*));
  op->ignorecase=ignorecase;
  op->next=0;
  int i; for(i=0; i< op->buckets; i++) op->lists[i]=0;
  return op;
}

bool properties_set(properties* op, char* key, item* i)
{
  if(!(op && key && i)) return false;
  hash_item** lisp;
  lisp=&op->lists[string_hash(key) % op->buckets];
  while((*lisp) && STRCMPCASE(op, (*lisp)->key, key)){
    lisp=&(*lisp)->next;
  }
  if(!(*lisp)){
    if(op->size==op->max_size) return false;
    (*lisp)=malloc(sizeof(hash_item));
    (*lisp)->key=key;
    op->keys[op->size]=(*lisp)->key;
    (*lisp)->item=i;
    (*lisp)->next=0;
    op->size++;
    //if(op->size==op->max_size) return false;
    WARN_SIZE(op);
  }
  else{
    (*lisp)->item=i;
  }
  return true;
}


item* properties_get(properties* op, char* key)
{
  if(!(op && key)) return 0;
  hash_item* list;
  list=op->lists[string_hash(key) % op->buckets];
  while(list && STRCMPCASE(op, list->key, key)){
    list=list->next;
  }
  return list? list->item: 0;
}

item_type properties_type(properties* op, char* key)
{
  if(!op) return 0;
  item* i=properties_get(op, key);
  return i? i->type: 0;
}

char* properties_key_n(properties* op, uint8_t index)
{
  if(!op) return 0;
  if(index<=0 || index>op->size) return 0;
  return op->keys[index-1];
}

item* properties_get_n(properties* op, uint8_t index)
{
  if(!op) return 0;
  if(index<=0 || index>op->size) return 0;
  return properties_get(op, op->keys[index-1]);
}

item* properties_delete(properties* op, char* key)
{
  if(!op) return false;
  item* v=0;
  hash_item** lisp;
  hash_item*  next;
  lisp=&op->lists[string_hash(key) % op->buckets];
  while((*lisp) && STRCMPCASE(op, (*lisp)->key, key)){
    lisp=&(*lisp)->next;
  }
  if((*lisp)){
    next=(*lisp)->next;
    int j;
    for(j=0; j<op->size;   j++) if(!strcmp(op->keys[j], key)) break;
    for(   ; j<op->size-1; j++) op->keys[j] = op->keys[j+1];
    v=(*lisp)->item;
    free((*lisp));
    (*lisp)=next;
    op->size--;
    WARN_SIZE(op);
    return v;
  }
  return 0;
}


uint8_t properties_size(properties* op)
{
  if(!op) return 0;
  return op->size;
}

char* properties_to_text(properties* op, char* b, uint16_t s)
{
  if(!op){ *b = 0; return b; }
  int ln=0;
  int j;
  ln+=snprintf(b+ln, s-ln, "{\n");
  if(ln>=s){ *b = 0; return b; }
  for(j=0; j<op->size; j++){
    ln+=snprintf(b+ln, s-ln, "  %s: ", op->keys[j]);
    if(ln>=s){ *b = 0; return b; }
    ln+=strlen(item_to_text(properties_get(op, op->keys[j]), b+ln, s-ln));
    if(ln>=s){ *b = 0; return b; }
    ln+=snprintf(b+ln, s-ln, "\n");
    if(ln>=s){ *b = 0; return b; }
  }
  ln+=snprintf(b+ln, s-ln, "}\n");
  if(ln>=s){ *b = 0; return b; }
  return b;
}

void properties_log(properties* op)
{
  char buf[128];
  log_write("%s\n", properties_to_text(op,buf,128));
}


