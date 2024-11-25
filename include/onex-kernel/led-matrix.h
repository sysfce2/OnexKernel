#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#define LED_MATRIX_WIDTH  12
#define LED_MATRIX_HEIGHT  6

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} led_matrix_rgb;

uint8_t led_matrix_array[LED_MATRIX_WIDTH*LED_MATRIX_HEIGHT][3];

void led_matrix_init();
void led_matrix_fill_rgb(led_matrix_rgb rgb);
void led_matrix_fill_col(char* colour);
void led_matrix_show();

#endif
