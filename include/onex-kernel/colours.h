#ifndef COLOURS_H
#define COLOURS_H

#include <stdint.h>

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} colours_rgb;

typedef struct {
  uint8_t h;
  uint8_t s;
  uint8_t v;
} colours_hsv;

colours_rgb colours_hsv_to_rgb(colours_hsv hsv);

#endif
