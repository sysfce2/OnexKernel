
#include "sdk_common.h"

#if NRF_MODULE_ENABLED(ST7789)

#include "nrf_lcd.h"
#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

#include "onex-kernel/spi.h"

// Set of commands described in ST7789 datasheet.
#define ST7789_NOP         0x00
#define ST7789_SWRESET     0x01
#define ST7789_RDDID       0x04
#define ST7789_RDDST       0x09

#define ST7789_SLPIN       0x10
#define ST7789_SLPOUT      0x11
#define ST7789_PTLON       0x12
#define ST7789_NORON       0x13

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

uint16_t x_offset = 0;
uint16_t y_offset = 0;

// Datasheet: https://www.numworks.com/shared/binary/datasheets/st7789v-lcd-controller-73f8bc3e.pdf

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(ST7789_SPI_INSTANCE);

static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(ST7789_DC_PIN);
    spi_tx(&c, sizeof(c), 0);
}

static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(ST7789_DC_PIN);
    spi_tx(&c, sizeof(c), 0);
}

static inline void write_command_buffered(uint8_t * c, uint16_t len)
{
    nrf_gpio_pin_clear(ST7789_DC_PIN);
    spi_tx(c, len, 0);
}

static inline void write_data_buffered(uint8_t * c, uint16_t len)
{
    nrf_gpio_pin_set(ST7789_DC_PIN);
    spi_tx(c, len, 0);
}

static inline void write_data_buffered_cb(uint8_t * c, uint16_t len, void (*cb)())
{
    nrf_gpio_pin_set(ST7789_DC_PIN);
    spi_tx(c, len, cb);
}

static void set_addr_window(uint16_t x_0, uint16_t y_0, uint16_t x_1, uint16_t y_1)
{
    ASSERT(x_0 <= x_1);
    ASSERT(y_0 <= y_1);

    y_0 += y_offset;
    y_1 += y_offset;
    x_0 += x_offset;
    x_1 += x_offset;

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

static void init_command_list(void)
{
    write_command(ST7789_SWRESET);
    nrf_delay_ms(125);

    write_command(ST7789_SLPOUT);
    nrf_delay_ms(150);

    write_command(ST7789_COLMOD);
    write_data(0x55); //16-bit, 565 RGB

    write_command(ST7789_MADCTL);
    write_data(0x00); //Bottom to top page address order

    write_command(ST7789_INVON);
    nrf_delay_ms(10);

    write_command(ST7789_NORON);
    nrf_delay_ms(10);

    write_command(ST7789_DISPON);
}

static ret_code_t init_spi(void)
{
    nrf_gpio_cfg_output(ST7789_DC_PIN);
    return spi_init();
}

static ret_code_t st7789_init(void)
{
    ret_code_t err_code;

    err_code = init_spi();

    if(err_code != NRF_SUCCESS) return err_code;

    init_command_list();

    return err_code;
}

static void st7789_uninit(void)
{
    nrf_drv_spi_uninit(&spi);
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

    nrf_gpio_pin_clear(ST7789_DC_PIN);
}

static void st7789_dummy_display(uint8_t * data, uint16_t len, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if(len == 0xFFFF){
        set_addr_window(x0, y0, x1, y1);
    }
    else{
        write_data_buffered(data, len);
    }
}

static void st7789_dummy_display_cb(uint8_t * data, uint16_t len, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, void (*cb)())
{
    if(len == 0xFFFF){
        set_addr_window(x0, y0, x1, y1);
    }
    else{
        write_data_buffered_cb(data, len, cb);
    }
}

static void st7789_rotation_set(nrf_lcd_rotation_t rotation)
{
    write_command(ST7789_MADCTL);
    switch (rotation % 4) {
        case NRF_LCD_ROTATE_0:
            write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);  //Not working correctly
            x_offset = 0;
            y_offset = 0;
            //Column address (MX): Right to left
            //Page address (MY): Bottom to top
            //Page/ Column order (MV): normal
            //RGB/BGR order: RGB
            break;
        case NRF_LCD_ROTATE_90:
            write_data(ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            x_offset = 0;
            y_offset = 0;
            //Column address (MX): Left to right
            //Page address (MY): Top to bottom
            //Page/ Column order (MV): reverse
            //RGB/BGR order: RGB
            break;
        case NRF_LCD_ROTATE_180:
            write_data(ST7789_MADCTL_RGB);
            x_offset = 0;
            y_offset = 0;
            //Column address (MX): Left to right
            //Page address (MY): Top to bottom
            //Page/ Column order (MV): normal
            //RGB/BGR order: RGB
            break;
        case NRF_LCD_ROTATE_270:
            write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            x_offset = 0;
            y_offset = 0;
            //Column address (MX): Right to left
            //Page address (MY): Top to bottom
            //Page/ Column order (MV): reverse
            //RGB/BGR order: RGB
            break;
        default:
            break;
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
    .lcd_display = st7789_dummy_display,
    .lcd_rotation_set = st7789_rotation_set,
    .lcd_display_invert = st7789_display_invert,
    .p_lcd_cb = &st7789_cb
};

void display_init()
{
  nrf_gpio_cfg_output(ST7789_RST_PIN);
  nrf_gpio_pin_set(ST7789_RST_PIN);
  st7789_init();
}

void display_draw_area(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t* colours, void (*cb)())
{
  int n=(x2-x1+1)*(y2-y1+1)*2;
  st7789_dummy_display(0, 0xFFFF, x1, y1, x2, y2);
  st7789_dummy_display_cb((uint8_t*)colours, n, 0,0,0,0, cb);
}

void display_sleep()
{
  write_command(ST7789_DISPOFF);
  write_command(ST7789_SLPIN);
}

void display_wake()
{
  write_command(ST7789_SLPOUT);
  write_command(ST7789_DISPON);
}

#endif // NRF_MODULE_ENABLED(ST7789)
