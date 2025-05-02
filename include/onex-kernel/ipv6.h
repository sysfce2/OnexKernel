#ifndef IPV6_H
#define IPV6_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <items.h>

typedef void (*ipv6_recv_cb)(bool connect, char* channel);

#define MAX_GROUPS 8

bool     ipv6_init(list* groups, ipv6_recv_cb cb);
uint16_t ipv6_read(char* group, char* buf, uint16_t l);
int16_t  ipv6_printf(char* group, const char* fmt, ...);
int16_t  ipv6_vprintf(char* group, const char* fmt, va_list args);
uint16_t ipv6_write(char* group, char* buf, uint16_t size);

#endif
