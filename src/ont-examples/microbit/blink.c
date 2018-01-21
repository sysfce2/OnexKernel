// --------------------------------------------------------------------

#include <stdint.h>
#include <variant.h>

#include <onex-kernel/gpio.h>
#include <onex-kernel/time.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/random.h>

static volatile uint16_t speed = 128;

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
    serial_printf("%dms %d %d\n", time_ms(), speed, random_byte());
    for(uint8_t l=0; l<LEDS_NUMBER; l++){ gpio_toggle(leds_list[l]); time_delay_ms(speed); }
  }
}

// --------------------------------------------------------------------
