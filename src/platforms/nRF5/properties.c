
#include <stdlib.h>
#include <string.h>
#include <items.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>

typedef struct properties {
  item_type type;
  uint16_t  max_size;
  char**    keys;
  void**    vals;
  uint16_t  size;
} properties;

properties* properties_new(uint16_t max_size)
{
  properties* op=(properties*)mem_alloc(sizeof(properties));
  if(!op) return 0;
  op->type=ITEM_PROPERTIES;
  op->max_size=max_size;
  op->keys=(char**)mem_alloc(max_size*sizeof(char*));
  if(!op->keys) return 0;
  op->vals=(void**)mem_alloc(max_size*sizeof(void*));
  if(!op->vals) return 0;
  op->size=0;
  return op;
}

bool properties_set(properties* op, char* key, void* i)
{
  if(!op) return false;
  int j;
  for(j=0; j<op->size; j++){
    if(!strcmp(op->keys[j], key)){
      op->vals[j]=i;
      return true;
    }
  }
  if(op->size==op->max_size) return false;
  op->keys[op->size]=mem_strdup(key);
  op->vals[op->size]=i;
  op->size++;
  return true;
}

void* properties_get(properties* op, char* key)
{
  if(!op) return 0;
  int j;
  for(j=0; j<op->size; j++) if(!strcmp(op->keys[j],key)) return op->vals[j];
  return 0;
}

char* properties_key_n(properties* op, uint16_t index)
{
  if(!op) return 0;
  if(index<=0 || index>op->size) return 0;
  return op->keys[index-1];
}

void* properties_get_n(properties* op, uint16_t index)
{
  if(!op) return 0;
  if(index<=0 || index > op->size) return 0;
  return op->vals[index-1];
}

void* properties_delete(properties* op, char* key)
{
  if(!op) return 0;
  void* v=0;
  int j;
  for(j=0; j<op->size; j++){
    if(!strcmp(op->keys[j], key)){
      v = op->vals[j];
      break;
    }
  }
  if(j==op->size) return 0;
  mem_freestr(op->keys[j]);
  for(; j < op->size-1; j++){
    op->keys[j] = op->keys[j+1];
    op->vals[j] = op->vals[j+1];
  }
  op->size--;
  return v;
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
  mem_free(op->vals);
  mem_free(op);
}

uint16_t properties_size(properties* op)
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
    ln+=snprintf(b+ln, s-ln, "  ");
    if(ln>=s){ *b = 0; return b; }
    ln+=snprintf(b+ln, s-ln, "%s", op->keys[j]);
    if(ln>=s){ *b = 0; return b; }
    ln+=snprintf(b+ln, s-ln, ": ");
    if(ln>=s){ *b = 0; return b; }
    ln+=strlen(item_to_text(op->vals[j], b+ln, s-ln));
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
  if(!op) return;
  char buf[256];
  log_write("%s\n", properties_to_text(op,buf,256));
}

