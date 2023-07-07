
#include <stdio.h>
#include <stdarg.h>
#include <serial.h>
#include <usb_serial.h>

__attribute__((weak))
int _write(int file, char *ptr, int len)
{
  usb_serial_write((uint8_t *)ptr, len);
  return 0;
}

void serial_init(uart_rx_handler_t p_rx_handler, uint32_t baudrate)
{
}

void serial_cb(uart_rx_handler_t p_rx_handler)
{
}

int  serial_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=vfprintf(stdout, fmt, args);
  va_end(args);
  return r;
}




