#ifndef ITEMS_H
#define ITEMS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  ITEM_PROPERTIES = 1,
  ITEM_LIST       = 2,
  ITEM_VALUE      = 3
} item_type;

typedef struct item {
  item_type type;
} item;

#define item_is_type(i,t) ((i)&&(((item*)(i))->type)==(t))

typedef struct properties properties;
typedef struct list       list;
typedef struct value      value;

char* unknown_to_text(char* b);

#define item_to_text(i,b,n) (\
  item_is_type(i,ITEM_PROPERTIES)? properties_to_text((properties*)i,b,n): (\
  item_is_type(i,ITEM_LIST)      ? list_to_text((list*)i,b,n): (\
  item_is_type(i,ITEM_VALUE)     ? value_to_text((value*)i,b,n): unknown_to_text(b) \
)))

#define item_equal(i,j) (\
  !i? !j: (\
  !j? false: (\
  item_is_type(i,ITEM_VALUE)      && item_is_type(j,ITEM_VALUE)?      value_equal((value*)i,(value*)j): (\
  item_is_type(i,ITEM_LIST)       && item_is_type(j,ITEM_LIST)?       false: (\
  item_is_type(i,ITEM_PROPERTIES) && item_is_type(j,ITEM_PROPERTIES)? false: false \
)))))

#define item_log(i) do{\
  if(!i)                              log_write("item_log(null)!\n"); else \
  if(item_is_type(i,ITEM_PROPERTIES)) properties_log((properties*)i); else \
  if(item_is_type(i,ITEM_LIST))       list_log((list*)i); else \
  if(item_is_type(i,ITEM_VALUE))      value_log((value*)i); else \
                                      log_write("item_log(not an item)!\n"); \
}while(0)

#define item_free(i) do{\
  if(!i)                              ; else \
  if(item_is_type(i,ITEM_PROPERTIES)) properties_free((properties*)i, true); else \
  if(item_is_type(i,ITEM_LIST))       list_free((list*)i, true); else \
  if(item_is_type(i,ITEM_VALUE))      value_free((value*)i); else \
                                    { log_write("item_free(not an item)! %p\n", i); __builtin_trap(); } \
}while(0)


// --------------------------------------------------------------------

/* Assoc array/dictionary/map/hash. */

properties* properties_new(uint16_t max_size);
bool        properties_set(properties* op, char* key, void* i);
void*       properties_get(properties* op, char* key);
char*       properties_key_n(properties* op, uint16_t index);
void*       properties_get_n(properties* op, uint16_t index);
void        properties_ins_setwise(properties* op, char* k, char* v);
uint16_t    properties_size(properties* op);
char*       properties_to_text(properties* op, char* b, uint16_t s);
void        properties_log(properties* op);
void*       properties_delete(properties* op, char* key);
void        properties_clear(properties* op, bool free_items);
void        properties_free(properties* op, bool free_items);
// only set free_items if you've only saved items objects and only you have used them!

// --------------------------------------------------------------------

/* List. */

list*    list_new(uint16_t max_size);
list*    list_new_from(char* text, uint16_t max_size);
list*    list_new_from_fixed(char* text);
bool     list_add(list* li, void* val);
bool     list_add_setwise(list* li, char* v);
bool     list_add_all_setwise(list* li, list* lj);
bool     list_add_value(list* li, char* v); // REVISIT list_add_string
bool     list_ins(list* li, uint16_t index, void* val);
bool     list_ins_setwise(list* li, char* v);
bool     list_set_n(list* li, uint16_t index, void* val);
void*    list_get_n(list* li, uint16_t index);
void*    list_del_n(list* li, uint16_t index);
void*    list_del_item(list* li, item* it);
uint16_t list_size(list* li);
uint16_t list_has_value(list* li, char* v); // REVISIT list_has_string
uint16_t list_find(list* li, item* it);     // REVISIT list_find_item
char*    list_to_text(list* li, char* b, uint16_t s);
void     list_log(list* li);
void     list_clear(list* li, bool free_items);
void     list_free(list* li, bool free_items);

// --------------------------------------------------------------------

/* Value: numbers, words, etc; immutable and interned. */

value* value_new(char*);
value* value_ref(value* v);
char*  value_string(value* v);
bool   value_equal(value* v1, value* v2);
bool   value_is(value* v, char* s);
void   value_log(value* v);
char*  value_to_text(value* v, char* b, uint16_t s);
void   value_free(value* v);
void   value_dump();

// --------------------------------------------------------------------

#endif


