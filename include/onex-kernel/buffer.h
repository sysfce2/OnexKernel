#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef bool (*buffer_write_cb)(size_t);

void   buffer_init(char* ch, size_t chs, buffer_write_cb wrcb);
void   buffer_clear();
size_t buffer_write(unsigned char* buf, size_t size);
void   buffer_write_chunk();

#endif
