
// --------------------------------------------------------------------

#if defined(NRF5)
#include <variant.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <assert.h>

extern void run_properties_tests();
extern void run_list_tests();
extern void run_value_tests();
extern void run_onf_tests(char* dbpath);

#if defined(NRF5)
const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;

void flash_led(int t)
{
    for(uint8_t l=0; l< LEDS_NUMBER; l++) gpio_mode(leds_list[l], OUTPUT);
    for(;;) for(int8_t l=LEDS_NUMBER-1; l>=0; l--){ gpio_toggle(leds_list[l]); time_delay_ms(t); }
}
#endif

int main(void) {

  time_init();
#if defined(NRF5)
  serial_init(0, 115200);
  time_delay_s(1);
#endif

  log_write("-----------------OnexKernel tests------------------------\n");

  run_value_tests();
  run_list_tests();
  run_properties_tests();
  run_onf_tests("Onex/onex.ondb");

  int failures=onex_assert_summary();

#if defined(NRF5)
  flash_led(failures? 16: 128);
#endif

  return failures;
}

// --------------------------------------------------------------------

