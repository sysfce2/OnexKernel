#ifndef BLENUS_H
#define BLENUS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

typedef struct blenus_info_t {
  bool   connected;
  int8_t rssi;
} blenus_info_t;

typedef void (*blenus_recv_cb)(unsigned char*, size_t);
typedef void (*blenus_status_cb)(blenus_info_t);

/* Initialise BLENUS if not already done.
   first callback is data received or (0,0) if connected
   second callback is connection status
   Can use to set either callback without unsetting the other
   and without re-initialising. */
bool   blenus_init(blenus_recv_cb cb, blenus_status_cb scb);

size_t blenus_printf(const char* fmt, ...);

size_t blenus_vprintf(const char* fmt, va_list args);

void   blenus_putchar(unsigned char ch);

size_t blenus_write(unsigned char* b, size_t l);

#endif
