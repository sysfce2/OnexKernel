
#include <onex-kernel/colours.h>

colours_rgb colours_hsv_to_rgb(colours_hsv hsv) {

  uint8_t v = hsv.v;
  colours_rgb rgb;

  if (hsv.s == 0) {
    rgb.r = rgb.g = rgb.b = v;
    return rgb;
  }

  uint16_t h6 = (uint16_t)hsv.h * 6; // scale 0–255 to 0–1530
  uint8_t region    = h6 / 256;      // 0–5
  uint8_t remainder = h6 % 256;      // 0–255

  uint8_t w, x, y;

  w = (v * (255 - ((hsv.s                   ))       )) / 255;
  x = (v * (255 - ((hsv.s * (remainder      )) / 255))) / 255;
  y = (v * (255 - ((hsv.s * (255 - remainder)) / 255))) / 255;

  switch (region) {
    case 0: rgb.r = v;  rgb.g = y; rgb.b = w; break;
    case 1: rgb.r = x;  rgb.g = v; rgb.b = w; break;
    case 2: rgb.r = w;  rgb.g = v; rgb.b = y; break;
    case 3: rgb.r = w;  rgb.g = x; rgb.b = v; break;
    case 4: rgb.r = y;  rgb.g = w; rgb.b = v; break;
    case 5: rgb.r = v;  rgb.g = w; rgb.b = x; break;
  }
  return rgb;
}

