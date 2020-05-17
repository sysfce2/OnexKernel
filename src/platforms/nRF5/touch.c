
#include <boards.h>

#include <nrfx_gpiote.h>
#include <app_timer.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/i2c.h>
#include <onex-kernel/touch.h>

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

static touch_touched_cb touch_cb = 0;

static void* twip;

static void nrfx_gpiote_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
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

  nrf_gpio_cfg_sense_input(TOUCH_IRQ_PIN, (nrf_gpio_pin_pull_t)GPIO_PIN_CNF_PULL_Pullup, (nrf_gpio_pin_sense_t)GPIO_PIN_CNF_SENSE_Low);

  nrfx_gpiote_in_config_t pinConfig;
  pinConfig.skip_gpio_setup = true;
  pinConfig.hi_accuracy = false;
  pinConfig.is_watcher = false;
  pinConfig.sense = (nrf_gpiote_polarity_t)NRF_GPIOTE_POLARITY_HITOLO;
  pinConfig.pull = (nrf_gpio_pin_pull_t)GPIO_PIN_CNF_PULL_Pullup;
  nrfx_gpiote_in_init(TOUCH_IRQ_PIN, &pinConfig, nrfx_gpiote_evt_handler);

  twip=i2c_init(400);

  ret_code_t e;

  e=app_timer_create(&touch_timer, APP_TIMER_MODE_REPEATED, every_50ms); APP_ERROR_CHECK(e);
  e=app_timer_start(touch_timer, APP_TIMER_TICKS(50), NULL); APP_ERROR_CHECK(e);
}

static bool pressed=false;

void every_50ms(void* xx)
{
  touch_info_t ti=touch_get_info();
  bool p=(ti.action==TOUCH_ACTION_CONTACT);
  if(p!=pressed){
    pressed=p;
    if(pressed && touch_cb) touch_cb(ti);
  }
}

uint8_t buf[63];

touch_info_t touch_get_info()
{
  touch_info_t info;

  i2c_read(twip, TOUCH_ADDRESS, buf, 63);

  int num_points = buf[TOUCH_NUM] & 0x0f;
  uint8_t point_id = buf[TOUCH_ID] >> 4;

  if(num_points == 0 && point_id == TOUCH_LAST) return info;

  info.gesture = buf[TOUCH_GESTURE];
  info.action = buf[TOUCH_ACTION] >> 6;
  info.x = (buf[TOUCH_X_HIGH] & 0x0f) << 8 | buf[TOUCH_X_LOW];
  info.y = (buf[TOUCH_Y_HIGH] & 0x0f) << 8 | buf[TOUCH_Y_LOW];

  return info;
}

void touch_reset()
{
  time_delay_ms(20);
  nrf_gpio_pin_clear(TOUCH_RESET_PIN);
  time_delay_ms(20);
  nrf_gpio_pin_set(TOUCH_RESET_PIN);
  time_delay_ms(200);
}

void touch_disable() {
  i2c_disable(twip);
  nrf_gpio_cfg_default(TOUCH_RESET_PIN);
}

