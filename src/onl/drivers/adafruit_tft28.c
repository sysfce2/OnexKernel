/***************************************************
  This is our library for the Adafruit  ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <variant.h>
#include <time.h>
#include <gpio.h>
#include <serial.h>
#include <spi.h>
#include <tft.h>

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0A
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF  0x20
#define ILI9341_INVON   0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29

#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_RAMRD   0x2E

#define ILI9341_PTLAR   0x30
#define ILI9341_MADCTL  0x36
#define ILI9341_PIXFMT  0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1
/*
#define ILI9341_PWCTR6  0xFC
*/

int16_t tft_width = 0;
int16_t tft_height = 0;
uint8_t tft_rotation = 0;

static int8_t cspin = PIN_SPI_SS;
static int8_t dcpin = PIN_TFT_DC;

static void write_command(uint8_t c)
{
  gpio_set(dcpin, LOW);
  gpio_set(cspin, LOW);
  spi_transfer(c);
  gpio_set(cspin, HIGH);
}

static void write_data(uint8_t c)
{
  gpio_set(dcpin, HIGH);
  gpio_set(cspin, LOW);
  spi_transfer(c);
  gpio_set(cspin, HIGH);
}

static void write_word(uint16_t w)
{
  gpio_set(dcpin, HIGH);
  gpio_set(cspin, LOW);
  spi_transfer(w >> 8);
  spi_transfer(w);
  gpio_set(cspin, HIGH);
}

void tft_init(int8_t cs, int8_t dc)
{
  cspin = cs;
  dcpin = dc;

  gpio_mode(dcpin, OUTPUT);
  gpio_mode(cspin, OUTPUT);

  spi_begin(0,0,0);
  spi_set_speed(70000);
  spi_set_bit_order(MSBFIRST);
  spi_set_data_mode(SPI_MODE0);

  tft_width = ILI9341_TFTWIDTH;
  tft_height = ILI9341_TFTHEIGHT;
  tft_rotation = 0;

  time_delay_ms(300); //??
  write_command(0xEF);
  write_data(0x03);
  write_data(0x80);
  write_data(0x02);

  write_command(0xCF);
  write_data(0x00);
  write_data(0XC1);
  write_data(0X30);

  write_command(0xED);
  write_data(0x64);
  write_data(0x03);
  write_data(0X12);
  write_data(0X81);

  write_command(0xE8);
  write_data(0x85);
  write_data(0x00);
  write_data(0x78);

  write_command(0xCB);
  write_data(0x39);
  write_data(0x2C);
  write_data(0x00);
  write_data(0x34);
  write_data(0x02);

  write_command(0xF7);
  write_data(0x20);

  write_command(0xEA);
  write_data(0x00);
  write_data(0x00);

  write_command(ILI9341_PWCTR1);    //Power control
  write_data(0x23);   //VRH[5:0]

  write_command(ILI9341_PWCTR2);    //Power control
  write_data(0x10);   //SAP[2:0];BT[3:0]

  write_command(ILI9341_VMCTR1);    //VCM control
  write_data(0x3e); //¶Ô±È¶Èµ÷½Ú
  write_data(0x28);

  write_command(ILI9341_VMCTR2);    //VCM control2
  write_data(0x86);  //--

  write_command(ILI9341_MADCTL);    // Memory Access Control
  write_data(0x48);

  write_command(ILI9341_PIXFMT);
  write_data(0x55);

  write_command(ILI9341_FRMCTR1);
  write_data(0x00);
  write_data(0x18);

  write_command(ILI9341_DFUNCTR);    // Display Function Control
  write_data(0x08);
  write_data(0x82);
  write_data(0x27);

  write_command(0xF2);    // 3Gamma Function Disable
  write_data(0x00);

  write_command(ILI9341_GAMMASET);    //Gamma curve selected
  write_data(0x01);

  write_command(ILI9341_GMCTRP1);    //Set Gamma
  write_data(0x0F);
  write_data(0x31);
  write_data(0x2B);
  write_data(0x0C);
  write_data(0x0E);
  write_data(0x08);
  write_data(0x4E);
  write_data(0xF1);
  write_data(0x37);
  write_data(0x07);
  write_data(0x10);
  write_data(0x03);
  write_data(0x0E);
  write_data(0x09);
  write_data(0x00);

  write_command(ILI9341_GMCTRN1);    //Set Gamma
  write_data(0x00);
  write_data(0x0E);
  write_data(0x14);
  write_data(0x03);
  write_data(0x11);
  write_data(0x07);
  write_data(0x31);
  write_data(0xC1);
  write_data(0x48);
  write_data(0x08);
  write_data(0x0F);
  write_data(0x0C);
  write_data(0x31);
  write_data(0x36);
  write_data(0x0F);

  write_command(ILI9341_SLPOUT);
  time_delay_ms(120);
  write_command(ILI9341_DISPON);    //Display on
}

static void set_drawing_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  write_command(ILI9341_CASET); // Column addr set
  write_data(x0 >> 8);
  write_data(x0 & 0xFF);     // XSTART
  write_data(x1 >> 8);
  write_data(x1 & 0xFF);     // XEND

  write_command(ILI9341_PASET); // Row addr set
  write_data(y0>>8);
  write_data(y0);     // YSTART
  write_data(y1>>8);
  write_data(y1);     // YEND

  write_command(ILI9341_RAMWR); // write to RAM
}

void tft_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
  if((x < 0) ||(x >= tft_width) || (y < 0) || (y >= tft_height)) return;
  set_drawing_rectangle(x,y,x+1,y+1);
  write_word(color);
}

void tft_draw_vertical_line(int16_t x, int16_t y, int16_t h, uint16_t color)
{

  if((x >= tft_width) || (y >= tft_height)) return;

  if((y+h-1) >= tft_height)
    h = tft_height-y;

  set_drawing_rectangle(x, y, x, y+h-1);

  uint8_t hi = color >> 8, lo = color;

  gpio_set(dcpin, HIGH);
  gpio_set(cspin, LOW);

  while (h--) {
    spi_transfer(hi);
    spi_transfer(lo);
  }
  gpio_set(cspin, HIGH);
}

void tft_draw_horizontal_line(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  if((x >= tft_width) || (y >= tft_height)) return;
  if((x+w-1) >= tft_width)  w = tft_width-x;
  set_drawing_rectangle(x, y, x+w-1, y);

  uint8_t hi = color >> 8, lo = color;
  gpio_set(dcpin, HIGH);
  gpio_set(cspin, LOW);
  while (w--) {
    spi_transfer(hi);
    spi_transfer(lo);
  }
  gpio_set(cspin, HIGH);
}

void tft_fill_rectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour) {

  if((x >= tft_width) || (y >= tft_height)) return;
  if((x + w - 1) >= tft_width)  w = tft_width  - x;
  if((y + h - 1) >= tft_height) h = tft_height - y;

  set_drawing_rectangle(x, y, x+w-1, y+h-1);

  gpio_set(dcpin, HIGH);
  gpio_set(cspin, LOW);

  for(y=h; y>0; y--) {
    for(x=w; x>0; x--) {
      spi_transfer_16(colour);
    }
  }

  gpio_set(cspin, HIGH);
}

void tft_fill_screen(uint16_t color)
{
  tft_fill_rectangle(0, 0,  tft_width, tft_height, color);
}


#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

void tft_set_rotation(uint8_t m)
{

  write_command(ILI9341_MADCTL);
  tft_rotation = m % 4; // can't be higher than 3
  switch (tft_rotation) {
   case 0:
     write_data(MADCTL_MX | MADCTL_BGR);
     tft_width  = ILI9341_TFTWIDTH;
     tft_height = ILI9341_TFTHEIGHT;
     break;
   case 1:
     write_data(MADCTL_MV | MADCTL_BGR);
     tft_width  = ILI9341_TFTHEIGHT;
     tft_height = ILI9341_TFTWIDTH;
     break;
  case 2:
    write_data(MADCTL_MY | MADCTL_BGR);
     tft_width  = ILI9341_TFTWIDTH;
     tft_height = ILI9341_TFTHEIGHT;
    break;
   case 3:
     write_data(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
     tft_width  = ILI9341_TFTHEIGHT;
     tft_height = ILI9341_TFTWIDTH;
     break;
  }
}

static void invert_display(bool i)
{
  write_command(i ? ILI9341_INVON : ILI9341_INVOFF);
}

void tft_draw_rectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  tft_draw_horizontal_line(x, y, w, color);
  tft_draw_horizontal_line(x, y+h-1, w, color);
  tft_draw_vertical_line(x, y, h, color);
  tft_draw_vertical_line(x+w-1, y, h, color);
}

void tft_render(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* fb) {

  if((x >= tft_width) || (y >= tft_height)) return;
  if((x + w - 1) >= tft_width)  w = tft_width  - x;
  if((y + h - 1) >= tft_height) h = tft_height - y;

  set_drawing_rectangle(x, y, x+w-1, y+h-1);

  gpio_set(dcpin, HIGH);
  gpio_set(cspin, LOW);

  spi_transfer_buf_16(fb, w*h);

  gpio_set(cspin, HIGH);
}


