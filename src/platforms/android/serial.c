
#include <string.h>

#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>

static bool initialised=false;

static uart_rx_handler_t rx_handler;

extern void serial_send(char* b);

void on_serial_recv(char* b)
{
  if(rx_handler) rx_handler(b);
}

void serial_init(uart_rx_handler_t cb, uint32_t baudrate)
{
  if(initialised) return; initialised=true;

  rx_handler = cb;
}

void serial_cb(uart_rx_handler_t cb)
{
    rx_handler = cb;
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
  serial_send(b);
  va_end(args);
  return strlen(b);
}



