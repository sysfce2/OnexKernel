
#include <string.h>

#include <onex-kernel/serial.h>
#include <channel-serial.h>

static bool initialised=false;

#define SERIAL_MAX_LENGTH 128

int  i=0;
char ser_buff[SERIAL_MAX_LENGTH];
int  ser_size=0;

void on_data(char* ch)
{
  while(*ch){
    ser_buff[i++]=*ch;
    if(i==SERIAL_MAX_LENGTH-1 || *ch=='\n'){
      ser_size = i;
      ser_buff[i]=0;
      i=0;
    }
    ch++;
  }
}

void channel_serial_init()
{
  if(initialised) return;
  serial_init(on_data, 9600);
  initialised=true;
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
