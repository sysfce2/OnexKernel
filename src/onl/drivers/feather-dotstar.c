
#include <string.h>
#include <strings.h>

#include "onex-kernel/spi.h"
#include "onex-kernel/led-matrix.h"

#include "color_table.h"

const uint16_t num_leds=LED_MATRIX_WIDTH*LED_MATRIX_HEIGHT;

uint8_t led_matrix_array[LED_MATRIX_WIDTH*LED_MATRIX_HEIGHT][3];

void led_matrix_init(){
  spi_fast_init();
}

int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

colours_rgb parse_colour_string(const char *cs) {

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

  if (cs[0] != '#') return black;

  colours_rgb color = black;

  size_t len = strlen(cs);
  if (len == 7) { // e.g., "#ff0000"
    for (int i = 1; i <= 6; i++) {

      int val = hex_to_int(cs[i]);

      if (val == -1) return black;

      if (i <= 2) color.r = (color.r << 4) | val;
      else
      if (i <= 4) color.g = (color.g << 4) | val;
      else        color.b = (color.b << 4) | val;
    }
  } else
  if (len == 4) { // e.g., "#f00"
    for (int i = 1; i <= 3; i++) {

      int val = hex_to_int(cs[i]);

      if (val == -1) return black;

      if (i == 1) color.r = (val << 4) | val;
      else
      if (i == 2) color.g = (val << 4) | val;
      else        color.b = (val << 4) | val;
    }
  }
  return color;
}

void led_matrix_fill_rgb(colours_rgb rgb){
  for(uint16_t i=0; i<num_leds; i++){
    led_matrix_array[i][0]=rgb.r;
    led_matrix_array[i][1]=rgb.g;
    led_matrix_array[i][2]=rgb.b;
  }
}

void led_matrix_fill_col(char* colour){
  led_matrix_fill_rgb(parse_colour_string(colour));
}

void led_matrix_show(){

  spi_fast_enable(true);

  uint8_t i;
  uint8_t d;

  d=0x00; for (i = 0; i < 4; i++) spi_fast_write(&d, 1);

  // BGR not RGB!
  for(uint16_t i=0; i<num_leds; i++){
    d=0xFF; spi_fast_write(&d, 1);
    d=led_matrix_array[i][2]; spi_fast_write(&d, 1);
    d=led_matrix_array[i][1]; spi_fast_write(&d, 1);
    d=led_matrix_array[i][0]; spi_fast_write(&d, 1);
  }

  // https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
  // and see Adafruit_DotStar.cpp
  d=0xFF; for (i = 0; i < ((num_leds + 15) / 16); i++) spi_fast_write(&d, 1);

  spi_fast_enable(false);
}

