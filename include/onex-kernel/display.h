#ifndef DISPLAY_H
#define DISPLAY_H

void display_init();
void display_draw_area(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t* pixels, void (*cb)());
void display_sleep();
void display_wake();
void display_reset();

#endif
