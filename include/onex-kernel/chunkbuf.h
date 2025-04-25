#ifndef CHUNKBUF_H
#define CHUNKBUF_H

#include <stdint.h>
#include <stdbool.h>

typedef struct chunkbuf chunkbuf;

chunkbuf* chunkbuf_new(uint16_t buf_size);
uint16_t  chunkbuf_write(chunkbuf* cb, char* buf, uint16_t size);
uint16_t  chunkbuf_read( chunkbuf* cb, char* buf, uint16_t size, int8_t delim);
uint16_t  chunkbuf_current_size(chunkbuf* cb);
void      chunkbuf_clear(chunkbuf* cb);

#endif
