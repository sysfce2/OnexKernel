
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

static uint16_t size_from_read_point(chunkbuf* cb, uint16_t cr){
  int16_t da=((int16_t)cb->current_write) - ((int16_t)cr);
  return da >= 0? da: da+cb->buf_size;
}

uint16_t chunkbuf_current_size(chunkbuf* cb){
  return size_from_read_point(cb, cb->current_read);
}

bool chunkbuf_write(chunkbuf* cb, char* buf, uint16_t size){
  if(size > (cb->buf_size-1) - chunkbuf_current_size(cb)){
    return false;
  }
  uint16_t i;
  for(i=0; i<size; i++){
    cb->buffer[cb->current_write++]=buf[i];
    if(cb->current_write==cb->buf_size) cb->current_write=0;
  }
  return true;
}

#define IS_NL(c) (newline_delim && ((c)=='\r' || (c)=='\n'))

uint16_t chunkbuf_read(chunkbuf* cb, char* buf, uint16_t size, int8_t delim){
  uint16_t i;
  bool newline_delim = (delim=='\r' || delim=='\n');
  if(delim>=0){
    bool found_delim=false;
    uint16_t cr=cb->current_read;
    for(i=0; i<size && size_from_read_point(cb, cr) && !found_delim; i++){

      char c=cb->buffer[cr++];
      if(cr==cb->buf_size) cr=0;

      if(c==delim || IS_NL(c)){
        if(i+1<size && size_from_read_point(cb,cr) && IS_NL(cb->buffer[cr])){
          continue;
        }
        found_delim=true;
      }
    }
    if(!found_delim){
      if(i==size) log_flash(1,0,0); // can fill whole buffer without seeing delim
      return 0;
    }
  }
  uint8_t num_delims=0;
  for(i=0; i<size && chunkbuf_current_size(cb); i++){

    buf[i]=cb->buffer[cb->current_read++];
    if(cb->current_read==cb->buf_size) cb->current_read=0;

    if(delim<0) continue;

    if(buf[i]==delim || IS_NL(buf[i])){
      buf[i]=0; num_delims++;
      if(i+1<size && chunkbuf_current_size(cb)){
        if(IS_NL(cb->buffer[cb->current_read])) continue;
      }
      i++;
      break;
    }
  }
  return i-num_delims;
}

void chunkbuf_clear(chunkbuf* cb){
  cb->current_write=0;
  cb->current_read=0;
}




