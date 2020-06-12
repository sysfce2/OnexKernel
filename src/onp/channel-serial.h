#ifndef CHANNEL_SERIAL_H
#define CHANNEL_SERIAL_H

typedef void (*channel_serial_connect_cb)(char*);

void     channel_serial_init(channel_serial_connect_cb cb);
uint16_t channel_serial_recv(char* b, uint16_t l);
uint16_t channel_serial_send(char* b, uint16_t n);

#endif
