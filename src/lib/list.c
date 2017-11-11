
#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <onex-kernel/log.h>

/*
  lists for MCUs, not Linux, so move to src/platforms/nrf51
  and do a proper hashtable for Linux in src/platforms/unix
*/

typedef struct list {
  uint8_t max_size;
  char**  vals;
  uint8_t i;
} list;

list* list_new(uint8_t max_size)
{
  list* li=(list*)calloc(1,sizeof(list));
  li->max_size=max_size;
  li->vals=(char**)calloc(max_size,sizeof(char*));
  li->i=0;
  return li;
}

bool list_add(list* li, char* val)
{
  if(!li) return false;
  if(li->i==li->max_size) return false;
  li->vals[li->i]=val;
  li->i++;
  return true;
}

bool list_set(list* li, uint8_t index, char* val)
{
  if(!li) return false;
  if(index<=0 || index>li->i) return false;
  li->vals[index-1]=val;
  return true;
}

char* list_get(list* li, uint8_t index)
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

void list_log(list* li)
{
  if(!li) return;
  int j;
  for(j=0; j<li->i; j++) log_write("%s ", li->vals[j]);
  log_write("\n");
}

char* list_to_text(list* li, char* b, uint8_t s)
{
  if(!li){ *b = 0; return b; }
  int ln=0;
  int j;
  for(j=0; j<li->i; j++){
    ln+=snprintf(b+ln, s-ln, "%s", li->vals[j]);
    if(j!=li->i-1) ln+=snprintf(b+ln, s-ln, " ");
    if(ln>=s){ *b = 0; return b; }
  }
  return b;
}

