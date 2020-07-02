
#include <stdint.h>
#include <nrf_gpio.h>
#include <nrf_gfx.h>
#include <nrf_delay.h>
#include <onex-kernel/gfx.h>

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

void gfx_sleep()
{
  nrf_gfx_on_off(lcd, false);
}

void gfx_wake()
{
  nrf_gfx_on_off(lcd, true);
}
