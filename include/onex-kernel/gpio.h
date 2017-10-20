#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define HIGH		1
#define LOW		0
#define INPUT		0
#define OUTPUT		1
#define INPUT_PULLUP	2
//#define INPUT_PULLDOWN  3
#define INPUT_PULLDOWN  4

void gpio_mode(      uint32_t pin, uint32_t mode);
int  gpio_get(       uint32_t pin);
void gpio_set(       uint32_t pin, uint32_t value);
void gpio_toggle(    uint32_t pin);
int  gpio_touch_read(uint32_t pin);

#endif

