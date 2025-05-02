#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include <items.h>

typedef void (*radio_recv_cb)(bool connect, int8_t rssi);

bool     radio_init(radio_recv_cb cb);
uint16_t radio_read(char* buf, uint16_t size);
uint16_t radio_write(char* buf, uint16_t size);

//void   radio_cb(radio_recv_cb cb);
//bool   radio_available();
//int8_t radio_last_rssi();
//bool   radio_set_frequency(float freq);
//void   radio_set_encryption_key(uint8_t* key);

#endif
