#ifndef PINETIME_H
#define PINETIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"
#include <nrf_saadc.h>

#define LEDS_NUMBER    3

#define LCD_BACKLIGHT_LOW           NRF_GPIO_PIN_MAP(0, 14)
#define LCD_BACKLIGHT_MID           NRF_GPIO_PIN_MAP(0, 22)
#define LCD_BACKLIGHT_HIGH          NRF_GPIO_PIN_MAP(0, 23)
#define VIBRATION                   NRF_GPIO_PIN_MAP(0, 16)

#define LED_1          LCD_BACKLIGHT_LOW
#define LED_2          LCD_BACKLIGHT_MID
#define LED_3          LCD_BACKLIGHT_HIGH

#define LEDS_ACTIVE_STATE 0

#define LEDS_LIST { LED_1, LED_2, LED_3 }

#define LEDS_INV_MASK  LEDS_MASK

#define BSP_LED_0      LED_1
#define BSP_LED_1      LED_2
#define BSP_LED_2      LED_3

#define BUTTONS_NUMBER 1

#define BUTTON_1       NRF_GPIO_PIN_MAP(0,13)
#define BUTTON_ENABLE  NRF_GPIO_PIN_MAP(0,15)

#define BUTTON_PULL          NRF_GPIO_PIN_PULLDOWN
#define BUTTONS_ACTIVE_STATE 1

#define BUTTONS_LIST { BUTTON_1 }

#define BSP_BUTTON_0   BUTTON_1

#define CHARGE_SENSE NRF_GPIO_PIN_MAP(0,12)
#define BATTERY_V    NRF_SAADC_INPUT_AIN7 // NRF_GPIO_PIN_MAP(0,31)

#define ST7789_DC_PIN   NRF_GPIO_PIN_MAP(0,18)
#define ST7789_RST_PIN  NRF_GPIO_PIN_MAP(0,26)

#define SPIM_SCK_PIN   NRF_GPIO_PIN_MAP(0,2)
#define SPIM_MOSI_PIN  NRF_GPIO_PIN_MAP(0,3)
#define SPIM_MISO_PIN  NRF_GPIO_PIN_MAP(0,4)
#define SPIM_SS_PIN    NRF_GPIO_PIN_MAP(0,25)
#define SPIM_MODE      NRF_SPIM_MODE_3
#define SPIM_FREQ      NRF_SPIM_FREQ_8M

#define SDA_PIN 6
#define SCL_PIN 7

#define MOTION_ADDRESS 0x18
#define MOTION_IRQ_PIN 8

#define TOUCH_ADDRESS 0x15
#define TOUCH_IRQ_PIN 28
#define TOUCH_RESET_PIN 10

#ifdef __cplusplus
}
#endif

#endif
