#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <items.h>

#include <onex-kernel/channels.h>

bool     serial_init(list* ttys, uint32_t baudrate, channel_recv_cb cb);
bool     serial_ready();
uint16_t serial_read(char* buf, uint16_t size);
uint16_t serial_write(char* tty, char* buf, uint16_t size);

// following are used by log.c:
int16_t  serial_printf(const char* fmt, ...);
int16_t  serial_vprintf(const char* fmt, va_list args);

bool     serial_loop();

#endif
