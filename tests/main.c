
// --------------------------------------------------------------------

#if defined(NRF5)
#include <boards.h>
#if defined(BOARD_PINETIME)
#include <onex-kernel/gfx.h>
#include <onex-kernel/touch.h>
#include <onex-kernel/motion.h>
#endif
#include <onex-kernel/gpio.h>
#if defined(HAS_SERIAL)
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/blenus.h>
#endif

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <tests.h>

extern void run_properties_tests();
extern void run_list_tests();
extern void run_value_tests();
extern void run_onn_tests(char* dbpath);

#if defined(BOARD_PINETIME)
static bool display_state_prev=!LEDS_ACTIVE_STATE;
static bool display_state=LEDS_ACTIVE_STATE;
#endif

#if defined(NRF5)
void button_changed(uint8_t pin, uint8_t type)
{
  bool pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  log_write("#%d\n", pressed);
#if defined(BOARD_PINETIME)
  if(pressed) display_state = !display_state;
#endif
}

#if defined(BOARD_PCA10059)
const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;
#endif

static void set_up_gpio(void)
{
#if defined(BOARD_PCA10059)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  for(uint8_t l=0; l< LEDS_NUMBER; l++){ gpio_mode(leds_list[l], OUTPUT); gpio_set(leds_list[l], 1); }
  gpio_set(leds_list[0], 0);
#elif defined(BOARD_PINETIME)
  gpio_mode_cb(BUTTON_1, INPUT_PULLDOWN, RISING_AND_FALLING, button_changed);
  gpio_mode(BUTTON_ENABLE, OUTPUT);
  gpio_set( BUTTON_ENABLE, 1);
  gpio_mode(LCD_BACKLIGHT_HIGH, OUTPUT);
  gpio_mode(VIBRATION, OUTPUT);
  gpio_set(VIBRATION, 1);
#define ADC_CHANNEL 0
  gpio_adc_init(BATTERY_V, ADC_CHANNEL);
#endif
}
#endif

#if defined(BOARD_PINETIME)
static void show_touch();
static bool new_touch_info=false;
static touch_info_t ti;

static void show_motion();
static bool new_motion_info=false;
static motion_info_t mi;

static int irqs=0;

void touched(touch_info_t touchinfo)
{
  ti=touchinfo;
  new_touch_info=true;
  irqs++;
}

void moved(motion_info_t motioninfo)
{
  mi=motioninfo;
  new_motion_info=true;
}

char buf[64];

void show_touch()
{
  snprintf(buf, 64, "-%03d-%03d-", ti.x, ti.y);
  gfx_pos(10, 85);
  gfx_text(buf);

  snprintf(buf, 64, "-%s-%s-%d-", touch_actions[ti.action], touch_gestures[ti.gesture], irqs);
  gfx_pos(10, 110);
  gfx_text(buf);
}

void show_motion()
{
  snprintf(buf, 64, "(%05d)(%05d)(%05d)", mi.x, mi.y, mi.z);
  gfx_pos(10, 60);
  gfx_text(buf);
}

void show_battery()
{
  int16_t bv = gpio_read(ADC_CHANNEL);
  int16_t mv = bv*2000/(1024/(33/10));
  int8_t  pc = ((mv-3520)*100/5200)*10;
  snprintf(buf, 64, "%d%%(%d)", pc, mv);
  gfx_pos(10, 160);
  gfx_text(buf);
}
#endif

static volatile bool run_tests=false;

void on_recv(unsigned char* chars, size_t size)
{
  if(!size) return;
  log_write(">%c<----------\n", chars[0]);
  if(chars[0]=='t') run_tests=true;
}

void run_tests_maybe()
{
  if(!run_tests) return;
  run_tests=false;

  log_write("-----------------OnexKernel tests------------------------\n");
  run_value_tests();
  run_list_tests();
  run_properties_tests();
  run_onn_tests("Onex/onex.ondb");

#if defined(NRF5)
  int failures=onex_assert_summary();
#if defined(BOARD_PCA10059)
  if(failures) gpio_set(leds_list[1], 0);
  else         gpio_set(leds_list[2], 0);
#elif defined(BOARD_PINETIME)
  gfx_pos(10, 10);
  gfx_text(failures? "FAIL": "SUCCESS");
#endif
#else
  onex_assert_summary();
#endif
}

#if defined(LOG_TO_GFX)
extern volatile char* event_log_buffer;
#endif

int main(void)
{
  log_init();
  time_init();
#if defined(NRF5)
  gpio_init();
#if defined(HAS_SERIAL)
  serial_init((serial_recv_cb)on_recv,0);
  blenus_init(0,0);
  set_up_gpio();
  time_ticker((void (*)())serial_loop, 1);
  while(1) run_tests_maybe();
#else
  blenus_init((blenus_recv_cb)on_recv, 0);
#if defined(BOARD_PINETIME)
  gfx_reset();
  gfx_init();
  gfx_screen_colour(0x0);
  gfx_screen_fill();
  gfx_rect_line(  0,  0, 240,240, GFX_GREY_F, 3);
  gfx_rect_fill( 15,180,  20, 20, GFX_RGB256(255,255,255));
  gfx_rect_fill( 15,210,  20, 20, GFX_WHITE);
  gfx_rect_fill( 35,180,  20, 20, GFX_RGB256(63,63,63));
  gfx_rect_fill( 35,210,  20, 20, GFX_GREY_7);
  gfx_rect_fill( 55,180,  20, 20, GFX_RGB256(31,31,31));
  gfx_rect_fill( 55,210,  20, 20, GFX_GREY_3);
  gfx_rect_fill( 75,180,  20, 20, GFX_RGB256(15,15,15));
  gfx_rect_fill( 75,210,  20, 20, GFX_GREY_1);
  gfx_rect_fill( 95,180,  20, 20, GFX_RGB256(255,0,0));
  gfx_rect_fill( 95,210,  20, 20, GFX_RED);
  gfx_rect_fill(115,180,  20, 20, GFX_RGB256(0,255,0));
  gfx_rect_fill(115,210,  20, 20, GFX_GREEN);
  gfx_rect_fill(135,180,  20, 20, GFX_RGB256(0,0,255));
  gfx_rect_fill(135,210,  20, 20, GFX_BLUE);
  gfx_rect_fill(155,180,  20, 20, GFX_RGB256(255,255,0));
  gfx_rect_fill(155,210,  20, 20, GFX_YELLOW);
  gfx_rect_fill(175,180,  20, 20, GFX_RGB256(255,0,255));
  gfx_rect_fill(175,210,  20, 20, GFX_MAGENTA);
  gfx_rect_fill(195,180,  20, 20, GFX_RGB256(0,255,255));
  gfx_rect_fill(195,210,  20, 20, GFX_CYAN);
  gfx_pos(10, 10);
  gfx_text_colour(GFX_BLUE);
  gfx_text("Onex");
  touch_init(touched);
  motion_init(moved);
#endif
  set_up_gpio();
  while(1){
    log_loop();
    run_tests_maybe();
#if defined(BOARD_PINETIME)
    if(new_touch_info){
      new_touch_info=false;
      show_touch();
      show_battery();
    }
    if(new_motion_info){
      new_motion_info=false;
      static int ticks=0; // every 20ms
      ticks++;
      if(!(ticks%20)) show_motion();
    }
    if (display_state_prev != display_state){
      display_state_prev = display_state;
      gpio_set(LCD_BACKLIGHT_HIGH, display_state);
    }
#if defined(LOG_TO_GFX)
    if(event_log_buffer){
      gfx_pos(10, 10);
      gfx_text_colour(GFX_WHITE);
      gfx_text((char*)event_log_buffer);
      event_log_buffer=0;
    }
#endif
#endif
  }
#endif
#else
  on_recv((unsigned char*)"t", 1);
  run_tests_maybe();
  time_end();
#endif
}

// --------------------------------------------------------------------

