#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include <items.h>

typedef void (*radio_recv_cb)(int8_t rssi);

bool     radio_init(radio_recv_cb cb);
uint16_t radio_recv(char* buf);                         // must be >=256 char array
size_t   radio_printf(const char* fmt, ...);            // will truncate to 254
size_t   radio_vprintf(const char* fmt, va_list args);  // will truncate to 254
bool     radio_write(char* buf, uint8_t len);           // no more than 254 chars

//void   radio_cb(radio_recv_cb cb);
//void   radio_putchar(char ch);
//bool   radio_available();
//int8_t radio_last_rssi();
//bool   radio_set_frequency(float freq);
//void   radio_set_encryption_key(uint8_t* key);
#endif
