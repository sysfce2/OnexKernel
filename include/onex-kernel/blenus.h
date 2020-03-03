#ifndef BLENUS_H
#define BLENUS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

typedef void (*blenus_recv_cb) (unsigned char*, size_t);

bool   blenus_init(blenus_recv_cb cb);
void   blenus_cb(blenus_recv_cb cb);
int    blenus_recv(char* b, int l);
size_t blenus_printf(const char* fmt, ...);
size_t blenus_vprintf(const char* fmt, va_list args);
void   blenus_putchar(unsigned char ch);
size_t blenus_write(unsigned char* b, size_t l);
void   blenus_loop();

#endif
