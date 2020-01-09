
#include <string.h>

#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>

static bool initialised=false;

static serial_recv_cb recv_cb;

void on_serial_recv(char* b)
{
  if(recv_cb) recv_cb(b);
}

bool serial_init(serial_recv_cb cb, uint32_t baudrate)
{
  if(!initialised){
    recv_cb = cb;
    initialised=true;
  }
  return true;
}

void serial_cb(serial_recv_cb cb)
{
    recv_cb = cb;
}

extern void serial_send(char* b);

int serial_printf(const char* fmt, ...)
{
  if(!initialised) return -1;
  va_list args;
  va_start(args, fmt);
  char b[256];
  int n=vsnprintf(b, 256, fmt, args);
  serial_send(b);
  va_end(args);
  return strlen(b);
}



