#ifndef FEATHER_SENSE_H
#define FEATHER_SENSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

#define LEDS_NUMBER    1

#define LED_1          NRF_GPIO_PIN_MAP(1,10)
#define LEDS_ACTIVE_STATE 1
#define LEDS_LIST { LED_1 }
#define LEDS_INV_MASK  LEDS_MASK
#define BSP_LED_0      LED_1

#define BUTTONS_NUMBER 1
#define BUTTON_1       NRF_GPIO_PIN_MAP(1,02)
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST { BUTTON_1 }
#define BSP_BUTTON_0   BUTTON_1

#define BSP_SELF_PINRESET_PIN NRF_GPIO_PIN_MAP(0,19)

#define RX_PIN_NUMBER  11
#define TX_PIN_NUMBER  9
#define CTS_PIN_NUMBER 31
#define RTS_PIN_NUMBER 31
#define HWFC           true

#ifdef __cplusplus
}
#endif

#endif
