#ifndef CHANNEL_RADIO_H
#define CHANNEL_RADIO_H

#include <onp-cb.h>

void     channel_radio_init(connect_cb radio_connect_cb);
uint16_t channel_radio_recv(char* buf, uint16_t size);
uint16_t channel_radio_send(char* buf, uint16_t size);

#endif
