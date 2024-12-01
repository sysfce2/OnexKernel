#ifndef CHANNEL_RADIO_H
#define CHANNEL_RADIO_H

typedef void (*channel_radio_connect_cb)(char*);

void     channel_radio_init(channel_radio_connect_cb cb);
uint16_t channel_radio_recv(char* b, uint16_t l);
uint16_t channel_radio_send(char* b, uint16_t n);

#endif
