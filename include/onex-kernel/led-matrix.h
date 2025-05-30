#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <onex-kernel/colours.h>

#define LED_MATRIX_WIDTH  12
#define LED_MATRIX_HEIGHT  6

void led_matrix_init();
void led_matrix_set_scale(uint8_t scale);
void led_matrix_fill_hsv(colours_hsv hsv);
void led_matrix_fill_rgb(colours_rgb rgb);
void led_matrix_fill_col(char* colour);
void led_matrix_show();

#endif
