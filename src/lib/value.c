
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <onex-kernel/log.h>

/*
*/

typedef struct value {
  item_type type;
  char*  val;
} value;

value* value_new(char* val)
{
  value* v=(value*)calloc(1,sizeof(value));
  v->type=ITEM_VALUE;
  v->val=val;
  return v;
}

char* value_string(value* v)
{
  if(!v) return 0;
  return v->val;
}

bool value_set(value* v, char* val)
{
  v->val=val;
  return true;
}

char* value_to_text(value* v, char* b, uint16_t s)
{
  if(!v){ *b = 0; return b; }
  int ln=snprintf(b,s,"%s",v->val);
  if(ln>=s){ *b = 0; return b; }
  return b;
}

void value_log(value* v)
{
  char buf[128];
  log_write("%s\n", value_to_text(v,buf,128));
}


