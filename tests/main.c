
// --------------------------------------------------------------------

#if defined(NRF5)
#include <boards.h>
#if defined(BOARD_PINETIME)
#include <onex-kernel/gfx.h>
#include <onex-kernel/touch.h>
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

#if defined(NRF5)

static void set_up_gpio(void)
{
  gpio_mode(   BUTTON_ENABLE, OUTPUT);
  gpio_set(    BUTTON_ENABLE, 1);
  gpio_mode(LCD_BACKLIGHT_HIGH, OUTPUT);
}
#endif

#if defined(BOARD_PCA10059)
const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;

void flash_led(int t)
{
    for(uint8_t l=0; l< LEDS_NUMBER; l++) gpio_mode(leds_list[l], OUTPUT);
    for(;;) for(int8_t l=LEDS_NUMBER-1; l>=0; l--){ gpio_toggle(leds_list[l]); time_delay_ms(t); }
}
#endif

#if defined(BOARD_PINETIME)
static volatile bool display_state_prev=!LEDS_ACTIVE_STATE;
static volatile bool display_state=LEDS_ACTIVE_STATE;
#endif

#if defined(BOARD_PINETIME)
static void show_touch();
static bool was_touched=false;
static int irqs=0;

void touched()
{
  was_touched=true;
  irqs++;
}

void show_touch()
{
  touch_info ti=touch_get_info();
  char buf[64];

  snprintf(buf, 64, "-%03d-%03d-", ti.x, ti.y);
  gfx_pos(10, 60);
  gfx_text(buf);

  snprintf(buf, 64, "-%02d-%02d-%02d-", ti.action, ti.gesture, irqs);
  gfx_pos(10, 110);
  gfx_text(buf);

  if(ti.gesture==TOUCH_GESTURE_TAP_LONG){
    display_state = !LEDS_ACTIVE_STATE;
  }
}
#endif

void on_recv(unsigned char* buf, size_t size)
{
  if(!size) return;

  log_write(">%c\n", buf[0]);

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
  gfx_pos(10, 10);
  gfx_text(failures? "FAIL": "SUCCESS");
#endif
#else
  onex_assert_summary();
#endif
}

int main(void)
{
  log_init();
  time_init();
  gpio_init();
#if defined(NRF5)
#if defined(HAS_SERIAL)
  serial_init((serial_recv_cb)on_recv,0);
  blenus_init(0);
  while(1) serial_loop();
#else
  blenus_init((blenus_recv_cb)on_recv);
#if defined(BOARD_PINETIME)
  gfx_reset();
  gfx_init();
  gfx_screen_colour(0xC618);
  gfx_text_colour(0x001F);
  gfx_screen_fill();
  gfx_pos(10, 10);
  gfx_text("Onex");
  set_up_gpio();
  touch_init(touched);
#endif
  while(1){
#if defined(BOARD_PINETIME)
    if(was_touched){
      was_touched=false;
      display_state = LEDS_ACTIVE_STATE;
      show_touch();
    }
    if (display_state_prev != display_state){
      display_state_prev = display_state;
      gpio_set(LCD_BACKLIGHT_HIGH, display_state);
    }
#endif
  }
#endif
#else
  on_recv((unsigned char*)"t", 1);
#endif
}

// --------------------------------------------------------------------

