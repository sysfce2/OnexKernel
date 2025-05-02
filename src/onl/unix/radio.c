
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <onex-kernel/radio.h>

bool radio_init(radio_recv_cb cb){
  return true;
}

uint16_t radio_read(char* buf, uint16_t size){
    return 0;
}

uint16_t radio_write(char* buf, uint16_t size) {
  return true;
}



