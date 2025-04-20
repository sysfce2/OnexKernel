#ifndef CHANNEL_RADIO_H
#define CHANNEL_RADIO_H

#include <onp-cb.h>

void     channel_radio_init(connect_cb radio_connect_cb);
uint16_t channel_radio_recv(char* b, uint16_t l);
uint16_t channel_radio_send(char* b, uint16_t n);

#endif
