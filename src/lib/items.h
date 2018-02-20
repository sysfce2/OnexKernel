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

#define item_to_text(i,b,n) (\
  item_is_type(i,ITEM_PROPERTIES)? properties_to_text((properties*)i,b,n): (\
  item_is_type(i,ITEM_LIST)      ? list_to_text((list*)i,b,n): (\
  item_is_type(i,ITEM_VALUE)     ? value_to_text((value*)i,b,n): "" \
)))

#define item_log(i) do{\
  if(!i)                              log_write("item_log(null)!\n"); else \
  if(item_is_type(i,ITEM_PROPERTIES)) properties_log((properties*)i); else \
  if(item_is_type(i,ITEM_LIST))       list_log((list*)i); else \
  if(item_is_type(i,ITEM_VALUE))      value_log((value*)i); else \
                                      log_write("item_log(not an item)!\n"); \
}while(0)


// --------------------------------------------------------------------

/* Assoc array/dictionary/map/hash. */

properties* properties_new(uint16_t max_size);
bool        properties_set(properties* op, value* key, void* i);
void*       properties_get(properties* op, value* key);
void*       properties_get_same(properties* op, value* key);
value*      properties_key_n(properties* op, uint16_t index);
void*       properties_get_n(properties* op, uint16_t index);
uint16_t    properties_size(properties* op);
char*       properties_to_text(properties* op, char* b, uint16_t s);
void        properties_log(properties* op);
void*       properties_delete(properties* op, value* key);

// --------------------------------------------------------------------

/* List. */

list*    list_new(uint16_t max_size);
bool     list_add(list* li, void* val);
bool     list_set_n(list* li, uint16_t index, void* val);
void*    list_get_n(list* li, uint16_t index);
void*    list_del_n(list* li, uint16_t index);
uint16_t list_size(list* li);
char*    list_to_text(list* li, char* b, uint16_t s);
void     list_log(list* li);

// --------------------------------------------------------------------

/* Value: numbers, words, etc; immutable and interned. */

value* value_new(char*);
char*  value_string(value* v);
void   value_log(value* v);
char*  value_to_text(value* v, char* b, uint16_t s);

// --------------------------------------------------------------------

#endif


