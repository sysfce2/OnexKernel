
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <onex-kernel/log.h>

/*
*/

typedef struct value {
  item_types type;
  char*  val;
} value;

value* value_new(char* vs)
{
  value* v=(value*)calloc(1,sizeof(value));
  v->type=ITEM_VALUE;
  v->val=vs;
  return v;
}

char* value_get(value* v)
{
  if(!v) return 0;
  return v->val;
}

void value_log(value* v)
{
  if(!v) return;
  log_write("%s\n", v->val);
}

