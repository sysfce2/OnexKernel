#define HAS_SERIAL
#if defined(HAS_SERIAL) && defined(ONP_OVER_SERIAL)
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/log.h>
#include <channel-serial.h>

static volatile bool initialised=false;

static channel_serial_connect_cb connect_cb;

#if !defined(ONP_OVER_SERIAL)

#define SERIAL_BUFFER_SIZE 2048

static volatile char     buffer[SERIAL_BUFFER_SIZE];
static volatile uint16_t current_write=0;
static volatile uint16_t current_read=0;

static uint16_t data_available()
{
  int16_t da=(int16_t)current_write-(int16_t)current_read;
  return da >= 0? da: da+SERIAL_BUFFER_SIZE;
}
#endif

void channel_serial_on_recv(unsigned char* ch, size_t len)
{
  if(!ch){
    if(connect_cb) connect_cb("serial");
    return;
  }
#if !defined(ONP_OVER_SERIAL)
  for(uint16_t i=0; i<len; i++){
    if(data_available()==SERIAL_BUFFER_SIZE-1){
      log_write("channel serial recv buffer full!\n");
      return;
    }
    buffer[current_write++]=ch[i];
    if(current_write==SERIAL_BUFFER_SIZE) current_write=0;
  }
#endif
}

void channel_serial_init(channel_serial_connect_cb cb)
{
  connect_cb=cb;
  initialised=true;
#if defined(HAS_SERIAL) && defined(ONP_OVER_SERIAL)
  initialised=serial_init(channel_serial_on_recv, 9600);
#endif
}

uint16_t channel_serial_recv(char* b, uint16_t l)
{
#if defined(ONP_OVER_SERIAL)
  if(!initialised) return 0;
  return serial_recv(b,l);
#else
  if(!initialised || !data_available()) return 0;

  uint16_t cr=current_read;
  uint16_t da=data_available();
  uint16_t s=0;
  while(true){
    char d=buffer[cr];
    if(d=='\r' || d=='\n') break;
    da--; if(!da) return 0;
    cr++; if(cr==SERIAL_BUFFER_SIZE) cr=0;
    s++;
    if(s==l){
      log_write("channel_serial_recv can fill all of the available buffer without hitting EOL!\n");
      return 0;
    }
  }

  uint16_t size=0;
  while(data_available() && size<l){
    b[size++]=buffer[current_read++];
    if(current_read==SERIAL_BUFFER_SIZE) current_read=0;
    if(b[size-1]=='\r' || b[size-1]=='\n'){
      if(buffer[current_read]=='\n') continue;
      break;
    }
  }
  return size;
#endif
}

uint16_t channel_serial_send(char* b, uint16_t n)
{
  if(!initialised) return 0;
#if defined(HAS_SERIAL) && defined(ONP_OVER_SERIAL)
  return serial_printf("%s\n", b);
#else
  return 0;
#endif
}

