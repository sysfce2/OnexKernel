#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#define LED_MATRIX_WIDTH  12
#define LED_MATRIX_HEIGHT  6

uint8_t led_matrix_array[LED_MATRIX_WIDTH*LED_MATRIX_HEIGHT][3];

void led_matrix_init();
void led_matrix_show(uint8_t style);

#endif
