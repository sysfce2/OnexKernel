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

typedef void (*gpio_pin_cb)(int);

void gpio_loop();
void gpio_mode(      uint32_t pin, uint32_t mode);
void gpio_mode_cb(   uint32_t pin, uint32_t mode, gpio_pin_cb cb);
int  gpio_get(       uint32_t pin);
void gpio_set(       uint32_t pin, uint32_t value);
void gpio_toggle(    uint32_t pin);
int  gpio_touch_read(uint32_t pin);

#endif

