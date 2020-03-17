// --------------------------------------------------------------------

#include <stdint.h>
#include <variant.h>

#include <onex-kernel/gpio.h>
#include <onex-kernel/time.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/random.h>

static uint16_t speed = 128;

static void serial_received(char* ch)
{
  if(*ch=='o') speed/=2;
  if(*ch=='i') speed*=2;
  if(!speed)  speed=1;
}

const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;

int main()
{
  for(uint8_t l=0; l< LEDS_NUMBER; l++) gpio_mode(leds_list[l], OUTPUT);
  serial_init(serial_received, 9600);
  time_init();

  serial_printf("Type 'o' or 'i'\n");
  for(;;){
    serial_printf("%dms %d %d\n", time_ms(), speed, random_ish_byte());
    gpio_set(leds_list[3], 0);
    gpio_set(leds_list[10], 0);
    time_delay_ms(speed);
    gpio_set(leds_list[3], 1);
    gpio_set(leds_list[10], 1);
    time_delay_ms(speed);
/*
    for(uint8_t l=0; l<LEDS_NUMBER; l++){ gpio_toggle(leds_list[l]); time_delay_ms(speed); }

    for(uint8_t r=0; r<LEDS_NUMBER; r++){
      gpio_set(leds_list[r], 0);
      for(uint8_t c=0; c<LEDS_NUMBER; c++){
        serial_printf("row %d col %d\n", r, c);
        gpio_set(leds_list[c], 0);
        time_delay_ms(speed);
        gpio_set(leds_list[c], 1);
      }
      gpio_set(leds_list[r], 1);
    }
*/
  }
}

// --------------------------------------------------------------------
