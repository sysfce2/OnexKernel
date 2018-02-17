
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

#if defined(TARGET_MCU_NRF51822)
#define MAX_VALUES 128
#define MAX_TEXT_LEN 128
#else
#define MAX_VALUES 4096
#define MAX_TEXT_LEN 4096
#endif

value* value_new(char* val)
{
  if(!val) return 0;
  if(!all_values) all_values=properties_new(MAX_VALUES);
  value ourstemp;
  ourstemp.type=ITEM_VALUE;
  ourstemp.val=val;
  value* ours=(value*)properties_get_same(all_values, &ourstemp);
  if(ours) return ours;
  ours=(value*)calloc(1,sizeof(value));
  if(!ours) return 0;
  ours->type=ITEM_VALUE;
  ours->val=strdup(val);
  if(!ours->val){ free(ours); return 0; }
  if(!properties_set(all_values, ours, (item*)ours)) return 0;
;;properties_log(all_values);
  return ours;
}

char* value_string(value* v)
{
  if(!v) return 0;
  return v->val;
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
  char buf[MAX_TEXT_LEN];
  log_write("%s\n", value_to_text(v,buf,MAX_TEXT_LEN));
}


