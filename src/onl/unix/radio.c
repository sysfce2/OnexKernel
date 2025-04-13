
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <onex-kernel/radio.h>

bool radio_init(radio_recv_cb cb){
  return true;
}

bool radio_write(char* buf, uint8_t len) {
  return true;
}

size_t radio_printf(const char* fmt, ...){
  return 0;
}

size_t radio_vprintf(const char* fmt, va_list args){
  return 0;
}

uint16_t radio_recv(char* buf) {
    return 0;
}



