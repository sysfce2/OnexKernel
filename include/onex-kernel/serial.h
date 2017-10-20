#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*uart_rx_handler_t) (uint8_t byte);

void serial_init(uart_rx_handler_t p_rx_handler, uint32_t baudrate);
void serial_cb(uart_rx_handler_t p_rx_handler);
int  serial_printf(const char* fmt, ...);
void serial_putchar(uint32_t ch);

#endif
