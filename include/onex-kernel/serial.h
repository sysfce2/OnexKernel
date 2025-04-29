#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <items.h>

typedef void (*serial_recv_cb)(char* buf, uint16_t size);

bool     serial_init(list* ttys, uint32_t baudrate, serial_recv_cb cb);
void     serial_cb(serial_recv_cb cb);
uint16_t serial_recv(char* buf, uint16_t size);
int16_t  serial_printf(const char* fmt, ...);
int16_t  serial_vprintf(const char* fmt, va_list args);
void     serial_putchar(char ch);
uint16_t serial_write(char* buf, uint16_t size);
bool     serial_loop();

#endif
