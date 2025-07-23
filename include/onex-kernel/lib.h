#ifndef LIB_H
#define LIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define ALIGN_LH_TO_32_BIT(t) t __attribute__((aligned(4)))
#define ALIGN_RH_TO_32_BIT(x) (((x) + 3) & ~3)

char* find_unescaped_colon(char* p);
char* remove_char_in_place(char* s, char remove);
char* prefix_char_in_place(char* s, char prefix, char target);
uint16_t num_tokens(char* text);
int32_t strto_int32(char* val);
bool string_is_blank(char* s);

#endif
