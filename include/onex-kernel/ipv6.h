#ifndef IPV6_H
#define IPV6_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

typedef void (*ipv6_recv_cb)();

bool     ipv6_init(ipv6_recv_cb cb, char* group);
uint16_t ipv6_recv(char* buf, uint16_t l);
size_t   ipv6_printf(const char* fmt, ...);
size_t   ipv6_vprintf(const char* fmt, va_list args);
bool     ipv6_write(char* buf, uint16_t len);

#endif
