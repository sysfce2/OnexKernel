
#include <boards.h>

#include <nrfx_twi.h>
#include <nrfx_gpiote.h>
#include <app_timer.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/touch.h>

#define TOUCH_SDA_PIN 6
#define TOUCH_SCL_PIN 7
#define TOUCH_IRQ_PIN 28
#define TOUCH_RESET_PIN 10
#define TOUCH_ADDRESS 0x15
// 0x15=touch 0x18=accel 0x44=HR

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

nrfx_twi_t twi = NRFX_TWI_INSTANCE(1);

static touch_touched_cb touch_cb = 0;

static void nrfx_gpiote_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  touch_info_t ti=touch_get_info();
  if(touch_cb) touch_cb(ti);
}

void touch_reset()
{
  time_delay_ms(20);
  nrf_gpio_pin_clear(TOUCH_RESET_PIN);
  time_delay_ms(20);
  nrf_gpio_pin_set(TOUCH_RESET_PIN);
  time_delay_ms(200);
}

APP_TIMER_DEF(touch_timer);
static void every_50ms(void* p);

void touch_init(touch_touched_cb cb)
{
  ret_code_t e;

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

  nrfx_twi_config_t twiConfig;
  twiConfig.frequency = NRF_TWI_FREQ_400K;
  twiConfig.sda = TOUCH_SDA_PIN;
  twiConfig.scl = TOUCH_SCL_PIN;
  twiConfig.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY;
  twiConfig.hold_bus_uninit = NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT;
  nrfx_twi_init(&twi, &twiConfig, 0, 0);
  nrfx_twi_enable(&twi);

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

  nrfx_twi_rx(&twi, TOUCH_ADDRESS, buf, 63);

  int num_points = buf[TOUCH_NUM] & 0x0f;
  uint8_t point_id = buf[TOUCH_ID] >> 4;

  if(num_points == 0 && point_id == TOUCH_LAST) return info;

  info.gesture = buf[TOUCH_GESTURE];
  info.action = buf[TOUCH_ACTION] >> 6;
  info.x = (buf[TOUCH_X_HIGH] & 0x0f) << 8 | buf[TOUCH_X_LOW];
  info.y = (buf[TOUCH_Y_HIGH] & 0x0f) << 8 | buf[TOUCH_Y_LOW];

  return info;
}

void touch_disable() {
  nrfx_twi_disable(&twi);
  nrf_gpio_cfg_default(TOUCH_SDA_PIN);
  nrf_gpio_cfg_default(TOUCH_SCL_PIN);
  nrf_gpio_cfg_default(TOUCH_RESET_PIN);
}

