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

// REVISIT: properties_equal
#define item_equal(i,j) (\
  !i? !j: (\
  !j? false: (\
  item_is_type(i,ITEM_VALUE)      && item_is_type(j,ITEM_VALUE)?      value_equal((value*)i,(value*)j): (\
  item_is_type(i,ITEM_LIST)       && item_is_type(j,ITEM_LIST)?       list_vals_equal((list*)i,(list*)j): (\
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
  void* __i__ = (i); \
  if(!__i__)                              break; else \
  if(item_is_type(__i__,ITEM_PROPERTIES)) properties_free((properties*)__i__, true); else \
  if(item_is_type(__i__,ITEM_LIST))       list_free((list*)__i__, true); else \
  if(item_is_type(__i__,ITEM_VALUE))      value_free((value*)__i__); else \
  { log_write("item_free(not an item)! %p %s %d\n", __i__, __FUNCTION__, __LINE__); } \
} while(0)


// --------------------------------------------------------------------

/* Assoc array/dictionary/map/hash. */

#define properties_new(max_size) properties_new_((char*)__FUNCTION__, __LINE__, max_size)

properties* properties_new_(char* func, uint32_t line, uint16_t max_size);
bool        properties_set(properties* op, char* key, void* i);
void*       properties_get(properties* op, char* key);
char*       properties_key_n(properties* op, uint16_t index);
void*       properties_get_n(properties* op, uint16_t index);
void*       properties_del_n(properties* op, uint16_t index);
void        properties_set_ins(properties* op, char* k, char* v);
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
list*    list_vals_new_from(char* text, uint16_t max_size);
list*    list_vals_new_from_fixed(char* text);
list*    list_new_from_array(void** items, uint16_t max_size);
#define list_new_from(...) list_new_from_array((void*[]){__VA_ARGS__}, sizeof((void*[]){__VA_ARGS__}) / sizeof(void*))
bool     list_add(list* li, void* val);
bool     list_vals_add(list* li, char* v);
void*    list_vals_del(list* li, char* v);
bool     list_vals_set_add(list* li, char* v);
bool     list_vals_set_add_all(list* li, list* lj);
bool     list_vals_set_ins(list* li, char* v);
void*    list_items_del(list* li, item* it);
bool     list_ins_n(list* li, uint16_t index, void* val);
bool     list_set_n(list* li, uint16_t index, void* val);
void*    list_get_n(list* li, uint16_t index);
void*    list_del_n(list* li, uint16_t index);
uint16_t list_size(list* li);
bool     list_vals_equal(list* l1, list* l2);
uint16_t list_vals_has(list* li, char* v);
uint16_t list_items_find(list* li, item* it);
char*    list_to_text(list* li, char* b, uint16_t s);
void     list_log(list* li);
void     list_clear(list* li, bool free_items);
void     list_free(list* li, bool free_items);

// --------------------------------------------------------------------

/* Value: numbers, words, etc; immutable and interned. */

value* value_new(char*);
value* value_new_fmt(char* fmt, ...);
value* value_ref(value* v);
char*  value_string(value* v);
bool   value_equal(value* v1, value* v2);
bool   value_num_greater(value* v1, value* v2);
bool   value_is(value* v, char* s);
void   value_log(value* v);
char*  value_to_text(value* v, char* b, uint16_t s);
void   value_free(value* v);
void   value_dump();
void   value_dump_small();

// --------------------------------------------------------------------

#endif


