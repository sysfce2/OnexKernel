#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include <items.h>

#include <onex-kernel/channels.h>

bool     radio_init(channel_recv_cb cb);
uint16_t radio_read(char* buf, uint16_t size);
uint16_t radio_write(char* band, char* buf, uint16_t size);
int8_t   radio_last_rssi();
uint16_t radio_available();

//bool   radio_set_frequency(float freq);
//void   radio_set_encryption_key(uint8_t* key);

#endif
