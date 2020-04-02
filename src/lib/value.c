
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <onex-kernel/log.h>

char* unknown_to_text(char* b){ *b='?'; *(b+1)=0; return b; }

#if defined(NRF5)
char* strdup(const char* s)
{
  size_t len=strlen(s)+1;
  char* r=malloc(len);
  if(r) memcpy(r,s,len);
  return r;
}
#endif

typedef struct value {
  item_type type;
  char*  val;
} value;

static properties* all_values=0;

#if defined(NRF5)
#define MAX_VALUES 128
#define MAX_TEXT_LEN 64
#else
#define MAX_VALUES 4096
#define MAX_TEXT_LEN 4096
#endif

value* value_new(char* val)
{
  if(!val) return 0;
  if(!all_values) all_values=properties_new(MAX_VALUES);
  value* ours=(value*)properties_get_str(all_values, val);
  if(ours) return ours;
  ours=(value*)calloc(1,sizeof(value));
  if(!ours) return 0;
  ours->type=ITEM_VALUE;
  ours->val=strdup(val);
  if(!ours->val){ free(ours); return 0; }
  if(!properties_set(all_values, ours, ours)) return 0;
  return ours;
}

char* value_string(value* v)
{
  if(!v) return 0;
  return v->val;
}

bool value_equal(value* v1, value* v2)
{
  if(!v1) return !v2;
  if(!v2) return false;
  return v1->val == v2->val;
}

bool value_is(value* v, char* s)
{
  if(!v) return !s;
  return !strcmp(v->val, s);
}

void value_free(value* v)
{
  value* w=(value*)properties_delete(all_values, v);
  free(w->val);
  free(w);
}

char* value_to_text(value* v, char* b, uint16_t s)
{
  if(!v){ *b = 0; return b; }
  int ln=snprintf(b,s,"%s",v->val);
  if(ln>=s){ *b = 0; return b; }
  if(b[ln-1]==':'){
    ln++;
    if(ln>=s){ *b = 0; return b; }
    b[ln-2]='\\';
    b[ln-1]=':';
    b[ln]=0;
  }
  return b;
}

void value_log(value* v)
{
  char buf[MAX_TEXT_LEN];
  log_write("%s\n", value_to_text(v,buf,MAX_TEXT_LEN));
}

void value_dump()
{
  properties_log(all_values);
}

