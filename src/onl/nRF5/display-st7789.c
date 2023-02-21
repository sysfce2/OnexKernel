
// https://www.newhavendisplay.com/appnotes/datasheets/LCDs/ST7789V.pdf

#include "sdk_common.h"

#include "nrf_lcd.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

#include <onex-kernel/gpio.h>
#include <onex-kernel/time.h>
#include "onex-kernel/log.h"
#include "onex-kernel/spi.h"
#include "onex-kernel/display.h"

#define ST7789_NOP         0x00
#define ST7789_SWRESET     0x01
#define ST7789_RDDID       0x04
#define ST7789_RDDST       0x09

#define ST7789_SLPIN       0x10
#define ST7789_SLPOUT      0x11
#define ST7789_PTLON       0x12
#define ST7789_NORON       0x13
#define ST7789_TEON        0x35
#define ST7789_TESCAN      0x44

#define ST7789_RDMODE      0x0A
#define ST7789_RDMADCTL    0x0B
#define ST7789_RDPIXFMT    0x0C
#define ST7789_RDIMGFMT    0x0D
#define ST7789_RDSELFDIAG  0x0F

#define ST7789_INVOFF      0x20
#define ST7789_INVON       0x21
#define ST7789_GAMMASET    0x26
#define ST7789_DISPOFF     0x28
#define ST7789_DISPON      0x29

#define ST7789_CASET       0x2A
#define ST7789_RASET       0x2B
#define ST7789_RAMWR       0x2C
#define ST7789_RAMRD       0x2E

#define ST7789_PTLAR       0x30
#define ST7789_MADCTL      0x36
#define ST7789_COLMOD      0x3A

#define ST7789_FRMCTR1     0xB1
#define ST7789_RGBCTRL     0xB1
#define ST7789_PORCTR      0xB2
//#define ST7789_FRMCTR2     0xB2
#define ST7789_FRMCTR3     0xB3
#define ST7789_INVCTR      0xB4
#define ST7789_DFUNCTR     0xB6
#define ST7789_GCTRL       0xB7

#define ST7789_VCOM        0xBB

#define ST7789_LCMCTR      0xC0
//#define ST7789_PWCTR1      0xC0
#define ST7789_PWCTR2      0xC1
#define ST7789_VDVVRHEN    0xC2
//#define ST7789_PWCTR3      0xC2
#define ST7789_VRHS        0xC3
//#define ST7789_PWCTR4      0xC3
#define ST7789_VDVS        0xC4
//#define ST7789_PWCTR5      0xC4
#define ST7789_VMCTR1      0xC5
#define ST7789_FRMCTR2     0xC6
#define ST7789_VMCTR2      0xC7
#define ST7789_PWCTRSEQ    0xCB
#define ST7789_PWCTRA      0xCD
#define ST7789_PWCTRB      0xCF

#define ST7789_PWCTRL1     0xD0

#define ST7789_RDID1       0xDA
#define ST7789_RDID2       0xDB
#define ST7789_RDID3       0xDC
#define ST7789_RDID4       0xDD

#define ST7789_PVGAMCTRL     0xE0
#define ST7789_NVGAMCTRL     0xE1
#define ST7789_DGMCTR1     0xE2
#define ST7789_DGMCTR2     0xE3
#define ST7789_SPI2EN      0xE7
#define ST7789_TIMCTRA     0xE8
#define ST7789_TIMCTRB     0xEA

#define ST7789_ENGMCTR     0xF2
#define ST7789_INCTR       0xF6
#define ST7789_PUMP        0xF7

#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00 //replace BGR with this to get normal coloring
#define ST7789_MADCTL_BGR 0x08 //used by default
#define ST7789_MADCTL_MH  0x04

static inline void write_command(uint8_t c)
{
    while(spi_sending());
    nrf_gpio_pin_clear(ST7789_DC_PIN);
    spi_tx(&c, sizeof(c), 0);
}

static inline void write_data(uint8_t c)
{
    while(spi_sending());
    nrf_gpio_pin_set(ST7789_DC_PIN);
    spi_tx(&c, sizeof(c), 0);
}

static inline void write_command_buffered(uint8_t * c, uint16_t len)
{
    while(spi_sending());
    nrf_gpio_pin_clear(ST7789_DC_PIN);
    spi_tx(c, len, 0);
}

static inline void write_data_buffered(uint8_t * c, uint16_t len)
{
    while(spi_sending());
    nrf_gpio_pin_set(ST7789_DC_PIN);
    spi_tx(c, len, 0);
}

static inline void write_data_buffered_cb(uint8_t * c, uint16_t len, void (*cb)())
{
    while(spi_sending());
    nrf_gpio_pin_set(ST7789_DC_PIN);
    spi_tx(c, len, cb);
}

static void set_addr_window(uint16_t x_0, uint16_t y_0, uint16_t x_1, uint16_t y_1)
{
    ASSERT(x_0 <= x_1);
    ASSERT(y_0 <= y_1);

#if defined(ST7789_ADDR_HEIGHT)
    y_0 = y_0 + (ST7789_ADDR_HEIGHT - ST7789_HEIGHT);
    y_1 = y_1 + (ST7789_ADDR_HEIGHT - ST7789_HEIGHT);
#endif
    uint8_t data[4];
    write_command(ST7789_CASET);
    data[0] = x_0 >> 8;
    data[1] = x_0;
    data[2] = x_1 >> 8;
    data[3] = x_1;
    write_data_buffered(data, 4);
    write_command(ST7789_RASET);
    data[0]=y_0 >> 8;
    data[1]=y_0;
    data[2]=y_1 >> 8;
    data[3]=y_1;
    write_data_buffered(data, 4);
    write_command(ST7789_RAMWR);
}

static void st7789_rotation_set(nrf_lcd_rotation_t rotation)
{
    write_command(ST7789_MADCTL);
    switch (rotation % 4) {
        case NRF_LCD_ROTATE_0:
            write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);  //Not working correctly
            //Column address (MX): Right to left
            //Page address (MY): Bottom to top
            //Page/ Column order (MV): normal
            //RGB/BGR order: RGB
            break;
        case NRF_LCD_ROTATE_90:
            write_data(ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            //Column address (MX): Left to right
            //Page address (MY): Top to bottom
            //Page/ Column order (MV): reverse
            //RGB/BGR order: RGB
            break;
        case NRF_LCD_ROTATE_180:
            write_data(ST7789_MADCTL_RGB);
            //Column address (MX): Left to right
            //Page address (MY): Top to bottom
            //Page/ Column order (MV): normal
            //RGB/BGR order: RGB
            break;
        case NRF_LCD_ROTATE_270:
            write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            //Column address (MX): Right to left
            //Page address (MY): Top to bottom
            //Page/ Column order (MV): reverse
            //RGB/BGR order: RGB
            break;
        default:
            break;
    }
}

static void init_command_list(void)
{
    write_command(ST7789_SWRESET);
    nrf_delay_ms(125);

    write_command(ST7789_SLPOUT);
    nrf_delay_ms(150);

    write_command(ST7789_COLMOD);
    write_data(0x55); //16-bit, 565 RGB

    st7789_rotation_set(NRF_LCD_ROTATE_180);

    write_command(ST7789_INVON);
    nrf_delay_ms(10);

    write_command(ST7789_NORON);
    nrf_delay_ms(10);

    write_command(ST7789_DISPON);
}

static ret_code_t st7789_init(void)
{
  nrf_gpio_cfg_output(ST7789_DC_PIN);

  ret_code_t e;
  e=spi_init();
  if(e!=NRF_SUCCESS) return e;

  init_command_list();

  return e;
}

static void st7789_uninit(void)
{
}

static void st7789_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    set_addr_window(x, y, x, y);

    uint8_t data[2] = {color >> 8, color};

    write_data_buffered(data, sizeof(data));
}

static void st7789_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    set_addr_window(x, y, x + width - 1, y + height - 1);

    uint8_t data[2] = {color >> 8, color};

    while(spi_sending());
    nrf_gpio_pin_set(ST7789_DC_PIN);

    // Duff's device algorithm for optimizing loop.
    uint32_t i = (height * width + 7) / 8;

    switch ((height * width) % 8) {
        case 0:
            do {
                spi_tx(data, sizeof(data), 0);
        case 7:
                spi_tx(data, sizeof(data), 0);
        case 6:
                spi_tx(data, sizeof(data), 0);
        case 5:
                spi_tx(data, sizeof(data), 0);
        case 4:
                spi_tx(data, sizeof(data), 0);
        case 3:
                spi_tx(data, sizeof(data), 0);
        case 2:
                spi_tx(data, sizeof(data), 0);
        case 1:
                spi_tx(data, sizeof(data), 0);
            } while (--i > 0);
        default:
            break;
    }
}

static void st7789_display(uint8_t * data, uint16_t len, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if(len == 0xFFFF){
        set_addr_window(x0, y0, x1, y1);
    }
    else{
        write_data_buffered(data, len);
    }
}

static void st7789_display_invert(bool invert)
{
    write_command(invert ? ST7789_INVON : ST7789_INVOFF);
}

static lcd_cb_t st7789_cb = {
    .height = ST7789_HEIGHT,
    .width = ST7789_WIDTH
};


const nrf_lcd_t nrf_lcd_st7789 = {
    .lcd_init = st7789_init,
    .lcd_uninit = st7789_uninit,
    .lcd_pixel_draw = st7789_pixel_draw,
    .lcd_rect_draw = st7789_rect_draw,
    .lcd_display = st7789_display,
    .lcd_rotation_set = st7789_rotation_set,
    .lcd_display_invert = st7789_display_invert,
    .p_lcd_cb = &st7789_cb
};

// ------------------------------------------

void display_init()
{
  nrf_gpio_cfg_output(ST7789_RST_PIN);
  nrf_gpio_cfg_output(ST7789_DC_PIN);
  nrf_gpio_pin_set(ST7789_RST_PIN);

  spi_init();

  init_command_list();
}

uint64_t time_ready_after_wake_command=0;

static bool sleeping=false;

void display_draw_area(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t* pixels, void (*cb)())
{
  if(sleeping){
    if(cb) cb();
    return;
  }

  if(time_ready_after_wake_command){
    int32_t time_till_ready=time_ready_after_wake_command-time_ms();
    if(time_till_ready>0) time_delay_ms(time_till_ready);
    time_ready_after_wake_command=0;
  }

  set_addr_window(x1, y1, x2, y2);

  int n=(x2-x1+1)*(y2-y1+1)*2;
  write_data_buffered_cb((uint8_t*)pixels, n, cb);
}

#define SPI_FLUSH_TIME          2
#define ST7789_WAKE_SETTLE_TIME 5

void display_sleep()
{
  if(sleeping) return;
  sleeping=true;

  write_command(ST7789_SLPIN);
}

void display_wake()
{
  if(!sleeping) return;
  sleeping=false;

  write_command(ST7789_SLPOUT);

  time_ready_after_wake_command=time_ms()+SPI_FLUSH_TIME+ST7789_WAKE_SETTLE_TIME;
}

void display_reset()
{
  nrf_delay_ms(10);
  nrf_gpio_pin_clear(ST7789_RST_PIN);
  nrf_delay_ms(10);
  nrf_gpio_pin_set(ST7789_RST_PIN);
}

// ------------------------------------------

#if defined(NRF52840_XXAA)

void start_write_fast(void)
{
  spi_fast_enable(true);
  gpio_set(SPIM_SS_PIN , 0);
}

void end_write_fast(void)
{
  gpio_set(SPIM_SS_PIN , 1);
  spi_fast_enable(false);
}

void write_command_fast(uint8_t d)
{
  gpio_set(ST7789_DC_PIN , 0);
  spi_fast_write(&d, 1);
  gpio_set(ST7789_DC_PIN , 1);
}

void write_char_fast(uint8_t d)
{
  spi_fast_write(&d, 1);
}

static void set_addr_window_fast(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
#if defined(ST7789_ADDR_HEIGHT)
  y = y + (ST7789_ADDR_HEIGHT - ST7789_HEIGHT);
#endif
  write_command_fast(ST7789_CASET);
  write_char_fast((x) >> 8);
  write_char_fast(x);
  write_char_fast((x + w - 1) >> 8);
  write_char_fast(x + w - 1);
  write_command_fast(ST7789_RASET);
  write_char_fast((y) >> 8);
  write_char_fast(y);
  write_char_fast(((y + h - 1) ) >> 8);
  write_char_fast((y + h - 1) );
  write_command_fast(ST7789_RAMWR);
}

void display_reset_kinda_slow()
{
  gpio_set(ST7789_RST_PIN, 1);
  time_delay_ms(20);
  gpio_set(ST7789_RST_PIN, 0);
  time_delay_ms(100);
  gpio_set(ST7789_RST_PIN, 1);
  time_delay_ms(100);
}

static void st7789_rotation_set_fast(nrf_lcd_rotation_t rotation)
{
    write_command_fast(ST7789_MADCTL);

    switch (rotation % 4) {
        case NRF_LCD_ROTATE_0:
            write_char_fast(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
            break;
        case NRF_LCD_ROTATE_90:
            write_char_fast(ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            break;
        case NRF_LCD_ROTATE_180:
            write_char_fast(ST7789_MADCTL_RGB);
            break;
        case NRF_LCD_ROTATE_270:
            write_char_fast(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            break;
        default:
            break;
    }
}

void init_command_list_fast()
{
  start_write_fast();

  write_command_fast(ST7789_SLPOUT);
  time_delay_ms(120);

  st7789_rotation_set_fast(NRF_LCD_ROTATE_180);

  write_command_fast(ST7789_COLMOD);
  write_char_fast(0x55); //16-bit, 565 RGB ATC had 5?
/*
  write_command_fast(ST7789_PORCTR);
  write_char_fast(0xB);
  write_char_fast(0xB);
  write_char_fast(0x33);
  write_char_fast(0x0);
  write_char_fast(0x33);

  write_command_fast(ST7789_GCTRL);
  write_char_fast(0x11);

  write_command_fast(ST7789_VCOM);
  write_char_fast(0x35);

  write_command_fast(ST7789_LCMCTR);
  write_char_fast(0x2c);

  write_command_fast(ST7789_VDVVRHEN);
  write_char_fast(1);
  write_command_fast(ST7789_VRHS);
  write_char_fast(8);
  write_command_fast(ST7789_VDVS);
  write_char_fast(0x20);
  write_command_fast(ST7789_FRMCTR2);
  write_char_fast(0x1f);

  write_command_fast(ST7789_PWCTRL1);
  write_char_fast(0xa4);
  write_char_fast(0xa1);

  write_command_fast(ST7789_PVGAMCTRL);
  write_char_fast(0xF0);
  write_char_fast(0x04);
  write_char_fast(0x0A);
  write_char_fast(0x0A);
  write_char_fast(0x08);
  write_char_fast(0x25);
  write_char_fast(0x33);
  write_char_fast(0x27);
  write_char_fast(0x3D);
  write_char_fast(0x38);
  write_char_fast(0x14);
  write_char_fast(0x14);
  write_char_fast(0x25);
  write_char_fast(0x2A);

  write_command_fast(ST7789_NVGAMCTRL);
  write_char_fast(0xF0);
  write_char_fast(0x05);
  write_char_fast(0x08);
  write_char_fast(0x07);
  write_char_fast(0x06);
  write_char_fast(0x02);
  write_char_fast(0x26);
  write_char_fast(0x32);
  write_char_fast(0x3D);
  write_char_fast(0x3A);
  write_char_fast(0x16);
  write_char_fast(0x16);
  write_char_fast(0x26);
  write_char_fast(0x2C);
*/
  write_command_fast(ST7789_INVON);

  write_command_fast(ST7789_TEON);
  write_char_fast(0x0);

  write_command_fast(ST7789_TESCAN);
  write_char_fast(0x25);
  write_char_fast(0x0);
  time_delay_ms(120);

  write_command_fast(ST7789_DISPON);

  set_addr_window_fast(0, 0, ST7789_WIDTH, ST7789_HEIGHT);

  end_write_fast();
}

void display_fast_init()
{
  spi_fast_init();

  gpio_mode(SPIM_SS_PIN, OUTPUT);
  gpio_mode(ST7789_DC_PIN, OUTPUT);
  gpio_mode(ST7789_RST_PIN, OUTPUT);

  gpio_set(SPIM_SS_PIN , 1);
  gpio_set(ST7789_DC_PIN , 1);

  display_reset_kinda_slow();

  init_command_list_fast();
}

void display_fast_write_out_buffer(uint8_t* buf, uint32_t size)
{
  if(time_ready_after_wake_command){
    int32_t time_till_ready=time_ready_after_wake_command-time_ms();
    if(time_till_ready>0) time_delay_ms(time_till_ready);
    time_ready_after_wake_command=0;
  }
  start_write_fast();
  spi_fast_write(buf, size);
  end_write_fast();
}

#define SPI_FLUSH_TIME_FAST 29

void display_fast_sleep() {

  if(sleeping) return;
  sleeping=true;

  write_command_fast(ST7789_SLPIN);
}

void display_fast_wake() {

  if(!sleeping) return;
  sleeping=false;

  write_command_fast(ST7789_SLPOUT);

  time_ready_after_wake_command=time_ms()+SPI_FLUSH_TIME_FAST+ST7789_WAKE_SETTLE_TIME;
}

#endif // NRF52840_XXAA-only



