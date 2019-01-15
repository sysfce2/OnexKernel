#ifndef RFDUINO_BOARD_H
#define RFDUINO_BOARD_H

#define LEDS_NUMBER    1
#define LED_START      3
#define LED_1          3
#define LED_2          3
#define LED_3          3
#define LED_STOP       3
#define LED_1_MASK    (1<<LED_1)
#define LED_2_MASK    (1<<LED_2)
#define LED_3_MASK    (1<<LED_3)
#define LEDS_LIST     { LED_1 }
#define LEDS_MASK      LED_1_MASK
#define LEDS_INV_MASK  LEDS_MASK

#define BUTTONS_NUMBER 0
#define BUTTONS_LIST {}
#define BUTTONS_MASK   0x00000000

#define RX_PIN_NUMBER  0
#define TX_PIN_NUMBER  1
//#define CTS_PIN_NUMBER 10
//#define RTS_PIN_NUMBER 8
#define HWFC           true

#define PIN_SPI_SS     6
#define PIN_SPI_MOSI   5
#define PIN_SPI_MISO   3
#define PIN_SPI_SCK    4

#define PIN_WIRE_SDA   6
#define PIN_WIRE_SCL   5

#endif
