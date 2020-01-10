
#include <string.h>

#include <onex-kernel/serial.h>
#include <channel-serial.h>

static bool initialised=false;

#if defined(NRF5)
#define SERIAL_MAX_LENGTH 192
#else
#define SERIAL_MAX_LENGTH 512
#endif

int  i=0;
char ser_buff[SERIAL_MAX_LENGTH];
int  ser_size=0;

static channel_serial_connect_cb connect_cb;

void channel_serial_on_recv(char* ch)
{
  if(*ch<' ' && *ch!='\n' && *ch!='\r') return;
  if(!ch){
    if(connect_cb) connect_cb();
    return;
  }
  if(ser_size) return; // !!
  ser_buff[i++]=*ch;
  if(i==SERIAL_MAX_LENGTH-1 || *ch=='\n' || *ch=='\r'){
    ser_size = i;
    ser_buff[i]=0;
    i=0;
  }
}

void channel_serial_init(channel_serial_connect_cb cb)
{
  initialised=serial_init(channel_serial_on_recv, 9600);
  connect_cb=cb;
}

int channel_serial_recv(char* b, int l)
{
  if(!initialised) return -1;
  if(!ser_size) return -1;
  int size=l<ser_size? l: ser_size;
  memcpy(b, ser_buff, size);
  ser_size=0;
  return size;
}

int channel_serial_send(char* b, int n)
{
  if(!initialised) return -1;
  return serial_printf("%s\n", b);
}

