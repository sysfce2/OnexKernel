#ifndef GFX_H
#define GFX_H

#define GFX_WHITE   0xffff
#define GFX_BLACK   0x0000
#define GFX_GREY_F  0x7bef // 0.1.1.1:1/0.1.1:1.1.1/0:1.1.1.1
#define GFX_GREY_7  0x39e7 // 0.0.1.1:1/0.0.1:1.1.1/0:0.1.1.1
#define GFX_GREY_3  0x18e3 // 0.0.0.1:1/0.0.0:1.1.1/0:0.0.1.1
#define GFX_GREY_1  0x0861 // 0.0.0.0:1/0.0.0:0.1.1/0:0.0.0.1
#define GFX_RED     0xf800 // 1.1.1.1:1/0.0.0:0.0.0/0:0.0.0.0
#define GFX_GREEN   0x07e0 // 0.0.0.0:0/1.1.1:1.1.1/0:0.0.0.0
#define GFX_BLUE    0x001f // 0.0.0.0:0/0.0.0:0.0.0/1:1.1.1.1
#define GFX_YELLOW  0xffe0 // 1.1.1.1:1/1.1.1:1.1.1/0:0.0.0.0
#define GFX_MAGENTA 0xf81f // 1.1.1.1:1/0.0.0:0.0.0/1:1.1.1.1
#define GFX_CYAN    0x07ff // 0.0.0.0:0/1.1.1:1.1.1/1:1.1.1.1

#define GFX_RGB256(r,g,b) ((((uint16_t)(r)&0xf8)<<8)|(((uint16_t)(g)&0xfc)<<3)|((uint16_t)(b)>>3))

void gfx_reset();
void gfx_init();

void gfx_screen_colour(uint16_t colour);
void gfx_text_colour(uint16_t colour);

void gfx_screen_fill();

void gfx_draw_area(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t* colours);
bool gfx_pixel(uint16_t x, uint16_t y, uint32_t colour);
bool gfx_rect_line(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t colour, uint16_t thickness);
bool gfx_rect_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t colour);

void gfx_pos(uint16_t x, uint16_t y);
void gfx_push(uint16_t x, uint16_t y);
void gfx_pop();

void gfx_text(char* text);

void gfx_fast_init();
void gfx_fast_clear_screen(uint8_t colour); // 8 bits * 2, so 0x00 => 0x0000, 0x77 => 0x7777, etc
void gfx_fast_text(int32_t x, int32_t y, char* text, uint16_t colour, uint16_t bg, uint32_t size);
void gfx_fast_write_out_buffer();

#endif

