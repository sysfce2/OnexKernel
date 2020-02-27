
#include <onex-kernel/serial.h>
#include <channel-serial.h>

static bool initialised=false;

static channel_serial_connect_cb connect_cb;

void channel_serial_on_recv(unsigned char* ch, size_t len)
{
  if(!ch){
    if(connect_cb) connect_cb("serial");
    return;
  }
}

void channel_serial_init(channel_serial_connect_cb cb)
{
  connect_cb=cb;
  serial_init(channel_serial_on_recv, 9600);
  initialised=true;
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

