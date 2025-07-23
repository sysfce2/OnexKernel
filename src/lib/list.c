
#include <stdlib.h>
#include <string.h>
#include <items.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/lib.h>
#include <onex-kernel/log.h>

/*
  REVISIT: lists for MCUs, not Linux, so move to src/onl/nrf51
  and do a proper list for Linux in src/onl/unix
*/

#if defined(NRF5)
#define MAX_TEXT_LEN 512
#else
#define MAX_TEXT_LEN 4096
#endif

typedef struct list {
  item_type type;
  uint16_t max_size;
  void**  vals;
  uint16_t size;
} list;

list* list_new(uint16_t max_size) {
  if(!max_size) return 0;
  list* li=(list*)mem_alloc(sizeof(list));
  if(!li) return 0;
  li->type=ITEM_LIST;
  li->max_size=max_size;
  li->vals=(void**)mem_alloc(max_size*sizeof(void*));
  if(!li->vals) return 0;
  li->size=0;
  return li;
}

list* list_vals_new_from(char* text, uint16_t max_size) {
  list* li=list_new(max_size);
  if(!text || !(*text)) return li;
  size_t m=strlen(text)+1;
  char textcopy[m]; memcpy(textcopy, text, m); // REVISIT why copy?
  char* t=strtok(textcopy, " \n");             // REVISIT cos not reentrant?!
  while(t) {
    if(!list_add(li,value_new(t))) break;
    t=strtok(0, " \n");                        // REVISIT so use strtok_r
  }
  return li;
}

list* list_vals_new_from_fixed(char* text) {
  return list_vals_new_from(text, num_tokens(text));
}

list* list_new_from_array(void** items, uint16_t max_size){
  list* li=list_new(max_size);
  for(uint16_t i=0; i< max_size; i++){
    if(!list_add(li, items[i])) break;
  }
  return li;
}

bool list_add(list* li, void* val) {
  if(!li) return false;
  if(li->size==li->max_size) return false;
  li->vals[li->size]=val;
  li->size++;
  return true;
}

/** Append v as value if it's not already there. */
bool list_vals_set_add(list* li, char* v){
  if(!li) return false;
  if(list_vals_has(li,v)) return true;
  return list_add(li,value_new(v));
}

bool list_vals_set_add_all(list* li, list* lj){
  if(!li) return false;
  if(!lj || !lj->size) return true;
  for(uint16_t j=0; j<lj->size; j++){
    value* v=lj->vals[j];
    if(list_vals_has(li,value_string(v))) continue;
    if(!list_add(li,v)) return false;
  }
  return true;
}

bool list_vals_add(list* li, char* v){
  if(!li) return false;
  return list_add(li,value_new(v));
}

bool list_ins_n(list* li, uint16_t index, void* val){

  if(!li) return false;
  if(index==0 || index-1 > li->size) return false;
  if(li->size==li->max_size) return false;

  if(index-1 < li->size){
    for(int16_t j=li->size-1; j>=index-1; j--){
      li->vals[j+1] = li->vals[j];
    }
  }
  li->vals[index-1]=val;
  li->size++;
  return true;
}

/** Insert v as value at front if it's not already there. */
bool list_vals_set_ins(list* li, char* v){
  if(!li) return false;
  if(list_vals_has(li,v)) return true;
  return list_ins_n(li,1,value_new(v));
}

bool list_vals_set_del(list* li, char* v){
  return !!list_del_n(li, list_vals_has(li,v));
}

bool list_set_n(list* li, uint16_t index, void* val) {
  if(!li) return false;
  if(index<=0 || index>li->size) return false;
  li->vals[index-1]=val;
  return true;
}

void* list_get_n(list* li, uint16_t index) {
  if(!li) return 0;
  if(index<=0 || index>li->size) return 0;
  return li->vals[index-1];
}

void* list_del_n(list* li, uint16_t index) {

  if(!li) return 0;
  if(index==0 || index>li->size) return 0;

  void* v=li->vals[index-1];
  for(int16_t j=index-1; j < li->size-1; j++){
    li->vals[j] = li->vals[j+1];
  }
  li->size--;
  return v;
}

void* list_items_del(list* li, item* it){
  return list_del_n(li, list_items_find(li, it));
}

void* list_vals_del(list* li, char* v){
  return list_del_n(li, list_vals_has(li, v));
}

uint16_t list_size(list* li) {
  if(!li) return 0;
  return li->size;
}

bool list_vals_equal(list* l1, list* l2){
  if(!l1) return !l2;
  if(!l2) return false;
  if(l1==l2) return true;
  if(l1->size != l2->size) return false;
  for(uint16_t i=0; i<l1->size; i++){
    if(!value_equal(l1->vals[i], l2->vals[i])) return false;
  }
  return true;
}

/** Return index if list has v as a value. */
uint16_t list_vals_has(list* li, char* v){
  if(!li || !v) return 0;
  for(uint16_t j=0; j<li->size; j++){
    if(value_is(li->vals[j], v)) return j+1;
  }
  return 0;
}

uint16_t list_items_find(list* li, item* it) {
  if(!li || !it) return 0;
  for(uint16_t j=0; j<li->size; j++){
    if(item_equal((item*)li->vals[j], it)) return j+1;
  }
  return 0;
}

void list_clear(list* li, bool free_items) {
  if(!li || !li->size) return;
  if(free_items) for(uint16_t j=0; j<li->size; j++) item_free((item*)li->vals[j]);
  li->size=0;
}

void list_free(list* li, bool free_items) {
  if(!li) return;
  list_clear(li, free_items);
  mem_free(li->vals);
  mem_free(li);
}

char* list_to_text(list* li, char* b, uint16_t s) {
  if(!li){ *b = 0; return b; }
  *b=0;
  if(!li || !li->size) return b;
  uint16_t ln=0;
  for(uint16_t j=0; j<li->size; j++){
    ln+=strlen(item_to_text(li->vals[j], b+ln, s-ln)); if(ln>=s){ *b = 0; return b; }
    if(j!=li->size-1) ln+=snprintf(b+ln, s-ln, " ");   if(ln>=s){ *b = 0; return b; }
  }
  return b;
}

void list_log(list* li) {
  if(!li) return;
  char buf[MAX_TEXT_LEN];
  log_write("%s\n", list_to_text(li,buf,MAX_TEXT_LEN));
}

