
#include <onex-kernel/serial.h>
#include <channel-serial.h>

static bool initialised=false;

void channel_serial_init(){
  if(initialised) return;
  serial_init(0, 9600);
  initialised=true;
}

int channel_serial_recv(char* b, int l)
{
  if(!initialised) return -1;
  return serial_recv(b,l);
}

int channel_serial_send(char* b, int n)
{
  if(!initialised) return -1;
  return serial_printf("%s\n", b);
}

