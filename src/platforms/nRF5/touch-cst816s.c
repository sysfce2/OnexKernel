
#include <boards.h>

#include <app_timer.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/i2c.h>
#include <onex-kernel/touch.h>

#define HYN_REG_POWER_MODE       0xA5
#define HYN_REG_POWER_MODE_SLEEP 0x03

#define TOUCH_GESTURE 1
#define TOUCH_NUM 2
#define TOUCH_ACTION 3
#define TOUCH_X_HIGH 3
#define TOUCH_X_LOW 4
#define TOUCH_Y_HIGH 5
#define TOUCH_Y_LOW 6
#define TOUCH_ID 5

#define TOUCH_LAST 0x0f

char* touch_gestures[]={ "none", "down", "up", "left", "right", "tap", "", "", "", "", "", "double", "long", };
char* touch_actions[]={ "none", "up", "up", "down" };

static touch_touched_cb touch_cb = 0;

static void* twip;

static void touched(uint8_t pin, uint8_t type) {
  touch_info_t ti=touch_get_info();
  if(touch_cb) touch_cb(ti);
}

APP_TIMER_DEF(touch_timer);
static void every_50ms(void* p);

void touch_init(touch_touched_cb cb)
{
  touch_cb = cb;

  nrf_gpio_cfg_output(TOUCH_RESET_PIN);
  nrf_gpio_pin_set(TOUCH_RESET_PIN);

  twip=i2c_init(400);

  ret_code_t e;

  e=app_timer_create(&touch_timer, APP_TIMER_MODE_REPEATED, every_50ms); APP_ERROR_CHECK(e);
  e=app_timer_start(touch_timer, APP_TIMER_TICKS(50), NULL); APP_ERROR_CHECK(e);

  gpio_mode_cb(TOUCH_IRQ_PIN, INPUT_PULLUP, FALLING, touched);
}

static bool pressed=false;

void every_50ms(void* xx)
{
  touch_info_t ti=touch_get_info();
  bool p=(ti.action==TOUCH_ACTION_CONTACT);
  if(p!=pressed){
    pressed=p;
    if(touch_cb) touch_cb(ti);
  }
}

static uint8_t buf[63];

touch_info_t touch_get_info()
{
  touch_info_t ti={0};

  uint8_t r=i2c_read(twip, TOUCH_ADDRESS, buf, 63);
  if(r) return ti;

  int num_points = buf[TOUCH_NUM] & 0x0f;
  uint8_t point_id = buf[TOUCH_ID] >> 4;

  if(num_points == 0 && point_id == TOUCH_LAST) return ti;

  ti.gesture = buf[TOUCH_GESTURE];
  ti.action = (buf[TOUCH_ACTION] >> 6)+1;
  ti.x = (buf[TOUCH_X_HIGH] & 0x0f) << 8 | buf[TOUCH_X_LOW];
  ti.y = (buf[TOUCH_Y_HIGH] & 0x0f) << 8 | buf[TOUCH_Y_LOW];

  return ti;
}

void touch_reset(uint8_t delay)
{
  nrf_gpio_pin_clear(TOUCH_RESET_PIN);
  time_delay_ms(delay);
  nrf_gpio_pin_set(TOUCH_RESET_PIN);
}

void touch_sleep() {
  touch_reset(5);
  i2c_write_register_byte(twip, TOUCH_ADDRESS, HYN_REG_POWER_MODE, HYN_REG_POWER_MODE_SLEEP);
}

void touch_wake() {
  // touch_reset(10);
  // ??
}

