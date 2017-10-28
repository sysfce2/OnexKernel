#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <stdbool.h>
#include <stdint.h>

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

#endif
