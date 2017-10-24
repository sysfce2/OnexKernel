
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


