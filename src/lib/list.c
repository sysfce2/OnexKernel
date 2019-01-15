
#include <stdlib.h>
#include <string.h>
#include <items.h>
#include <onex-kernel/log.h>

/*
  lists for MCUs, not Linux, so move to src/platforms/nrf51
  and do a proper list for Linux in src/platforms/unix
*/

#if defined(NRF5)
#define MAX_TEXT_LEN 128
#else
#define MAX_TEXT_LEN 4096
#endif

typedef struct list {
  item_type type;
  uint16_t max_size;
  void**  vals;
  uint16_t i;
} list;

list* list_new(uint16_t max_size)
{
  list* li=(list*)calloc(1,sizeof(list));
  if(!li) return 0;
  li->type=ITEM_LIST;
  li->max_size=max_size;
  li->vals=(void**)calloc(max_size,sizeof(void*));
  if(!li->vals) return 0;
  li->i=0;
  return li;
}

bool list_add(list* li, void* val)
{
  if(!li) return false;
  if(li->i==li->max_size) return false;
  li->vals[li->i]=val;
  li->i++;
  return true;
}

bool list_set_n(list* li, uint16_t index, void* val)
{
  if(!li) return false;
  if(index<=0 || index>li->i) return false;
  li->vals[index-1]=val;
  return true;
}

void* list_get_n(list* li, uint16_t index)
{
  if(!li) return 0;
  if(index<=0 || index>li->i) return 0;
  return li->vals[index-1];
}

void* list_del_n(list* li, uint16_t index)
{
  if(!li) return 0;
  if(index<=0 || index>li->i) return 0;
  int j=index-1;
  void* v=li->vals[j];
  for(; j < li->i-1; j++){
    li->vals[j] = li->vals[j+1];
  }
  li->i--;
  return v;
}

uint16_t list_size(list* li)
{
  if(!li) return 0;
  return li->i;
}

void list_free(list* li)
{
  free(li->vals);
  free(li);
}

char* list_to_text(list* li, char* b, uint16_t s)
{
  *b=0;
  if(!li || !li->i) return b;
  int ln=0;
  int j;
  for(j=0; j<li->i; j++){
    ln+=strlen(item_to_text(li->vals[j], b+ln, s-ln));
    if(ln>=s){ *b = 0; return b; }
    if(j!=li->i-1) ln+=snprintf(b+ln, s-ln, " ");
    if(ln>=s){ *b = 0; return b; }
  }
  return b;
}

void list_log(list* li)
{
  char buf[MAX_TEXT_LEN];
  log_write("%s\n", list_to_text(li,buf,MAX_TEXT_LEN));
}

