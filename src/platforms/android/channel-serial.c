
#include <string.h>

#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>
#include <channel-serial.h>

static bool initialised=false;

#define SERIAL_BUFFER_SIZE 512

static char buffer[SERIAL_BUFFER_SIZE];
static int  current_write=0;
static int  current_read=0;
static int  data_available=0;

static channel_serial_connect_cb connect_cb;

void channel_serial_on_recv(char* ch, int len)
{
  if(!ch){
    if(connect_cb) connect_cb("serial");
    return;
  }
  for(int i=0; i<len; i++){
    if(data_available==SERIAL_BUFFER_SIZE){
      log_write("channel serial buffer full!\n");
      return;
    }
    buffer[current_write++]=ch[i];
    if(current_write==SERIAL_BUFFER_SIZE) current_write=0;
    data_available++;
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
  if(!initialised) return 0;
  int size=0;
  while(data_available && size<l){
    b[size++]=buffer[current_read++];
    if(current_read==SERIAL_BUFFER_SIZE) current_read=0;
    data_available--;
    if(b[size-1]=='\r' || b[size-1]=='\n'){
      if(buffer[current_read]=='\n') continue;
      break;
    }
  }
  return size;
}

int channel_serial_send(char* b, int n)
{
  if(!initialised) return -1;
  return serial_printf("%s\n", b);
}

