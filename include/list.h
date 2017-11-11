#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include <stdint.h>

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

#endif
