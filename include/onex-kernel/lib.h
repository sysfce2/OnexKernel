#ifndef LIB_H
#define LIB_H

#include <stdint.h>

char* find_unescaped_colon(char* p);
char* remove_char_in_place(char* s, char remove);
char* prefix_char_in_place(char* s, char prefix, char target);
uint16_t num_tokens(char* text);

#endif
