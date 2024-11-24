
#include "onex-kernel/spi.h"
#include "onex-kernel/led-matrix.h"

const uint16_t num_leds=LED_MATRIX_WIDTH * LED_MATRIX_HEIGHT;

void led_matrix_init(){
  spi_fast_init();
}

void led_matrix_show(uint8_t style){

  spi_fast_enable(true);

  uint8_t i;
  uint8_t d;

  d=0x00; for (i = 0; i < 4; i++) spi_fast_write(&d, 1);

  uint16_t n = num_leds;
  do {
    d=0xFF; spi_fast_write(&d, 1);
    for (i = 0; i < 3; i++){
      // BGR
      d=0;
      if(style==1){
        if((n%3)==0 && i==0) d=(num_leds-n)/4;
        if((n%3)==1 && i==1) d=(num_leds-n)/4;
        if((n%3)==2 && i==2) d=(num_leds-n)/4;
      }else{
        if((n%3)==0 && i==0) d=n/4;
        if((n%3)==1 && i==1) d=n/4;
        if((n%3)==2 && i==2) d=n/4;
      }
      spi_fast_write(&d, 1);
    }
  } while (--n);

  // https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
  // and see Adafruit_DotStar.cpp
  d=0xFF; for (i = 0; i < ((num_leds + 15) / 16); i++) spi_fast_write(&d, 1);

  spi_fast_enable(false);
}

