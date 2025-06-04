#ifndef CHUNKBUF_H
#define CHUNKBUF_H

#include <stdint.h>
#include <stdbool.h>

typedef struct chunkbuf chunkbuf;

chunkbuf* chunkbuf_new(uint16_t buf_size);
void      chunkbuf_write(chunkbuf* cb, char* buf, uint16_t size, int8_t delim);
uint16_t  chunkbuf_read( chunkbuf* cb, char* buf, uint16_t size, int8_t delim);
uint16_t  chunkbuf_current_size(chunkbuf* cb);
uint16_t  chunkbuf_writable(chunkbuf* cb);
uint16_t  chunkbuf_readable(chunkbuf* cb, int8_t delim);
void      chunkbuf_clear(chunkbuf* cb);
void      chunkbuf_free(chunkbuf* cb);

#endif
