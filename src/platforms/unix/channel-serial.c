
#include <onex-kernel/serial.h>
#include <channel-serial.h>

static bool initialised=false;

void channel_serial_init(channel_serial_connect_cb connect_cb)
{
  initialised=serial_init(0, 9600);
  if(initialised && connect_cb) connect_cb("serial");
}

int channel_serial_recv(char* b, int l)
{
  if(!initialised) return 0;
  return serial_recv(b,l);
}

int channel_serial_send(char* b, int n)
{
  if(!initialised) return -1;
  return serial_printf("%s\n", b);
}

