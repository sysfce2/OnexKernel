
#include <string.h>

#include <onex-kernel/serial.h>
#include <channel-serial.h>

static bool initialised=false;

#define SERIAL_MAX_LENGTH 192

int  ser_curr=0;
char ser_buff[SERIAL_MAX_LENGTH];
int  ser_size=0;

static channel_serial_connect_cb connect_cb;

void channel_serial_on_recv(char* ch, int len)
{
  if(!ch){
    if(connect_cb) connect_cb("serial");
    return;
  }
  if(ser_size) return; // !!
  for(int i=0; i<len; i++){
    ser_buff[ser_curr++]=ch[i];
    if(ser_curr==SERIAL_MAX_LENGTH-1 || ch[i]=='\n' || ch[i]=='\r'){
      ser_size = ser_curr;
      ser_buff[ser_curr]=0;
      ser_curr=0;
    }
  }
}

void channel_serial_init(channel_serial_connect_cb cb)
{
  connect_cb=cb;
  initialised=true;
  initialised=serial_init(channel_serial_on_recv, 9600);
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

