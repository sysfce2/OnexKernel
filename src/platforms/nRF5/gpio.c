
#include <boards.h>
#include <stdint.h>
#include <nrf.h>
#include <nrf_gpio.h>
#include <nrfx_gpiote.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>

void gpio_init()
{
  if(!nrfx_gpiote_is_init()) APP_ERROR_CHECK(nrfx_gpiote_init());
}

void gpio_mode(uint32_t pin, uint32_t mode)
{
    switch (mode){
    case INPUT:
      nrf_gpio_cfg(pin,
                   GPIO_PIN_CNF_DIR_Input,
                   GPIO_PIN_CNF_INPUT_Connect,
                   GPIO_PIN_CNF_PULL_Disabled,
                   GPIO_PIN_CNF_DRIVE_S0S1,
                   GPIO_PIN_CNF_SENSE_Disabled
      );
    break;
    case INPUT_PULLUP:
      nrf_gpio_cfg(pin,
                   GPIO_PIN_CNF_DIR_Input,
                   GPIO_PIN_CNF_INPUT_Connect,
                   GPIO_PIN_CNF_PULL_Pullup,
                   GPIO_PIN_CNF_DRIVE_S0S1,
                   GPIO_PIN_CNF_SENSE_Disabled
      );
    break;
    case INPUT_PULLDOWN:
      nrf_gpio_cfg(pin,
                   GPIO_PIN_CNF_DIR_Input,
                   GPIO_PIN_CNF_INPUT_Connect,
                   GPIO_PIN_CNF_PULL_Pulldown,
                   GPIO_PIN_CNF_DRIVE_S0S1,
                   GPIO_PIN_CNF_SENSE_Disabled
      );
    break;
    case OUTPUT:
      nrf_gpio_cfg(pin,
                   GPIO_PIN_CNF_DIR_Output,
                   GPIO_PIN_CNF_INPUT_Connect,
                   GPIO_PIN_CNF_PULL_Disabled,
                   GPIO_PIN_CNF_DRIVE_H0H1,
                   GPIO_PIN_CNF_SENSE_Disabled
      );
    break;
    }
}

static gpio_pin_cb pin_cb=0;

void in_pin_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  if(pin_cb) pin_cb(BUTTONS_ACTIVE_STATE==gpio_get(pin));
}

void gpio_mode_cb(uint32_t pin, uint32_t mode, gpio_pin_cb cb)
{
  pin_cb = cb;
  nrfx_gpiote_in_config_t config;
  config.skip_gpio_setup = true;
  config.hi_accuracy = false;
  config.is_watcher = false;
  config.sense = (nrf_gpiote_polarity_t)NRF_GPIOTE_POLARITY_TOGGLE;

  switch (mode){
    case INPUT:
      nrf_gpio_cfg_sense_input(pin, GPIO_PIN_CNF_PULL_Disabled, GPIO_PIN_CNF_SENSE_High);
      config.pull = GPIO_PIN_CNF_PULL_Disabled;
      break;
    case INPUT_PULLUP:
      nrf_gpio_cfg_sense_input(pin, GPIO_PIN_CNF_PULL_Pullup, GPIO_PIN_CNF_SENSE_High);
      config.pull = GPIO_PIN_CNF_PULL_Pullup;
      break;
    case INPUT_PULLDOWN:
      nrf_gpio_cfg_sense_input(pin, GPIO_PIN_CNF_PULL_Pulldown, GPIO_PIN_CNF_SENSE_High);
      config.pull = GPIO_PIN_CNF_PULL_Pulldown;
      break;
  }
  APP_ERROR_CHECK(nrfx_gpiote_in_init(pin, &config, in_pin_handler));
  nrfx_gpiote_in_event_enable(pin, true);
}

int gpio_get(uint32_t pin)
{
  return nrf_gpio_pin_read(pin);
}

void gpio_set(uint32_t pin, uint32_t value)
{
  if (value) nrf_gpio_pin_set(pin);
  else       nrf_gpio_pin_clear(pin);
}

void gpio_toggle(uint32_t pin)
{
  nrf_gpio_pin_toggle(pin);
}

