#ifndef RADIO_H
#define RADIO_H

typedef void (*radio_recv_cb)(size_t, int8_t);

bool     radio_init(radio_recv_cb cb);
uint16_t radio_recv(unsigned char* buf, size_t maxlen);
bool     radio_write(unsigned char* buf, uint8_t len);

//void   radio_cb(radio_recv_cb cb);
//size_t radio_printf(const char* fmt, ...);
//size_t radio_vprintf(const char* fmt, va_list args);
//void   radio_putchar(unsigned char ch);
//bool   radio_available();
//int8_t radio_last_rssi();
//bool   radio_set_frequency(float freq);
//void   radio_set_encryption_key(uint8_t* key);
#endif
