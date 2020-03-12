#ifndef GFX_H
#define GFX_H

void gfx_init();

void gfx_screen_colour(uint16_t colour);
void gfx_text_colour(uint16_t colour);

void gfx_screen_fill();

void gfx_pos(uint16_t x, uint16_t y);
void gfx_text(char* text);

#endif

