
#include <string.h>
#include <stdint.h>
#include <nrf_gpio.h>
#include <nrf_gfx.h>
#include <nrf_delay.h>
#include <onex-kernel/display.h>
#include <onex-kernel/gfx.h>
#include "font57.h"

extern const nrf_lcd_t           nrf_lcd_st7789;
extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;

static const nrf_lcd_t*           lcd  = &nrf_lcd_st7789;
static const nrf_gfx_font_desc_t* font = &orkney_8ptFontInfo;

void gfx_reset()
{
  nrf_delay_ms(10);
  nrf_gpio_pin_clear(ST7789_RST_PIN);
  nrf_delay_ms(10);
  nrf_gpio_pin_set(ST7789_RST_PIN);
}

void gfx_init()
{
  nrf_gpio_cfg_output(ST7789_RST_PIN);
  nrf_gpio_pin_set(ST7789_RST_PIN);
  nrf_gfx_init(lcd);
}

uint16_t screen_colour=0;
void gfx_screen_colour(uint16_t colour)
{
  screen_colour = colour;
}

void gfx_screen_fill()
{
  nrf_gfx_screen_fill(lcd, screen_colour);
}

bool gfx_pixel(uint16_t x, uint16_t y, uint32_t colour)
{
  nrf_gfx_point_t p=NRF_GFX_POINT(x,y);
  nrf_gfx_point_draw(lcd, &p, colour);
  return true;
}

void gfx_draw_area(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t* colours)
{
  int n=(x2-x1+1)*(y2-y1+1)*2;
  nrf_gfx_display(lcd, 0, 0xFFFF, x1, y1, x2, y2);
  nrf_gfx_display(lcd, (uint8_t*)colours, n, 0,0,0,0);
}

bool gfx_rect_line(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t colour, uint16_t thickness)
{
  nrf_gfx_rect_t r=NRF_GFX_RECT(x,y, w,h);
  return nrf_gfx_rect_draw(lcd, &r, thickness, colour, false) == NRF_SUCCESS;
}

bool gfx_rect_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t colour)
{
  nrf_gfx_rect_t r=NRF_GFX_RECT(x,y, w,h);
  return nrf_gfx_rect_draw(lcd, &r, 0, colour, true) == NRF_SUCCESS;
}

uint16_t x_pos=0;
uint16_t y_pos=0;
void gfx_pos(uint16_t x, uint16_t y)
{
  x_pos = x;
  y_pos = y;
}

uint16_t x_pos_save_this_is_not_really_a_stack=0;
uint16_t y_pos_save_this_is_not_really_a_stack=0;
void gfx_push(uint16_t x, uint16_t y)
{
  x_pos_save_this_is_not_really_a_stack=x_pos;
  y_pos_save_this_is_not_really_a_stack=y_pos;
  x_pos = x;
  y_pos = y;
}

void gfx_pop()
{
  x_pos=x_pos_save_this_is_not_really_a_stack;
  y_pos=y_pos_save_this_is_not_really_a_stack;
}

static uint16_t text_colour;
void gfx_text_colour(uint16_t colour)
{
  text_colour = colour;
}

void gfx_text(char* text)
{
  nrf_gfx_point_t text_start = NRF_GFX_POINT(x_pos, y_pos);
  nrf_gfx_print_fast(lcd, &text_start, screen_colour, text_colour, text, font, true);
}

// ---------------------------------
// drawing into huge buffer, plus basic fonts, from ATC1441

#define LCD_BUFFER_SIZE ((ST7789_WIDTH * ST7789_HEIGHT) * 2)
static uint8_t lcd_buffer[LCD_BUFFER_SIZE + 4];

void gfx_fast_init()
{
  display_fast_init();
}

void gfx_fast_set_pixel(int32_t x, int32_t y, uint16_t colour)
{
  if(x<0 || y<0) return;

  int32_t i = 2 * (x + (y * ST7789_WIDTH));

  if(i+1 >= sizeof(lcd_buffer)) return;

  lcd_buffer[i]   = colour >> 8;
  lcd_buffer[i+1] = colour & 0xff;
}

void gfx_fast_display_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, uint16_t colour)
{
  for(int px = x; px < (x + w); px++){
    for(int py = y; py < (y + h); py++){
      gfx_fast_set_pixel(px, py,  colour);
    }
  }
}

bool gfx_fast_draw_char(int32_t x, int32_t y, unsigned char c, uint16_t colour, uint16_t bg, uint32_t size)
{
  if (c < 32) return false;
  if (c >= 127) return false;

  for (int8_t i = 0; i < 5; i++) {

    uint8_t line = font57[c * 5 + i];

    for (int8_t j = 0; j < 8; j++, line >>= 1){

      if(line & 1)          gfx_fast_display_rect(x + i * size, y + j * size, size, size, colour);
      else if(bg != colour) gfx_fast_display_rect(x + i * size, y + j * size, size, size, bg);
    }
  }
  if(bg != colour) gfx_fast_display_rect(x + 5 * size, y, size, 8 * size, bg);

  return true;
}

void gfx_fast_text(int32_t x, int32_t y, char* text, uint16_t colour, uint16_t bg, uint32_t size)
{
  int p = 0;
  for(int i = 0; i < strlen(text); i++){
    if(x + (p * 6 * size) >= (ST7789_WIDTH - 6)){
      x = -(p * 6 * size);
      y += (8 * size);
    }
    if(gfx_fast_draw_char(x + (p * 6 * size), y, text[i], colour, bg, size)) {
      p++;
    }
  }
}

void gfx_fast_clear_screen(uint8_t colour)
{
  memset(lcd_buffer, colour, LCD_BUFFER_SIZE);
}

void gfx_fast_write_out_buffer()
{
  display_fast_write_out_buffer(lcd_buffer, LCD_BUFFER_SIZE);
}


