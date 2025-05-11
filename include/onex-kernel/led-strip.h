#ifndef LED_STRIP_H
#define LED_STRIP_H

#include <onex-kernel/colours.h>

void led_strip_init();
void led_strip_fill_hsv(colours_hsv hsv);
void led_strip_fill_rgb(colours_rgb rgb);
void led_strip_fill_col(char* colour);
void led_strip_show();

#endif
