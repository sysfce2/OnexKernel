#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <onex-kernel/log.h>
#include <onex-kernel/ipv6.h>

bool ipv6_init(list* groups){ // ipv6_recv_cb cb??
  return true;
}

uint16_t ipv6_recv(char* group, char* buf, uint16_t l){
  return 0;
}

size_t ipv6_printf(char* group, const char* fmt, ...){
  return 0;
}

size_t ipv6_vprintf(char* group, const char* fmt, va_list args){
  return 0;
}

bool ipv6_write(char* group, char* buf, uint16_t len){
  return 0;
}

