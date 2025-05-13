#ifndef COMPASS_H
#define COMPASS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct compass_info_t {
  int16_t x;
  int16_t y;
  int16_t z;
  int16_t o;
} compass_info_t;

bool           compass_init();
compass_info_t compass_direction();
uint8_t        compass_temperature();

#endif

