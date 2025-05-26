#ifndef COLOURS_H
#define COLOURS_H

#include <stdint.h>

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} colours_rgb;

typedef struct {
  uint8_t b;  // Brightness
  uint8_t c;  // Colour
  uint8_t s;  // Softness
} colours_bcs;

typedef struct {
  uint8_t h;
  uint8_t s;
  uint8_t v;
} colours_hsv;

colours_rgb colours_hsv_to_rgb(colours_hsv hsv);
colours_rgb colours_bcs_to_rgb(colours_bcs bcs);
colours_rgb colours_parse_string(char* cs);
uint8_t     colours_hex_to_int(char c);

#endif
