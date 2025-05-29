#ifndef IPV6_H
#define IPV6_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <items.h>

#include <onex-kernel/channels.h>

#define MAX_GROUPS 8

bool     ipv6_init(list* groups, channel_recv_cb cb);
uint16_t ipv6_read(char* group, char* buf, uint16_t size);
uint16_t ipv6_write(char* group, char* buf, uint16_t size);

#endif
