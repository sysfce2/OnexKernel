
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <onex-kernel/radio.h>

bool radio_init(radio_recv_cb cb){
  return true;
}

uint16_t radio_write(char* buf, uint16_t size) {
  return true;
}

int16_t radio_printf(const char* fmt, ...){
  return 0;
}

int16_t radio_vprintf(const char* fmt, va_list args){
  return 0;
}

uint8_t radio_recv(char* buf) {
    return 0;
}



