
#include <boards.h>
#include <stdint.h>
#include <nrf.h>
#include <nrf_gpio.h>
#include <nrfx_gpiote.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>

/*
static void nrfx_gpiote_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  log_write("irq %d %d\n", pin, action);
}

static void set_up_buttons()
{
  nrf_gpio_cfg_output(BUTTON_ENABLE);
  nrf_gpio_pin_set(BUTTON_ENABLE);

  nrf_gpio_cfg_sense_input(BUTTON_1, (nrf_gpio_pin_pull_t)GPIO_PIN_CNF_PULL_Pulldown, (nrf_gpio_pin_sense_t)GPIO_PIN_CNF_SENSE_High);

  nrfx_gpiote_in_config_t pinConfig;
  pinConfig.skip_gpio_setup = true;
  pinConfig.hi_accuracy = false;
  pinConfig.is_watcher = false;
  pinConfig.sense = (nrf_gpiote_polarity_t)NRF_GPIOTE_POLARITY_HITOLO;
  pinConfig.pull = (nrf_gpio_pin_pull_t)GPIO_PIN_CNF_PULL_Pulldown;

  nrfx_gpiote_in_init(BUTTON_1, &pinConfig, nrfx_gpiote_evt_handler);
}
*/

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

#if defined(BUTTON_1)
static int button_1_pressed=0;
static gpio_pin_cb button_1_cb;
#endif

void gpio_loop()
{
#if defined(BUTTON_1)
  int b1p = BUTTONS_ACTIVE_STATE==gpio_get(BUTTON_1);
  if(button_1_cb && button_1_pressed != b1p){
     button_1_pressed = b1p;
     button_1_cb(button_1_pressed);
  }
#endif
}

void gpio_mode_cb(uint32_t pin, uint32_t mode, gpio_pin_cb cb)
{
  gpio_mode(pin, mode);

#if defined(BUTTON_1)
  if(pin==BUTTON_1){
    button_1_cb=cb;
  }
#endif
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

