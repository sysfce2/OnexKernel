#ifndef CHANNEL_IPV6_H
#define CHANNEL_IPV6_H

#include <onp-cb.h>

void     channel_ipv6_init(list* groups, connect_cb ipv6_connect_cb);
uint16_t channel_ipv6_recv(char* group, char* buf, uint16_t size);
uint16_t channel_ipv6_send(char* group, char* buf, uint16_t size);

#endif

