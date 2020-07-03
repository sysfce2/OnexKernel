#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

// mode values in gpio_mode/gpio_mode_cb
#define INPUT		1
#define OUTPUT		2
#define INPUT_PULLUP	3
#define INPUT_PULLDOWN  4

// edge values in gpio_mode_cb
#define RISING             1
#define FALLING            2
#define RISING_AND_FALLING 3

typedef void (*gpio_pin_cb)(uint8_t, uint8_t); // (pin, edge) (edge: RISING, FALLING)

void    gpio_init();
void    gpio_mode(      uint8_t pin, uint8_t mode);
void    gpio_mode_cb(   uint8_t pin, uint8_t mode, uint8_t edge, gpio_pin_cb cb);
uint8_t gpio_get(       uint8_t pin);
void    gpio_adc_init(  uint8_t pin, uint8_t channel);
int16_t gpio_read(      uint8_t  channel);
void    gpio_set(       uint8_t pin, uint8_t value);
void    gpio_toggle(    uint8_t pin);
int     gpio_touch_read(uint8_t pin);
void    gpio_enable_interrupts();
void    gpio_disable_interrupts();
void    gpio_sleep();
void    gpio_wake();

#endif

