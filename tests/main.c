
// --------------------------------------------------------------------

#if defined(NRF5)
#include <boards.h>
#if defined(BOARD_PINETIME)
#include <nrf_gfx.h>
#endif
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

#if defined(BOARD_PCA10059)
const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;

void flash_led(int t)
{
    for(uint8_t l=0; l< LEDS_NUMBER; l++) gpio_mode(leds_list[l], OUTPUT);
    for(;;) for(int8_t l=LEDS_NUMBER-1; l>=0; l--){ gpio_toggle(leds_list[l]); time_delay_ms(t); }
}
#elif defined(BOARD_PINETIME)
extern const nrf_lcd_t nrf_lcd_st7789;
extern const nrf_gfx_font_desc_t orkney_24ptFontInfo;
static const nrf_lcd_t * p_lcd = &nrf_lcd_st7789;
static const nrf_gfx_font_desc_t * p_font = &orkney_24ptFontInfo;
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
#if defined(BOARD_PCA10059)
  flash_led(failures? 16: 128);
#elif defined(BOARD_PINETIME)
  nrf_gfx_init(p_lcd);
  nrf_gfx_screen_fill(p_lcd, 0xC618);
  nrf_gfx_point_t text_start = NRF_GFX_POINT(5,55);
  nrf_gfx_print_fast2(p_lcd, &text_start, 0x001F, failures? "FAIL": "SUCCESS", p_font, true);
#endif
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

