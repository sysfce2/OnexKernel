
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <onex-kernel/log.h>

/*
  lists for MCUs, not Linux, so move to src/platforms/nrf51
  and do a proper hashtable for Linux in src/platforms/unix
*/

typedef struct list {
  item_type type;
  uint8_t max_size;
  item**  vals;
  uint8_t i;
} list;

list* list_new(uint8_t max_size)
{
  list* li=(list*)calloc(1,sizeof(list));
  li->type=ITEM_LIST;
  li->max_size=max_size;
  li->vals=(item**)calloc(max_size,sizeof(item*));
  li->i=0;
  return li;
}

bool list_add(list* li, item* val)
{
  if(!li) return false;
  if(li->i==li->max_size) return false;
  li->vals[li->i]=val;
  li->i++;
  return true;
}

bool list_set_n(list* li, uint8_t index, item* val)
{
  if(!li) return false;
  if(index<=0 || index>li->i) return false;
  li->vals[index-1]=val;
  return true;
}

item* list_get_n(list* li, uint8_t index)
{
  if(!li) return 0;
  if(index<=0 || index>li->i) return 0;
  return li->vals[index-1];
}

uint8_t list_size(list* li)
{
  if(!li) return 0;
  return li->i;
}

char* list_to_text(list* li, char* b, uint8_t s)
{
  if(!li){ *b = 0; return b; }
  int ln=0;
  int j;
  for(j=0; j<li->i; j++){
    ln+=strlen(item_to_text(li->vals[j], b+ln, s-ln));
    if(j!=li->i-1) ln+=snprintf(b+ln, s-ln, " ");
    if(ln>=s){ *b = 0; return b; }
  }
  return b;
}

void list_log(list* li)
{
  char buf[128];
  log_write("%s\n", list_to_text(li,buf,128));
}

