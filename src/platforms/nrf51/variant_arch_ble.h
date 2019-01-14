#ifndef ARCH_BLE_BOARD_H
#define ARCH_BLE_BOARD_H

#define LEDS_NUMBER    1
#define LED_START      30
#define LED_1          30
#define LED_STOP       30
#define LEDS_LIST     { LED_1 }
#define LED_1_MASK    (1<<LED_1)
#define LEDS_MASK      LED_1_MASK
#define LEDS_INV_MASK  LEDS_MASK

#define BUTTONS_NUMBER 0
#define BUTTONS_LIST {}
#define BUTTONS_MASK   0x00000000

#define RX_PIN_NUMBER  7
#define TX_PIN_NUMBER  8
#define CTS_PIN_NUMBER 26
#define RTS_PIN_NUMBER 27
#define HWFC           true

#endif
