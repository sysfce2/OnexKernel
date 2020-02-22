#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

typedef void (*serial_recv_cb) (char*, uint16_t);

bool   serial_init(serial_recv_cb cb, uint32_t baudrate);
void   serial_cb(serial_recv_cb cb);
int    serial_recv(char* b, int l);
size_t serial_printf(const char* fmt, ...);
size_t serial_vprintf(const char* fmt, va_list args);
void   serial_putchar(unsigned char ch);
size_t serial_write(unsigned char* b, size_t l);
void   serial_loop();

#endif
