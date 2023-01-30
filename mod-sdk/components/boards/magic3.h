#ifndef MAGIC3_BOARD_H
#define MAGIC3_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"
#include <nrf_saadc.h>

#define LEDS_NUMBER       1
#define LCD_BACKLIGHT     NRF_GPIO_PIN_MAP(0,12)
#define LED_1             LCD_BACKLIGHT
#define LEDS_ACTIVE_STATE 1
#define LEDS_LIST         { LED_1 }
#define LEDS_INV_MASK     LEDS_MASK
#define BSP_LED_0         LED_1

#define BUTTONS_NUMBER       1
#define BUTTON_1             NRF_GPIO_PIN_MAP(0,26)
#define BUTTON_PULL          NRF_GPIO_PIN_PULLDOWN
#define BUTTONS_ACTIVE_STATE 1
#define BUTTONS_LIST         { BUTTON_1 }
#define BSP_BUTTON_0         BUTTON_1

#define ST7789_DC_PIN   NRF_GPIO_PIN_MAP(1,15)
#define ST7789_RST_PIN  NRF_GPIO_PIN_MAP(0,2)

#define SPIM_SS_PIN    NRF_GPIO_PIN_MAP(0,3)
#define SPIM_SCK_PIN   NRF_GPIO_PIN_MAP(1,13)
#define SPIM_MOSI_PIN  NRF_GPIO_PIN_MAP(1,12)
#define SPIM_MISO_PIN  NRFX_SPIM_PIN_NOT_USED
#define SPIM_MODE      NRF_SPIM_MODE_0
#define SPIM_FREQ      NRF_SPIM_FREQ_32M

#define SDA_PIN 15
#define SCL_PIN 14
#define I2C_ENABLE      NRF_GPIO_PIN_MAP(0,7)

#define TOUCH_ADDRESS   0x15
#define TOUCH_IRQ_PIN   32
#define TOUCH_RESET_PIN 39

/*
12-+-backlight

26-+-button, 1=pressed
46---button 2 on Kospet Rock MOY-NAX3 , needs pull down - D46.mode("input_pulldown"), 1 when pressed

47-+-LCD D/C
02-+-LCD RST
03-+-LCD CS
45-+-LCD SCK
44-+-LCD MOSI

14-+-I2C SCL
15-+-I2C SDA - devices on bus 0x15 (touch) ,0x18 (accel) ,0x44 (hr sensor?)

32-+-touch IRQ
39-+-touch RST

07-+-1 = enables i2c and vibration; turns red led off
06-+-vibration 0=on

16---sc7a20 accel IRQ pin

17---SPI Flash CS
19---SPI Flash SCK
20---MOSI/IO0
21---MISO/IO1
22---WP/IO2
23---RS/HOLD/IO3

08---charger, 0=attached
30---battery voltage, 4.20/0.60 * analogRead(D30)

18---unused - 52840 reset pin
*/

#ifdef __cplusplus
}
#endif

#endif
