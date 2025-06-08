
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <onex-kernel/chunkbuf.h>

typedef struct chunkbuf {
  uint16_t    buf_size;
  char*       buffer;
  uint16_t    current_write;
  uint16_t    current_read;
  bool        checksumming;
} chunkbuf;

chunkbuf* chunkbuf_new(uint16_t buf_size, bool checksumming){
  chunkbuf* cb=(chunkbuf*)mem_alloc(sizeof(chunkbuf)); if(!cb) return 0;
  cb->buf_size=buf_size;
  cb->buffer=(char*)mem_alloc(buf_size); if(!cb->buffer) return 0;
  cb->current_write=0;
  cb->current_read=0;
  cb->checksumming=checksumming;
  return cb;
}

static uint16_t size_from_read_point(chunkbuf* cb, uint16_t cr){
  int16_t da=((int16_t)cb->current_write) - ((int16_t)cr);
  return da >= 0? da: da+cb->buf_size;
}

uint16_t chunkbuf_current_size(chunkbuf* cb){
  return size_from_read_point(cb, cb->current_read);
}

bool chunkbuf_writable(chunkbuf* cb, uint16_t size, int8_t delim){
  uint16_t s = (delim < 0?         size:
                (cb->checksumming? size+2:
                                   size+1));
  return s <= ((cb->buf_size-1) - chunkbuf_current_size(cb));
}

#define INC_CURRENT_WRITE cb->current_write++; if(cb->current_write==cb->buf_size) cb->current_write=0

void chunkbuf_write(chunkbuf* cb, char* buf, uint16_t size, int8_t delim){

  if(!chunkbuf_writable(cb, size, delim)) return; // shoulda checked with chunkbuf_writable()!

  uint8_t checksum=0;
  for(uint16_t i=0; i<size; i++){
    cb->buffer[cb->current_write]=buf[i]; INC_CURRENT_WRITE;
    checksum ^= buf[i];
  }
  if(delim>=0){
    if(cb->checksumming){
      cb->buffer[cb->current_write]=checksum; INC_CURRENT_WRITE;
    }
    cb->buffer[cb->current_write]=delim;    INC_CURRENT_WRITE;
  }
}

#define IS_NL(c) (newline_delim && ((c)=='\r' || (c)=='\n'))

uint16_t chunkbuf_readable(chunkbuf* cb, int8_t delim){
  if(!chunkbuf_current_size(cb)) return 0;
  if(delim < 0) return chunkbuf_current_size(cb);
  bool newline_delim = (delim=='\r' || delim=='\n');
  uint16_t cr=cb->current_read;
  uint16_t s;
  for(s=0; size_from_read_point(cb,cr); s++){
    char c=cb->buffer[cr++]; if(cr==cb->buf_size) cr=0;
    if(c==delim || IS_NL(c)){
      if(size_from_read_point(cb,cr) && IS_NL(cb->buffer[cr])){
        continue;
      }
      return s+1; // including checksum and all delims
    }
  }
  return 0; // either nothing to read or delim not found in readable
}

#define INC_CURRENT_READ cb->current_read++; if(cb->current_read==cb->buf_size) cb->current_read=0

uint16_t chunkbuf_read(chunkbuf* cb, char* buf, uint16_t size, int8_t delim){
  if(!size) return 0;
  if(!chunkbuf_current_size(cb)){ buf[0]=0; return 0; }
  uint16_t i;
  bool newline_delim = (delim=='\r' || delim=='\n');
  uint8_t num_delims=0;
  for(i=0; i<size && chunkbuf_current_size(cb); i++){

    buf[i]=cb->buffer[cb->current_read]; INC_CURRENT_READ;

    if(delim < 0) continue;

    if(buf[i]==delim || IS_NL(buf[i])){
      buf[i]=0; num_delims++;
      if(i+1<size && chunkbuf_current_size(cb)){
        if(IS_NL(cb->buffer[cb->current_read])) continue;
      }
      i++;
      break;
    }
  }
  if(delim < 0) return i;

  if(!num_delims){
    // consumed whole chunkbuf or filled whole buf but no delim! shoulda checked 1st with chunkbuf_readable()
    buf[i-1]=0;
    return 0;
  }
  if(!cb->checksumming) return i-num_delims;

  if(i <= 1+num_delims){ buf[0]=0; return 0; }

  uint8_t checksum=0;
  uint16_t j; for(j=0; j < i-1-num_delims; j++) checksum ^= buf[j];
  if(checksum != buf[j]){ buf[j]=0; return 0; }
  buf[j]=0;
  return j;  // checksum+delims consumed but zero'd out, so buffer needs to be big enough for zeroes
}

void chunkbuf_clear(chunkbuf* cb){
  cb->current_write=0;
  cb->current_read=0;
}

void chunkbuf_free(chunkbuf* cb){
  if(!cb) return;
  mem_free(cb->buffer);
  mem_free(cb);
}



