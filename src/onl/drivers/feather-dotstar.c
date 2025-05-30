
#include "onex-kernel/spi.h"
#include "onex-kernel/led-matrix.h"

static const uint16_t num_leds=LED_MATRIX_WIDTH*LED_MATRIX_HEIGHT;

static uint8_t led_matrix_array[LED_MATRIX_WIDTH*LED_MATRIX_HEIGHT][3];

void led_matrix_init(){
  spi_fast_init();
}

void led_matrix_fill_hsv(colours_hsv hsv){
  colours_rgb rgb = colours_hsv_to_rgb(hsv);
  led_matrix_fill_rgb(rgb);
}

void led_matrix_fill_rgb(colours_rgb rgb){
  for(uint16_t i=0; i<num_leds; i++){
    led_matrix_array[i][0]=rgb.r;
    led_matrix_array[i][1]=rgb.g;
    led_matrix_array[i][2]=rgb.b;
  }
}

void led_matrix_fill_col(char* colour){
  led_matrix_fill_rgb(colours_parse_string(colour));
}

static uint8_t scale=10;

void led_matrix_set_scale(uint8_t scale_){
  scale = scale_;
}

void led_matrix_show(){

  spi_fast_enable(true);

  uint8_t i;
  uint8_t d;

  d=0x00; for (i = 0; i < 4; i++) spi_fast_write(&d, 1);

  // BGR not RGB!
  for(uint16_t i=0; i<num_leds; i++){
    d=0xFF; spi_fast_write(&d, 1);
    d=led_matrix_array[i][2]/scale; spi_fast_write(&d, 1);
    d=led_matrix_array[i][1]/scale; spi_fast_write(&d, 1);
    d=led_matrix_array[i][0]/scale; spi_fast_write(&d, 1);
  }

  // https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
  // and see Adafruit_DotStar.cpp
  d=0xFF; for (i = 0; i < ((num_leds + 15) / 16); i++) spi_fast_write(&d, 1);

  spi_fast_enable(false);
}

