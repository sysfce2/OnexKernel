#ifndef ITEMS_H
#define ITEMS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  ITEM_PROPERTIES,
  ITEM_LIST,
  ITEM_VALUE
} item_types;

typedef struct item {
  item_types type;
} item;

#define item_type(i) (((item*)i)->type)

// --------------------------------------------------------------------

/* Assoc array/dictionary/map/hash. */

typedef struct properties properties;

properties* properties_new(uint8_t max_size);
bool        properties_set(properties* op, char* key, char* val);
char*       properties_get(properties* op, char* key);
char*       properties_get_key(properties* op, uint8_t index);
char*       properties_get_val(properties* op, uint8_t index);
uint8_t     properties_size(properties* op);
void        properties_log(properties* op);

// --------------------------------------------------------------------

/* List. */

typedef struct list list;

list*   list_new(uint8_t max_size);
bool    list_add(list* li, char* val);
char*   list_get(list* li, uint8_t index);
bool    list_set(list* li, uint8_t index, char* val);
uint8_t list_size(list* li);
char*   list_to_text(list* li, char* b, uint8_t s);
void    list_log(list* li);

// --------------------------------------------------------------------

/* Value: numbers, words, etc. */

typedef struct value value;

value* value_new(char*);
char*  value_get(value* v);
void   value_log(value* v);

// --------------------------------------------------------------------

#endif


