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

#define item_is_type(i,t) ((((item*)(i))->type)==(t))

typedef struct properties properties;
typedef struct list       list;
typedef struct value      value;

#define item_to_text(i,b,n) (\
  item_is_type(i,ITEM_PROPERTIES)? properties_to_text((properties*)i,b,n): (\
  item_is_type(i,ITEM_LIST)      ? list_to_text((list*)i,b,n): (\
  item_is_type(i,ITEM_VALUE)     ? value_to_text((value*)i,b,n): "" \
)))

#define item_log(i) do{\
  if(item_is_type(i,ITEM_PROPERTIES)) properties_log((properties*)i); else \
  if(item_is_type(i,ITEM_LIST))       list_log((list*)i); else \
  if(item_is_type(i,ITEM_VALUE))      value_log((value*)i); \
}while(0)


// --------------------------------------------------------------------

/* Assoc array/dictionary/map/hash. */

properties* properties_new(uint8_t max_size);
bool        properties_set_item(properties* op, char* key, item* i);
item_type   properties_type(properties* op, char* key);
bool        properties_set(properties* op, char* key, char* val);
char*       properties_get(properties* op, char* key);
char*       properties_get_key(properties* op, uint8_t index);
char*       properties_get_val(properties* op, uint8_t index);
uint8_t     properties_size(properties* op);
char*       properties_to_text(properties* op, char* b, uint8_t s);
void        properties_log(properties* op);

// --------------------------------------------------------------------

/* List. */

list*   list_new(uint8_t max_size);
bool    list_add(list* li, item* val);
item*   list_get(list* li, uint8_t index);
bool    list_set(list* li, uint8_t index, item* val);
uint8_t list_size(list* li);
char*   list_to_text(list* li, char* b, uint8_t s);
void    list_log(list* li);

// --------------------------------------------------------------------

/* Value: numbers, words, etc. */

value* value_new(char*);
char*  value_get(value* v);
void   value_log(value* v);
char*  value_to_text(value* v, char* b, uint8_t s);

// --------------------------------------------------------------------

#endif


