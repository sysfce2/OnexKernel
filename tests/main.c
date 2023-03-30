
// --------------------------------------------------------------------

#if defined(NRF5)

#include <boards.h>

#include <onex-kernel/gpio.h>

#if defined(BOARD_MAGIC3)
#include <onex-kernel/spi-flash.h>
#endif

#if defined(BOARD_PINETIME)
#include <onex-kernel/motion.h>
#endif

#if defined(HAS_SERIAL)
#include <onex-kernel/serial.h>
#endif

#if defined(BOARD_PCA10059) || defined(BOARD_PINETIME)
#include <onex-kernel/blenus.h>
#endif

#include <onex-kernel/gfx.h>
#include <onex-kernel/touch.h>

#endif // NRF5

#include <onex-kernel/random.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include <tests.h>

static volatile int16_t run_tests= -1;

extern void run_properties_tests();
extern void run_list_tests();
extern void run_value_tests();
extern void run_onn_tests(char* dbpath);

#if defined(NRF5)
static volatile bool display_state_prev=!LEDS_ACTIVE_STATE;
static volatile bool display_state=LEDS_ACTIVE_STATE;

void button_changed(uint8_t pin, uint8_t type)
{
  bool pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  if(pressed) display_state = !display_state;
}

#if defined(BOARD_PINETIME) || defined(BOARD_MAGIC3)
static volatile bool is_charging=false;

static void charging_changed(uint8_t pin, uint8_t type){
  is_charging=!gpio_get(CHARGE_SENSE);
}
#endif

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
  gpio_mode_cb(CHARGE_SENSE, INPUT, RISING_AND_FALLING, charging_changed);
#define ADC_CHANNEL 0
  gpio_adc_init(BATTERY_V, ADC_CHANNEL);
#elif defined(BOARD_MAGIC3)
  gpio_mode_cb(BUTTON_1, INPUT_PULLDOWN, RISING_AND_FALLING, button_changed);
  gpio_mode(I2C_ENABLE, OUTPUT);
  gpio_set( I2C_ENABLE, 1);
  gpio_mode(LCD_BACKLIGHT, OUTPUT);
  gpio_set(LCD_BACKLIGHT, LEDS_ACTIVE_STATE);
  gpio_mode_cb(CHARGE_SENSE, INPUT, RISING_AND_FALLING, charging_changed);
#define ADC_CHANNEL 0
  gpio_adc_init(BATTERY_V, ADC_CHANNEL);
#endif
}
#endif

#if defined(BOARD_PINETIME) || defined(BOARD_MAGIC3)
static void show_touch();
static bool new_touch_info=false;
static touch_info_t ti={ 120, 140 };

#if defined(BOARD_PINETIME)
static void show_motion();
static bool new_motion_info=false;
static motion_info_t mi;
#endif

static int irqs=0;

void touched(touch_info_t touchinfo)
{
  ti=touchinfo;
  new_touch_info=true;
  irqs++;
}

#if defined(BOARD_PINETIME)
void moved(motion_info_t motioninfo)
{
  mi=motioninfo;
  new_motion_info=true;
}
#endif

char buf[64];

void show_random()
{
  uint8_t r1=random_ish_byte();
  uint8_t r2=random_byte();
  snprintf(buf, 64, "#%02x#%02x#", r1, r2);
  gfx_pos(10, 135);
  gfx_text(buf);
}

void show_touch()
{
  snprintf(buf, 64, "-%03d-%03d-", ti.x, ti.y);
  gfx_pos(10, 90);
  gfx_text(buf);

  snprintf(buf, 64, "-%s-%s-%d-", touch_actions[ti.action], touch_gestures[ti.gesture], irqs);
  gfx_pos(10, 110);
  gfx_text(buf);

  run_tests++;
}

#if defined(BOARD_PINETIME)
void show_motion()
{
  snprintf(buf, 64, "(%05d)(%05d)(%05d)", mi.x, mi.y, mi.z);
  gfx_pos(10, 30);
  gfx_text(buf);
}
#endif

void show_battery()
{
  int16_t bv = gpio_read(ADC_CHANNEL);
  int16_t mv = bv*2000/(1024/(33/10));
  int8_t  pc = ((mv-3520)*100/5200)*10;

  snprintf(buf, 64, "%d/%dmv/%d%%", bv, mv, pc);
  gfx_pos(10, 135);
  gfx_text(buf);

  gfx_pos(10, 155);
  gfx_text(is_charging? "charging": "battery");
}

#endif // watches

#if !defined(BOARD_MAGIC3)
void on_recv(unsigned char* chars, size_t size)
{
  if(!size) return;
  log_write(">%c<----------\n", chars[0]);
  if(chars[0]=='t') run_tests++;
}
#endif

#if defined(BOARD_MAGIC3)

#define FLASH_TEST_DATA_START 0x30000
#define FLASH_TEST_DATA_SIZE  4096
#define FLASH_TEST_ERASE_LEN  SPI_FLASH_ERASE_LEN_4KB

static uint8_t wrbuf[FLASH_TEST_DATA_SIZE];
static uint8_t rdbuf[FLASH_TEST_DATA_SIZE];

char* run_flash_tests(char* allids) {

  char* err;

  err=spi_flash_init(allids);
  if(err) return err;

  err = spi_flash_erase(FLASH_TEST_DATA_START, FLASH_TEST_ERASE_LEN, 0);
  if(err) return err;

  for(uint32_t i=0; i < FLASH_TEST_DATA_SIZE; i++){
    wrbuf[i] = random_ish_byte();
  }
  uint32_t start_write_ts=(uint32_t)time_ms();
  static volatile uint32_t end_write_ts=0;
  void record_end_ts(){ end_write_ts=(uint32_t)time_ms(); }
  err = spi_flash_write(FLASH_TEST_DATA_START, wrbuf, FLASH_TEST_DATA_SIZE, record_end_ts);
  if(err) return err;
  while(!end_write_ts);

  err = spi_flash_read(FLASH_TEST_DATA_START, rdbuf, FLASH_TEST_DATA_SIZE, 0);
  if(err) return err;

  bool ok=!memcmp(wrbuf, rdbuf, FLASH_TEST_DATA_SIZE);
  static char res[64];
  snprintf(res, 64, ok? "%ldms %x:%x:%x:%x==%x:%x:%x:%x":
                        "%ldms %x:%x:%x:%x!=%x:%x:%x:%x",
                    end_write_ts-start_write_ts,
                    wrbuf[0], wrbuf[1], wrbuf[2], wrbuf[3],
                    rdbuf[0], rdbuf[1], rdbuf[2], rdbuf[3]);
  return res;
}
#endif

void run_tests_maybe() {

  if(run_tests) return;
  run_tests++;

  log_write("-----------------OnexKernel tests------------------------\n");

#if defined(BOARD_MAGIC3)
  char allids[64];
  char* flash_result=run_flash_tests(allids);

  gfx_pos(15, 235);
  gfx_text(flash_result);

  gfx_pos(15, 255);
  gfx_text(allids);
#endif

  run_value_tests();
  run_list_tests();
  run_properties_tests();
  run_onn_tests("Onex/onex.ondb");

#if defined(NRF5)
  int failures=onex_assert_summary();
#if defined(BOARD_PCA10059)
  if(failures) gpio_set(leds_list[1], 0);
  else         gpio_set(leds_list[2], 0);
#else
  gfx_pos(10, 55);
  gfx_text(failures? "FAIL": "SUCCESS");
#endif
#else
  onex_assert_summary();
#endif
}

#if defined(LOG_TO_GFX)
extern volatile char* event_log_buffer;
#endif

int main(void) {

  log_init();
  time_init();
  random_init();
#if defined(NRF5)
  gpio_init();
#if defined(HAS_SERIAL) // board dongle or dk?
  serial_init((serial_recv_cb)on_recv,0);
  blenus_init(0,0);
  set_up_gpio();
  time_ticker((void (*)())serial_loop, 1);
  while(1){
    run_tests_maybe();
    if (display_state_prev != display_state){
      display_state_prev = display_state;
      gpio_set(leds_list[3], display_state);
      log_write("#%d %d %d\n", display_state, random_ish_byte(), random_byte());
    }
  }
#else
#if defined(BOARD_PINETIME)
  blenus_init((blenus_recv_cb)on_recv, 0);
#endif
  set_up_gpio();
  gfx_init();
  gfx_screen_colour(GFX_YELLOW);
  gfx_screen_fill();
  gfx_rect_line(  0,  0, ST7789_WIDTH, ST7789_HEIGHT, GFX_RED, 3);
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
  gfx_text_colour(GFX_BLUE);

  touch_init(touched);
#if defined(BOARD_PINETIME)
  motion_init(moved);
#endif

  while(1){

    log_loop();

    run_tests_maybe();

    if(new_touch_info){
      new_touch_info=false;
      show_touch();
    //show_random();
      show_battery();
    }
#if defined(BOARD_PINETIME)
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
#endif
#if defined(BOARD_MAGIC3)
    if (display_state_prev != display_state){
      display_state_prev = display_state;
      gpio_set(LCD_BACKLIGHT, display_state);
    }
#endif
#if defined(LOG_TO_GFX)
    if(event_log_buffer){
      gfx_pos(10, 10);
      gfx_text_colour(GFX_RED);
      gfx_text((char*)event_log_buffer);
      gfx_text_colour(GFX_BLUE);
      event_log_buffer=0;
    }
#endif

    static uint8_t  loop_count = 0;
    static uint64_t tm_last = 0;
    static uint8_t  lps = 111;

    loop_count++;
    uint64_t tm=time_ms();
    if(tm > tm_last + 1000) {
      tm_last = tm;
      lps = loop_count;
      loop_count = 0;
    }
    snprintf(buf, 64, "lps: ===%d===", lps);
    gfx_pos(10, 70);
    gfx_text(buf);
  }
#endif // HAS_SERIAL
#else // NRF5
  on_recv((unsigned char*)"t", 1);
  run_tests_maybe();
  time_end();
#endif
}

// --------------------------------------------------------------------
