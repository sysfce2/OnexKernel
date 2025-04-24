
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <onex-kernel/chunkbuf.h>

typedef struct chunkbuf {
  uint16_t    buf_size;
  char*       buffer;
  uint16_t    current_write;
  uint16_t    current_read;
} chunkbuf;

chunkbuf* chunkbuf_new(uint16_t buf_size){
  chunkbuf* cb=(chunkbuf*)mem_alloc(sizeof(chunkbuf));
  cb->buf_size=buf_size;
  cb->buffer=(char*)mem_alloc(buf_size);
  cb->current_write=0;
  cb->current_read=0;
  if(!cb) return 0;
  return cb;
}

uint16_t chunkbuf_current_size(chunkbuf* cb){
  int16_t da=((int16_t)cb->current_write) - ((int16_t)cb->current_read);
  return da >= 0? da: da+cb->buf_size;
}

uint16_t chunkbuf_write(chunkbuf* cb, char* buf, uint16_t size){
  if(size > (cb->buf_size-1) - chunkbuf_current_size(cb)){
    log_flash(1,0,0);
    return 0;
  }
  uint16_t i;
  for(i=0; i<size; i++){
    cb->buffer[cb->current_write++]=buf[i];
    if(cb->current_write==cb->buf_size) cb->current_write=0;
  }
  return i;
}

uint16_t chunkbuf_read(chunkbuf* cb, char* buf, uint16_t size, char delim){
  uint16_t i;
  for(i=0; i<size && chunkbuf_current_size(cb); i++){
    buf[i]=cb->buffer[cb->current_read++];
    if(cb->current_read==cb->buf_size) cb->current_read=0;
    if(buf[i]==delim){ i++; break; }
  }
  return i;
}

void chunkbuf_clear(chunkbuf* cb){
  cb->current_write=0;
  cb->current_read=0;
}




