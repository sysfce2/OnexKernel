
#include "onex-kernel/spi.h"
#include "onex-kernel/led-matrix.h"

const uint16_t num_leds=LED_MATRIX_WIDTH * LED_MATRIX_HEIGHT;

void led_matrix_init(){
  spi_fast_init();
}

void led_matrix_fill(uint8_t r, uint8_t g, uint8_t b){
  for(uint16_t i=0; i<num_leds; i++){
    led_matrix_array[i][0]=r;
    led_matrix_array[i][1]=g;
    led_matrix_array[i][2]=b;
  }
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

