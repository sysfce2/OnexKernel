
#include <nrfx_atomic.h>

#define BUFFER_WRITE_CONTINUE 0
#define BUFFER_WRITE_FAILED 1
#define BUFFER_WRITE_DONE 2

typedef uint8_t (*buffer_write_cb)(size_t);

#define BUFFER_SIZE 2048
static char              buffer_buffer[BUFFER_SIZE];
static volatile uint16_t buffer_current_write=0;
static volatile uint16_t buffer_current_read=0;
static volatile uint16_t buffer_data_available=0;
static volatile nrfx_atomic_u32_t buffer_in_use=false;
static volatile nrfx_atomic_u32_t buffer_chunk_in_use=false;

static buffer_write_cb   buffer_do_write;
static volatile char*    buffer_chunk;
static volatile size_t   buffer_chunk_size;

static void buffer_init(char* ch, size_t chs, buffer_write_cb wrcb)
{
  buffer_chunk=ch;
  buffer_chunk_size=chs;
  buffer_do_write=wrcb;
}

#if defined(BUFFER_CLEAR_NEEDED)
static void buffer_clear()
{
  buffer_current_write=0;
  buffer_current_read=0;
  buffer_data_available=0;
  buffer_in_use=false;
  buffer_chunk_in_use=false;
}
#endif

static void buffer_write_chunk_guard(bool done);

static size_t buffer_write(unsigned char* buf, size_t size)
{
  uint32_t reqd=false; if(!nrfx_atomic_u32_cmp_exch(&buffer_in_use, &reqd, true)) return 0;

  if(size > BUFFER_SIZE - buffer_data_available){
    buffer_in_use=false;
    buffer_write_chunk_guard(false);
    return 0;
  }
  for(int i=0; i<size; i++){
    buffer_buffer[buffer_current_write++]=buf[i];
    if(buffer_current_write==BUFFER_SIZE) buffer_current_write=0;
    buffer_data_available++;
  }

  buffer_in_use=false;
  buffer_write_chunk_guard(false);
  return size;
}

static void buffer_write_chunk_guard(bool done)
{
  if(done) buffer_chunk_in_use=false;
  if(buffer_in_use||!buffer_data_available) return;
  uint32_t reqd=false; if(!nrfx_atomic_u32_cmp_exch(&buffer_chunk_in_use, &reqd, true)) return;

  while(buffer_data_available){
    uint16_t da=buffer_data_available;
    uint16_t cr=buffer_current_read;

    uint16_t size=0;
    while(buffer_data_available && size<buffer_chunk_size){
      buffer_chunk[size++]=buffer_buffer[buffer_current_read++];
      if(buffer_current_read==BUFFER_SIZE) buffer_current_read=0;
      buffer_data_available--;
      if(buffer_chunk[size-1]=='\n') break;
    }

    uint8_t r=buffer_do_write(size);

    if(r==BUFFER_WRITE_DONE){
      break;
    }
    if(r==BUFFER_WRITE_FAILED){
      buffer_data_available=da;
      buffer_current_read=cr;
      buffer_chunk_in_use=false;
      break;
    }
  }
}

static void buffer_write_chunk()
{
  buffer_write_chunk_guard(true);
}

