
#include <stdlib.h>
#include <string.h>
#include <properties.h>

/*
  properties for MCUs, not Linux, so move to src/platforms/nrf51
  and do a proper hashtable for Linux in src/platforms/unix
*/

typedef struct properties {
  uint8_t max_size;
  char**  keys;
  char**  vals;
  uint8_t i;
} properties;

properties* properties_new(uint8_t max_size)
{
  properties* op=(properties*)calloc(1,sizeof(properties));
  op->max_size=max_size;
  op->keys=(char**)calloc(max_size,sizeof(char*));
  op->vals=(char**)calloc(max_size,sizeof(char*));
  op->i=0;
  return op;
}

bool properties_set(properties* op, char* key, char* val)
{
  if(!op) return false;
  int j;
  for(j=0; j<op->i; j++){
    if(!strcmp(op->keys[j], key)){
      op->vals[j]=val;
      return true;
    }
  }
  if(op->i==op->max_size) return false;
  op->keys[op->i]=key;
  op->vals[op->i]=val;
  op->i++;
  return true;
}

char* properties_get(properties* op, char* key)
{
  if(!op) return 0;
  int j;
  for(j=0; j<op->i; j++) if(!strcmp(op->keys[j], key)) return op->vals[j];
  return 0;
}

char* properties_get_key(properties* op, uint8_t index)
{
  if(!op) return 0;
  if(index>=op->i) return 0;
  return op->keys[index];
}

char* properties_get_val(properties* op, uint8_t index)
{
  if(!op) return 0;
  if(index>=op->i) return 0;
  return op->vals[index];
}

uint8_t properties_size(properties* op)
{
  if(!op) return 0;
  return op->i;
}


