
#include <string.h>

#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>

extern void serialSend(char* b);

static bool initialised=false;

void serial_init(uart_rx_handler_t cb, uint32_t baudrate)
{
  if(initialised) return;
  initialised=true;
}

int serial_recv(char* b, int l)
{
  if(!initialised) return -1;
  return -1;
}

int serial_printf(const char* fmt, ...)
{
  if(!initialised) return -1;
  va_list args;
  va_start(args, fmt);
  char b[256];
  int n=vsnprintf(b, 256, fmt, args);
  serialSend(b);
  va_end(args);
  return strlen(b);
}



