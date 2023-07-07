/*
Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <tft.h>
#include <glcdfont.c>

static int16_t  cursor_x = 0;
static int16_t  cursor_y = 0;
static uint8_t  textsize = 1;
static uint16_t textcolor = 0xFFFF;
static uint16_t textbgcolor = 0xFFFF;
static bool     wrap = false;

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

void tft_set_text_colour(uint16_t c)
{
  textcolor = textbgcolor = c;
}

void tft_set_text_size(uint8_t s)
{
  textsize = (s > 0) ? s : 1;
}

static void draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size)
{
  if((x >= tft_width)            || // Clip right
     (y >= tft_height)           || // Clip bottom
     ((x + 6 * size - 1) < 0) || // Clip left
     ((y + 8 * size - 1) < 0))   // Clip top
    return;

  for (int8_t i=0; i<6; i++ ) {
    uint8_t line;
    if (i == 5)
      line = 0x0;
    else
      line = pgm_read_byte(font+(c*5)+i);
    for (int8_t j = 0; j<8; j++) {
      if (line & 0x1) {
        if (size == 1) // default size
          tft_draw_pixel(x+i, y+j, color);
        else {  // big size
          tft_fill_rectangle(x+(i*size), y+(j*size), size, size, color);
        }
      } else if (bg != color) {
        if (size == 1) // default size
          tft_draw_pixel(x+i, y+j, bg);
        else {  // big size
          tft_fill_rectangle(x+i*size, y+j*size, size, size, bg);
        }
      }
      line >>= 1;
    }
  }
}

static size_t write(uint8_t c)
{
  if (c == '\n') {
    cursor_y += textsize*8;
    cursor_x  = 0;
  } else if (c == '\r') {
    // skip em
  } else {
    draw_char(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
    cursor_x += textsize*6;
    if (wrap && (cursor_x > (tft_width - textsize*6))) {
      cursor_y += textsize*8;
      cursor_x = 0;
    }
  }
  return 1;
}

static size_t write_buffer(const uint8_t *buffer, size_t size)
{
  size_t n = 0;
  while (size--) {
    if (write(*buffer++)) n++;
    else break;
  }
  return n;
}

void tft_set_cursor(int16_t x, int16_t y)
{
  if((x < 0) ||(x >= tft_width) || (y < 0) || (y >= tft_height)) return;
  cursor_x = x;
  cursor_y = y;
}

size_t tft_draw_text(const uint8_t *buffer)
{
  return write_buffer(buffer, strlen(buffer));
}

#define abs(x) ((x)>0?(x):-(x))
#define swap(a, b) { int16_t t = a; a = b; b = t; }

static void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      tft_draw_pixel(y0, x0, color);
    } else {
      tft_draw_pixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

static void draw_line_better(int16_t x0, int16_t y0,int16_t x1, int16_t y1, uint16_t color)
{
  if (y0 == y1) {
    if (x1 > x0) {
      tft_draw_horizontal_line(x0, y0, x1 - x0 + 1, color);
    } else if (x1 < x0) {
      tft_draw_horizontal_line(x1, y0, x0 - x1 + 1, color);
    } else {
      tft_draw_pixel(x0, y0, color);
    }
    return;
  } else if (x0 == x1) {
    if (y1 > y0) {
      tft_draw_vertical_line(x0, y0, y1 - y0 + 1, color);
    } else {
      tft_draw_vertical_line(x0, y1, y0 - y1 + 1, color);
    }
    return;
  }

  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }


  int16_t xbegin = x0;
  if (steep) {
    for (; x0<=x1; x0++) {
      err -= dy;
      if (err < 0) {
        int16_t len = x0 - xbegin;
        if (len) {
          tft_draw_vertical_line(y0, xbegin, len + 1, color);
        } else {
          tft_draw_pixel(y0, x0, color);
        }
        xbegin = x0 + 1;
        y0 += ystep;
        err += dx;
      }
    }
    if (x0 > xbegin + 1) {
      tft_draw_vertical_line(y0, xbegin, x0 - xbegin, color);
    }

  } else {
    for (; x0<=x1; x0++) {
      err -= dy;
      if (err < 0) {
        int16_t len = x0 - xbegin;
        if (len) {
          tft_draw_horizontal_line(xbegin, y0, len + 1, color);
        } else {
          tft_draw_pixel(x0, y0, color);
        }
        xbegin = x0 + 1;
        y0 += ystep;
        err += dx;
      }
    }
    if (x0 > xbegin + 1) {
      tft_draw_horizontal_line(xbegin, y0, x0 - xbegin, color);
    }
  }
//spi_tee_write_command_done(CMD_NOP);
}

static uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t rgb24_to_565(int32_t c)
{
 return ((((c >> 16) & 0xFF) / 8) << 11) | ((((c >> 8) & 0xFF) / 4) << 5) | (((c) &  0xFF) / 8);
}

