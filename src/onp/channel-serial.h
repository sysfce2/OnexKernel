#ifndef CHANNEL_SERIAL_H
#define CHANNEL_SERIAL_H

#include <onp-cb.h>

void     channel_serial_init(list* ttys, connect_cb serial_connect_cb);
uint16_t channel_serial_recv(char* buf, uint16_t size);
uint16_t channel_serial_send(char* buf, uint16_t size);

#endif
