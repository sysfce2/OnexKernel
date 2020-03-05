
// --------------------------------------------------------------------

#if defined(NRF5)
#include <boards.h>
#include <onex-kernel/gpio.h>
#if defined(HAS_SERIAL)
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/blenus.h>
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

void on_recv(unsigned char* buf, size_t size)
{
  if(!size) return;

  log_write("on_recv (%c)\n", buf[0]);

  if(buf[0]!='t') return;

  log_write("-----------------OnexKernel tests------------------------\n");

  run_value_tests();
  run_list_tests();
  run_properties_tests();
  run_onf_tests("Onex/onex.ondb");

#if defined(NRF5)
  int failures=onex_assert_summary();
  flash_led(failures? 16: 128);
#else
  onex_assert_summary();
#endif
}

int main(void)
{
  log_init();
  time_init();
#if defined(NRF5)
#if defined(HAS_SERIAL)
  serial_init((serial_recv_cb)on_recv,0);
  blenus_init(0);
  while(1) serial_loop();
#else
  blenus_init((blenus_recv_cb)on_recv);
  while(1);
#endif
#else
  on_recv((unsigned char*)"t", 1);
#endif
}

// --------------------------------------------------------------------

