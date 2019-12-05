#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*uart_rx_handler_t) (char*);

bool serial_init(uart_rx_handler_t cb, uint32_t baudrate);
void serial_cb(uart_rx_handler_t cb);
int  serial_recv(char* b, int l);
int  serial_printf(const char* fmt, ...);
void serial_putchar(uint32_t ch);

#endif
