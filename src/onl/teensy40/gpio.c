
#include <core_pins.h>
#include <gpio.h>

void gpio_mode(uint32_t pin, uint32_t mode)
{
  pinMode(pin,mode);
}

int  gpio_get(uint32_t pin)
{
  return digitalRead(pin);
}

void gpio_set(uint32_t pin, uint32_t value)
{
  digitalWriteFast(pin,value);
}

void gpio_toggle(uint32_t pin)
{
}

int gpio_touch_read(uint32_t pin)
{
  return touchRead(pin);
}

