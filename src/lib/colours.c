
#include <string.h>
#include <strings.h>

#include "color_table.h"

#include <onex-kernel/colours.h>

colours_rgb colours_bcs_to_rgb(colours_bcs bcs) {
  return colours_hsv_to_rgb((colours_hsv){ bcs.c, 255-bcs.s, bcs.b });
}

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

uint8_t colours_hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

colours_rgb colours_parse_string(char* cs) {

  colours_rgb black = {0, 0, 0};

  if (!cs || *cs == '\0') return black;

  for (const ColorName *entry = colorNames; entry->name; ++entry) {
      if (strcasecmp(cs, entry->name) == 0) {
          return (colours_rgb){
            (entry->value >> 16) & 0xFF,
            (entry->value >>  8) & 0xFF,
            (entry->value      ) & 0xFF
          };
      }
  }

  if(!(cs[0]=='#' || cs[0]=='%')) return black;

  uint8_t rb=0;
  uint8_t gc=0;
  uint8_t bs=0;

  size_t len = strlen(cs);
  if (len == 7) { // e.g., "#ff0000"
    for (int i = 1; i <= 6; i++) {

      int val = colours_hex_to_int(cs[i]);

      if (val == -1) return black;

      if (i <= 2) rb = (rb << 4) | val;
      else
      if (i <= 4) gc = (gc << 4) | val;
      else        bs = (bs << 4) | val;
    }
  }
  else
  if (len == 4) { // e.g., "#f00"
    for (int i = 1; i <= 3; i++) {

      int val = colours_hex_to_int(cs[i]);

      if (val == -1) return black;

      if (i == 1) rb = (val << 4) | val;
      else
      if (i == 2) gc = (val << 4) | val;
      else        bs = (val << 4) | val;
    }
  }
  if(cs[0]=='#'){
    colours_rgb rgb = { rb, gc, bs };
    return rgb;
  }
  if(cs[0]=='%'){
    colours_bcs bcs = { rb, gc, bs };
    return colours_bcs_to_rgb(bcs);
  }
  return black;
}


