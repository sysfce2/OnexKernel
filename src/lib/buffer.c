
typedef bool (*buffer_write_cb)(size_t);

#define BUFFER_SIZE 1024
static char              buffer_buffer[BUFFER_SIZE];
static volatile uint16_t buffer_current_write=0;
static volatile uint16_t buffer_current_read=0;
static volatile uint16_t buffer_data_available=0;
static volatile bool     buffer_in_use=false;
static volatile bool     buffer_chunk_in_use=false;
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

static bool buffer_drop_oldest_line()
{
  while(buffer_data_available){
    char ch=buffer_buffer[buffer_current_read++];
    if(buffer_current_read==BUFFER_SIZE) buffer_current_read=0;
    buffer_data_available--;
    if(ch=='\n') return true;
  }
  return false;
}

static void buffer_write_chunk_guard(bool done);

static size_t buffer_write(unsigned char* buf, size_t size)
{
  if(buffer_in_use) return 0;
  buffer_in_use=true;
  while(size > BUFFER_SIZE - buffer_data_available && buffer_drop_oldest_line());
  if(   size > BUFFER_SIZE - buffer_data_available){
#define LOG_TO_THIS_ONE
#if !defined(LOG_TO_THIS_ONE)
    log_write("buf_wr size %d\n", size);
#endif
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
  if(buffer_chunk_in_use||buffer_in_use||!buffer_data_available) return;
  buffer_chunk_in_use=true;

  uint16_t da=buffer_data_available;
  uint16_t cr=buffer_current_read;

  uint16_t size=0;
  while(buffer_data_available && size<buffer_chunk_size){
    buffer_chunk[size++]=buffer_buffer[buffer_current_read++];
    if(buffer_current_read==BUFFER_SIZE) buffer_current_read=0;
    buffer_data_available--;
    if(buffer_chunk[size-1]=='\n') break;
  }

  bool ok=buffer_do_write(size);

  if(ok) return;

  buffer_chunk_in_use=false;
  buffer_data_available=da;
  buffer_current_read=cr;
}

static void buffer_write_chunk()
{
  buffer_write_chunk_guard(true);
}

