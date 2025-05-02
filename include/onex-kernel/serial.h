#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <items.h>

typedef void (*serial_recv_cb)(bool connect, char* tty);

bool     serial_init(list* ttys, uint32_t baudrate, serial_recv_cb cb);
uint16_t serial_read(char* buf, uint16_t size);
uint16_t serial_write(char* buf, uint16_t size);

// following are used by log.c:
int16_t  serial_printf(const char* fmt, ...);
int16_t  serial_vprintf(const char* fmt, va_list args);

bool     serial_loop();

#endif
