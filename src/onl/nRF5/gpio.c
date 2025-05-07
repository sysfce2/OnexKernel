
#include <boards.h>
#include <stdint.h>
#include <nrf.h>
#include <nrf_gpio.h>
#include <nrfx_saadc.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>

static volatile bool initialised=false;

void gpio_init() {
  if(initialised) return;
  NRF_GPIOTE->EVENTS_PORT = 0; volatile uint32_t readit=NRF_GPIOTE->EVENTS_PORT; (void)readit;
  NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
  NVIC_SetPriority(GPIOTE_IRQn, 2);
  NVIC_EnableIRQ(GPIOTE_IRQn);
  initialised=true;
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

typedef struct gpio_interrupt {
  uint8_t     pin;
  uint8_t     mode;
  uint8_t     edge;
  gpio_pin_cb cb;
  uint8_t     last_state;
} gpio_interrupt;

#define MAX_GPIO_INTERRUPTS 8
static uint8_t                 top_gpio_interrupt=0;
static volatile gpio_interrupt gpio_interrupts[MAX_GPIO_INTERRUPTS];

void set_sense(int pin, int hi_lo_dis)
{
  if(pin<32){
    NRF_P0->PIN_CNF[pin] &= ~GPIO_PIN_CNF_SENSE_Msk;
    NRF_P0->PIN_CNF[pin] |= (hi_lo_dis << GPIO_PIN_CNF_SENSE_Pos);
  }
#if (GPIO_COUNT == 2)
  else{
    NRF_P1->PIN_CNF[pin-32] &= ~GPIO_PIN_CNF_SENSE_Msk;
    NRF_P1->PIN_CNF[pin-32] |= (hi_lo_dis << GPIO_PIN_CNF_SENSE_Pos);
  }
#endif
}

bool get_latch_and_clear(uint8_t pin)
{
  bool r=false;
  if(pin<32){
    uint32_t b=1<<pin;
    r=!!(NRF_P0->LATCH & b);
    if(r) NRF_P0->LATCH=b;
  }
#if (GPIO_COUNT == 2)
  else{
    uint32_t b=1<<(pin-32);
    r=!!(NRF_P1->LATCH & b);
    if(r) NRF_P0->LATCH=b;
  }
#endif
  return r;
}

void GPIOTE_IRQHandler()
{
  if(!(NRF_GPIOTE->EVENTS_PORT)) return;

  gpio_disable_interrupts();

  NRF_GPIOTE->EVENTS_PORT = 0; volatile uint32_t readit=NRF_GPIOTE->EVENTS_PORT; (void)readit;

  for(uint8_t i=0; i<top_gpio_interrupt; i++){

    uint8_t pin=gpio_interrupts[i].pin;

    bool latched=get_latch_and_clear(pin);

    uint8_t state=gpio_get(pin);

    set_sense(pin, state? GPIO_PIN_CNF_SENSE_Low: GPIO_PIN_CNF_SENSE_High);

    bool changed=(state!=gpio_interrupts[i].last_state);

    if(!(changed || latched)) continue;

    gpio_interrupts[i].last_state=state;

    bool quick_change=(!changed && latched);
#if defined(LOG_GPIO_SUCCESSSSS)
    if(changed && !latched) log_write("pin %d not DETECTed but change read\n", pin);
    if(quick_change)        log_write("pin %d quick change missed but DETECTed by LATCH\n", pin);
#endif

    switch(gpio_interrupts[i].edge){
      case(RISING):             { if(quick_change ||  state) gpio_interrupts[i].cb(pin, RISING);                 break; }
      case(FALLING):            { if(quick_change || !state) gpio_interrupts[i].cb(pin, FALLING);                break; }
      case(RISING_AND_FALLING): {                            gpio_interrupts[i].cb(pin, state? RISING: FALLING); break; }
    }
  }
}

void gpio_mode_cb(uint8_t pin, uint8_t mode, uint8_t edge, gpio_pin_cb cb)
{
  gpio_mode(pin, mode);
  if(top_gpio_interrupt==MAX_GPIO_INTERRUPTS) return;
  uint8_t i=top_gpio_interrupt++;
  gpio_interrupts[i].pin=pin;
  gpio_interrupts[i].mode=mode;
  gpio_interrupts[i].edge=edge;
  gpio_interrupts[i].cb=cb;
  uint8_t state=gpio_get(pin);
  gpio_interrupts[i].last_state=state;
  set_sense(pin, state? GPIO_PIN_CNF_SENSE_Low: GPIO_PIN_CNF_SENSE_High);
}

void gpio_enable_interrupts()
{
  for(uint8_t i=0; i<top_gpio_interrupt; i++){
    uint8_t pin=gpio_interrupts[i].pin;
    uint8_t state=gpio_get(pin);
    gpio_interrupts[i].last_state=state;
    set_sense(pin, state? GPIO_PIN_CNF_SENSE_Low: GPIO_PIN_CNF_SENSE_High);
  }
}

void gpio_disable_interrupts()
{
  for(uint8_t i=0; i<top_gpio_interrupt; i++){
    set_sense(gpio_interrupts[i].pin, GPIO_PIN_CNF_SENSE_Disabled);
  }
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
  gpio_wake();
  nrf_saadc_value_t value = 0;
  nrfx_saadc_sample_convert(channel, &value);
  return value;
}

static bool sleeping=false;
void gpio_sleep()
{
  if(sleeping) return;
  sleeping=true;
  NRF_SAADC->ENABLE=(SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos);
}

void gpio_wake()
{
  if(!sleeping) return;
  sleeping=false;
  NRF_SAADC->ENABLE=(SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos);
}

