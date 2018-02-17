
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

static properties* all_values=0;

value* value_new(char* val)
{
  if(!val) return 0;
  if(!all_values) all_values=properties_new(100);
  value ourstemp;
  ourstemp.type=ITEM_VALUE;
  ourstemp.val=val;
  value* ours=(value*)properties_get_same(all_values, &ourstemp);
  if(ours) return ours;
  ours=(value*)calloc(1,sizeof(value));
  ours->type=ITEM_VALUE;
  ours->val=strdup(val);
  properties_set(all_values, ours, (item*)ours);

  properties_log(all_values);

  return ours;
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


