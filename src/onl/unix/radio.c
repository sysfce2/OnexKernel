
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <onex-kernel/radio.h>

bool radio_init(list* bands, channel_recv_cb cb){
  return true;
}

int16_t radio_read(char* buf, uint16_t size){
    return 0;
}

uint16_t radio_write(char* band, char* buf, uint16_t size) {
  return true;
}

uint16_t radio_available(){
  return 0;
}


