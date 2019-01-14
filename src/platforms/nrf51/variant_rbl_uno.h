#ifndef RBL_UNO_BOARD_H
#define RBL_UNO_BOARD_H

#define LEDS_NUMBER    1
#define LED_START      15
#define LED_1          15
#define LED_2          15
#define LED_3          15
#define LED_STOP       15
#define LED_1_MASK    (1<<LED_1)
#define LED_2_MASK    (1<<LED_2)
#define LED_3_MASK    (1<<LED_3)
#define LEDS_LIST     { LED_1 }
#define LEDS_MASK      LED_1_MASK
#define LEDS_INV_MASK  LEDS_MASK

#define BUTTONS_NUMBER 0
#define BUTTONS_LIST {}
#define BUTTONS_MASK   0x00000000

#define RX_PIN_NUMBER  11
#define TX_PIN_NUMBER  9
#define CTS_PIN_NUMBER 10
#define RTS_PIN_NUMBER 8
#define HWFC           true

#define PIN_SPI_SS     14
#define PIN_SPI_MOSI   20
#define PIN_SPI_MISO   22
#define PIN_SPI_SCK    25

#define PIN_WIRE_SDA   29
#define PIN_WIRE_SCL   28

#endif
