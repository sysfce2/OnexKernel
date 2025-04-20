#ifndef CHANNEL_SERIAL_H
#define CHANNEL_SERIAL_H

#include <onp-cb.h>

void     channel_serial_init(connect_cb serial_connect_cb);
uint16_t channel_serial_recv(char* b, uint16_t l);
uint16_t channel_serial_send(char* b, uint16_t n);

#endif
