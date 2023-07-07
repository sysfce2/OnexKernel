#ifndef TFT_H
#define TFT_H

#define ILI9163_TFTWIDTH  128
#define ILI9163_TFTHEIGHT 128

#define ILI9341_TFTWIDTH  240
#define ILI9341_TFTHEIGHT 320

#define TFT_BLACK   0x0000
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_BLUE    0x001F
#define TFT_YELLOW  0xFFE0
#define TFT_MAGENTA 0xF81F
#define TFT_CYAN    0x07FF
#define TFT_GREY    0x4208
#define TFT_GREYX   0x7777
#define TFT_ORANGE  0xDD80
#define TFT_WHITE   0xFFFF

extern int16_t tft_width;
extern int16_t tft_height;
extern uint8_t tft_rotation;

void   tft_init(int8_t cs, int8_t dc);
void   tft_render(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* fb);
void   tft_set_rotation(uint8_t m);
void   tft_set_text_colour(uint16_t c);
void   tft_set_text_size(uint8_t s);
void   tft_draw_pixel(int16_t x, int16_t y, uint16_t color);
void   tft_draw_horizontal_line(int16_t x, int16_t y, int16_t w, uint16_t color);
void   tft_draw_vertical_line(int16_t x, int16_t y, int16_t h, uint16_t color);
void   tft_fill_rectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void   tft_draw_rectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void   tft_set_cursor(int16_t x, int16_t y);
void   tft_fill_screen(uint16_t color);
size_t tft_draw_text(const uint8_t *buffer);

#endif
