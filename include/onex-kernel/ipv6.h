#ifndef IPV6_H
#define IPV6_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <items.h>

typedef void (*ipv6_recv_cb)();

#define MAX_GROUPS 8

bool     ipv6_init(list* groups);
uint16_t ipv6_recv(char* group, char* buf, uint16_t l);
size_t   ipv6_printf(char* group, const char* fmt, ...);
size_t   ipv6_vprintf(char* group, const char* fmt, va_list args);
bool     ipv6_write(char* group, char* buf, uint16_t len);

#endif
