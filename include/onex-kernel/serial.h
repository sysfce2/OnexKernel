#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*serial_rx_cb) (char*);

bool serial_init(serial_rx_cb cb, uint32_t baudrate);
void serial_cb(serial_rx_cb cb);
int  serial_recv(char* b, int l);
int  serial_printf(const char* fmt, ...);
void serial_putchar(uint32_t ch);

#endif
