
#include <stdio.h>
#include <stdarg.h>
#include <onex-kernel/serial.h>

void serial_init(uart_rx_handler_t p_rx_handler, uint32_t baudrate)
{
}

void serial_putchar(uint32_t ch)
{
  serial_printf("%c",ch);
}

int serial_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=vfprintf(stdout, fmt, args);
  va_end(args);
  return r;
}

