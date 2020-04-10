
#include <onex-kernel/buffer.h>

#define BUFFER_SIZE 1024
static char buffer[BUFFER_SIZE];
static volatile uint16_t current_write=0;
static volatile uint16_t current_read=0;
static volatile uint16_t data_available=0;
static volatile bool buffer_in_use=false;
static volatile bool chunk_in_use=false;

static buffer_write_cb write_cb;

static char* chunk;
static size_t chunk_size;

void buffer_init(char* ch, size_t chs, buffer_write_cb wrcb)
{
  chunk=ch;
  chunk_size=chs;
  write_cb=wrcb;
}

void buffer_clear()
{
  current_write=0;
  current_read=0;
  data_available=0;
  buffer_in_use=false;
  chunk_in_use=false;
}

static bool drop_oldest_line()
{
  while(data_available){
    char ch=buffer[current_read++];
    if(current_read==BUFFER_SIZE) current_read=0;
    data_available--;
    if(ch=='\n') return true;
  }
  return false;
}

static void buffer_write_chunk_guard(bool done);

size_t buffer_write(unsigned char* buf, size_t size)
{
  if(buffer_in_use) return 0;
  buffer_in_use=true;
  while(size > BUFFER_SIZE - data_available && drop_oldest_line());
  if(   size > BUFFER_SIZE - data_available){
#define LOG_TO_THIS_ONE
#if !defined(LOG_TO_THIS_ONE)
    log_write("buf_wr size %d\n", size);
#endif
    buffer_in_use=false;
    buffer_write_chunk_guard(false);
    return 0;
  }
  for(int i=0; i<size; i++){
    buffer[current_write++]=buf[i];
    if(current_write==BUFFER_SIZE) current_write=0;
    data_available++;
  }
  buffer_in_use=false;
  buffer_write_chunk_guard(false);
  return size;
}

static void buffer_write_chunk_guard(bool done)
{
  if(done) chunk_in_use=false;
  if(chunk_in_use||buffer_in_use||!data_available) return;
  chunk_in_use=true;

  uint16_t da=data_available;
  uint16_t cr=current_read;

  uint16_t size=0;
  while(data_available && size<chunk_size){
    chunk[size++]=buffer[current_read++];
    if(current_read==BUFFER_SIZE) current_read=0;
    data_available--;
    if(chunk[size-1]=='\n') break;
  }

  bool ok=write_cb(size);

  if(ok) return;

  chunk_in_use=false;
  data_available=da;
  current_read=cr;
}

void buffer_write_chunk()
{
  buffer_write_chunk_guard(true);
}

