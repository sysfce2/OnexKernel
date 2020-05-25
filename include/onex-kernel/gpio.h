#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define HIGH		1
#define LOW		0
#define INPUT		0
#define OUTPUT		1
#define INPUT_PULLUP	2
#define INPUT_PULLDOWN  3
#define RISING             1
#define FALLING            2
#define RISING_AND_FALLING 3

typedef void (*gpio_pin_cb)(uint8_t, uint8_t); // pin, type (RISING, FALLING, RISING_AND_FALLING)

void    gpio_init();
void    gpio_mode(      uint8_t pin, uint8_t mode);
void    gpio_mode_cb(   uint8_t pin, uint8_t mode, uint8_t edge, gpio_pin_cb cb);
uint8_t gpio_get(       uint8_t pin);
void    gpio_adc_init(  uint8_t pin, uint8_t channel);
int16_t gpio_read(      uint8_t  channel);
void    gpio_set(       uint8_t pin, uint8_t value);
void    gpio_toggle(    uint8_t pin);
int     gpio_touch_read(uint8_t pin);

#endif

