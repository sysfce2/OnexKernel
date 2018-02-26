
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <onex-kernel/log.h>

typedef struct properties {
  item_type type;
  uint16_t  max_size;
  value**   keys;
  void**    vals;
  uint16_t  size;
} properties;

properties* properties_new(uint16_t max_size)
{
  properties* op=(properties*)calloc(1,sizeof(properties));
  if(!op) return 0;
  op->type=ITEM_PROPERTIES;
  op->max_size=max_size;
  op->keys=(value**)calloc(max_size,sizeof(value*));
  op->vals=(void**)calloc(max_size,sizeof(void*));
  if(!op->keys || !op->vals) return 0;
  op->size=0;
  return op;
}

bool properties_set(properties* op, value* key, void* i)
{
  if(!op) return false;
  int j;
  for(j=0; j<op->size; j++){
    if(op->keys[j]==key){
      op->vals[j]=i;
      return true;
    }
  }
  if(op->size==op->max_size) return false;
  op->keys[op->size]=key;
  op->vals[op->size]=i;
  op->size++;
  return true;
}

void* properties_get(properties* op, value* key)
{
  if(!op) return 0;
  int j;
  for(j=0; j<op->size; j++) if(op->keys[j]==key) return op->vals[j];
  return 0;
}

void* properties_get_same(properties* op, value* key)
{
  if(!op) return 0;
  int j;
  for(j=0; j<op->size; j++) if(!strcmp(value_string(op->keys[j]), value_string(key))) return op->vals[j];
  return 0;
}

value* properties_key_n(properties* op, uint16_t index)
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

void* properties_delete(properties* op, value* key)
{
  if(!op) return false;
  void* v=0;
  int j;
  for(j=0; j<op->size; j++){
    if(op->keys[j]==key){
      v = op->vals[j];
      break;
    }
  }
  if(j==op->size) return 0;
  for(; j < op->size-1; j++){
    op->keys[j] = op->keys[j+1]; // reference--
    op->vals[j] = op->vals[j+1];
  }
  op->size--;
  return v;
}

void properties_clear(properties* op, bool freeItems)
{
  int sz=op->size;
  for(int j=0; j<sz; j++){
    void* v=properties_delete(op, op->keys[0]);
    if(freeItems && v) free(v);
  }
}

void properties_free(properties* op)
{
  free(op->keys);
  free(op->vals);
  free(op);
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
    ln+=strlen(value_to_text(op->keys[j], b+ln, s-ln));
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
  char buf[128];
  log_write("%s\n", properties_to_text(op,buf,128));
}

