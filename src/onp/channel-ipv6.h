#ifndef CHANNEL_IPV6_H
#define CHANNEL_IPV6_H

typedef void (*channel_ipv6_connect_cb)(char*);

void     channel_ipv6_init(channel_ipv6_connect_cb cb, char* group);
uint16_t channel_ipv6_recv(char* b, uint16_t l);
uint16_t channel_ipv6_send(char* b, uint16_t n);

#endif

