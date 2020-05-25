
#include <boards.h>
#include <stdint.h>
#include <nrf.h>
#include <nrf_gpio.h>
#include <nrfx_gpiote.h>
#include <nrfx_saadc.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>

void gpio_init()
{
  if(!nrfx_gpiote_is_init()) APP_ERROR_CHECK(nrfx_gpiote_init());
}

void gpio_mode(uint8_t pin, uint8_t mode)
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

void gpio_mode_cb(uint8_t pin, uint8_t mode, uint8_t edge, gpio_pin_cb cb)
{
  nrfx_gpiote_in_config_t config;
  config.skip_gpio_setup = true;
  config.hi_accuracy = true; // HIGHER POWER CONSUMPTION! FIXME
  config.is_watcher = false;
  config.sense = (nrf_gpiote_polarity_t)edge; // sdk/modules/nrfx/mdk/nrf52_bitfields.h

  switch (mode){
    case INPUT:
      nrf_gpio_cfg_sense_input(pin, GPIO_PIN_CNF_PULL_Disabled, GPIO_PIN_CNF_SENSE_High);
      config.pull = GPIO_PIN_CNF_PULL_Disabled;
      break;
    case INPUT_PULLUP:
      nrf_gpio_cfg_sense_input(pin, GPIO_PIN_CNF_PULL_Pullup, GPIO_PIN_CNF_SENSE_Low);
      config.pull = GPIO_PIN_CNF_PULL_Pullup;
      break;
    case INPUT_PULLDOWN:
      nrf_gpio_cfg_sense_input(pin, GPIO_PIN_CNF_PULL_Pulldown, GPIO_PIN_CNF_SENSE_High);
      config.pull = GPIO_PIN_CNF_PULL_Pulldown;
      break;
  }
  nrfx_gpiote_in_init(pin, &config, (void (*)(nrfx_gpiote_pin_t, nrf_gpiote_polarity_t))cb);
  nrfx_gpiote_in_event_enable(pin, true);
}

uint8_t gpio_get(uint8_t pin)
{
  return nrf_gpio_pin_read(pin);
}

void gpio_set(uint8_t pin, uint8_t value)
{
  if (value) nrf_gpio_pin_set(pin);
  else       nrf_gpio_pin_clear(pin);
}

void gpio_toggle(uint8_t pin)
{
  nrf_gpio_pin_toggle(pin);
}

void saadc_event(nrfx_saadc_evt_t const * event) { }

void gpio_adc_init(uint8_t pin, uint8_t channel) {

  nrfx_saadc_config_t adc_config = NRFX_SAADC_DEFAULT_CONFIG;
  nrfx_saadc_init(&adc_config, saadc_event);

  nrf_saadc_channel_config_t adc_channel_config = {
          .resistor_p = NRF_SAADC_RESISTOR_DISABLED,
          .resistor_n = NRF_SAADC_RESISTOR_DISABLED,
          .gain       = NRF_SAADC_GAIN1_5,
          .reference  = NRF_SAADC_REFERENCE_INTERNAL,
          .acq_time   = NRF_SAADC_ACQTIME_3US,
          .mode       = NRF_SAADC_MODE_SINGLE_ENDED,
          .burst      = NRF_SAADC_BURST_DISABLED,
          .pin_p      = pin,
          .pin_n      = NRF_SAADC_INPUT_DISABLED
  };
  nrfx_saadc_channel_init(channel, &adc_channel_config);
}

int16_t gpio_read(uint8_t channel) {
  nrf_saadc_value_t value = 0;
  nrfx_saadc_sample_convert(channel, &value);
  return value;
}

