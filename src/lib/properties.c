
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <onex-kernel/log.h>

/*
  properties for MCUs, not Linux, so move to src/platforms/nrf51
  and do a proper hashtable for Linux in src/platforms/unix
*/

typedef struct properties {
  item_type type;
  uint8_t max_size;
  char**  keys;
  item**  vals;
  uint8_t i;
} properties;

properties* properties_new(uint8_t max_size)
{
  properties* op=(properties*)calloc(1,sizeof(properties));
  op->type=ITEM_PROPERTIES;
  op->max_size=max_size;
  op->keys=(char**)calloc(max_size,sizeof(char*));
  op->vals=(item**)calloc(max_size,sizeof(item*));
  op->i=0;
  return op;
}

bool properties_set(properties* op, char* key, item* i)
{
  if(!op) return false;
  int j;
  for(j=0; j<op->i; j++){
    if(!strcmp(op->keys[j], key)){
      op->vals[j]=i;
      return true;
    }
  }
  if(op->i==op->max_size) return false;
  op->keys[op->i]=key;
  op->vals[op->i]=i;
  op->i++;
  return true;
}

item* properties_get(properties* op, char* key)
{
  if(!op) return 0;
  int j;
  for(j=0; j<op->i; j++) if(!strcmp(op->keys[j], key)) return op->vals[j];
  return 0;
}

item_type properties_type(properties* op, char* key)
{
  if(!op) return 0;
  int j;
  for(j=0; j<op->i; j++) if(!strcmp(op->keys[j], key)) return (op->vals[j])->type;
  return 0;
}

char* properties_key_n(properties* op, uint8_t index)
{
  if(!op) return 0;
  if(index<=0 || index>op->i) return 0;
  return op->keys[index-1];
}

item* properties_get_n(properties* op, uint8_t index)
{
  if(!op) return 0;
  if(index<=0 || index > op->i) return 0;
  return op->vals[index-1];
}

item* properties_delete(properties* op, char* key)
{
  if(!op) return false;
  item* v=0;
  int j;
  for(j=0; j<op->i; j++){
    if(!strcmp(op->keys[j], key)){
      v = op->vals[j];
      break;
    }
  }
  if(j==op->i) return 0;
  for(; j < op->i-1; j++){
    op->keys[j] = op->keys[j+1];
    op->vals[j] = op->vals[j+1];
  }
  op->i--;
  return v;
}

uint8_t properties_size(properties* op)
{
  if(!op) return 0;
  return op->i;
}

char* properties_to_text(properties* op, char* b, uint8_t s)
{
  if(!op){ *b = 0; return b; }
  int ln=0;
  int j;
  ln+=snprintf(b+ln, s-ln, "{\n");
  if(ln>=s){ *b = 0; return b; }
  for(j=0; j<op->i; j++){
    ln+=snprintf(b+ln, s-ln, "  %s: ", op->keys[j]);
    ln+=strlen(item_to_text(op->vals[j], b+ln, s-ln));
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

