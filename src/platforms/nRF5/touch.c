
#include <nrfx_twi.h>
#include <nrfx_log.h>
#include <legacy/nrf_drv_gpiote.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
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

nrfx_twi_t twi = NRFX_TWI_INSTANCE(1);

void touch_reset()
{
  time_delay_ms(20);
  nrf_gpio_pin_clear(TOUCH_RESET_PIN);
  time_delay_ms(20);
  nrf_gpio_pin_set(TOUCH_RESET_PIN);
  time_delay_ms(200);
}

void touch_init()
{
  nrf_gpio_cfg_output(TOUCH_RESET_PIN);
  nrf_gpio_pin_set(TOUCH_RESET_PIN);

  nrfx_twi_config_t config;
  config.frequency = NRF_TWI_FREQ_400K;
  config.sda = TOUCH_SDA_PIN;
  config.scl = TOUCH_SCL_PIN;
  config.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY;
  config.hold_bus_uninit = NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT;

  nrfx_twi_init(&twi, &config, 0, 0);
  nrfx_twi_enable(&twi);
}

uint8_t buf[63];

touch_info touch_get_info()
{
  touch_info info;

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

