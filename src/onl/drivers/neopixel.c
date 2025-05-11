
#include <boards.h>

#include <onex-kernel/time.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/led-strip.h>

#define LED_STRIP_COUNT 1

static const uint16_t num_leds=LED_STRIP_COUNT;

static uint8_t led_strip_array[LED_STRIP_COUNT][3];

void led_strip_init(){ }

void led_strip_fill_hsv(colours_hsv hsv){
  colours_rgb rgb = colours_hsv_to_rgb(hsv);
  led_strip_fill_rgb(rgb);
}

void led_strip_fill_rgb(colours_rgb rgb){
  for(uint16_t i=0; i<num_leds; i++){
    led_strip_array[i][0]=rgb.r;
    led_strip_array[i][1]=rgb.g;
    led_strip_array[i][2]=rgb.b;
  }
}

void led_strip_fill_col(char* colour){
  led_strip_fill_rgb(colours_parse_colour_string(colour));
}

#define ZERO_COUNTS       6 | 0x8000  // 0.3750 µs
#define ONE_COUNTS       13 | 0x8000  // 0.8125 µs 1.1875
#define TOTAL_COUNTS     20           // 1.2500 µs total period

void led_strip_show(){

  uint8_t r = led_strip_array[0][0];
  uint8_t g = led_strip_array[0][1];
  uint8_t b = led_strip_array[0][2];

  static uint16_t pwm_bits[24 + 2];
  uint8_t grb[3] = {g, r, b};
  int idx = 0;
  for (int i = 0; i < 3; i++) {
      for (int bit = 7; bit >= 0; bit--) {
          pwm_bits[idx++] = (grb[i] & (1 << bit)) ? ONE_COUNTS : ZERO_COUNTS;
      }
  }
  pwm_bits[idx++] = 0x0 | 0x8000;
  pwm_bits[idx++] = 0x0 | 0x8000;

  gpio_mode(LED_NEOPIXEL, OUTPUT);

  NRF_PWM0->ENABLE      = PWM_ENABLE_ENABLE_Disabled;

  NRF_PWM0->MODE        = PWM_MODE_UPDOWN_Up             << PWM_MODE_UPDOWN_Pos;
  NRF_PWM0->PRESCALER   = PWM_PRESCALER_PRESCALER_DIV_1  << PWM_PRESCALER_PRESCALER_Pos;  // 16 MHz
  NRF_PWM0->COUNTERTOP  = TOTAL_COUNTS                   << PWM_COUNTERTOP_COUNTERTOP_Pos;
  NRF_PWM0->LOOP        = PWM_LOOP_CNT_Disabled          << PWM_LOOP_CNT_Pos;
  NRF_PWM0->DECODER     = (PWM_DECODER_LOAD_Common       << PWM_DECODER_LOAD_Pos) |
                          (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

  NRF_PWM0->SEQ[0].PTR = ((uint32_t)pwm_bits) << PWM_SEQ_PTR_PTR_Pos;
  NRF_PWM0->SEQ[0].CNT = idx                  << PWM_SEQ_CNT_CNT_Pos;
  NRF_PWM0->SEQ[0].REFRESH = 0;
  NRF_PWM0->SEQ[0].ENDDELAY = 0;

  NRF_PWM0->PSEL.OUT[0] = LED_NEOPIXEL;

  NRF_PWM0->ENABLE = PWM_ENABLE_ENABLE_Enabled;
  NRF_PWM0->EVENTS_SEQEND[0] = 0;
  NRF_PWM0->TASKS_SEQSTART[0] = 1;
  while(!NRF_PWM0->EVENTS_SEQEND[0]);
  NRF_PWM0->EVENTS_SEQEND[0] = 0;

  NRF_PWM0->ENABLE = PWM_ENABLE_ENABLE_Disabled;
  NRF_PWM0->PSEL.OUT[0] = 0xFFFFFFFFUL;

  time_delay_us(70);
}

// -------------- bit bang code -----------------

static inline void delay_ns(uint32_t ns) {

    uint32_t cycles = (ns/2 + 62) / 63;

    __asm__ volatile (
        "1: \n"
        "nop \n"
        "subs %[cnt], %[cnt], #1 \n"
        "bne 1b \n"
        : [cnt] "+r" (cycles)
        :
        : "cc"
    );
}

#define ZERO_ON_NS    375
#define ONE_ON_NS     813
#define TOTAL_PERIOD 1250

static inline void send_bit(bool bit) {
    nrf_gpio_pin_set(LED_NEOPIXEL);
    if (bit) {
        delay_ns(ONE_ON_NS);
        nrf_gpio_pin_clear(LED_NEOPIXEL);
        delay_ns(TOTAL_PERIOD-ONE_ON_NS);
    } else {
        delay_ns(ZERO_ON_NS);
        nrf_gpio_pin_clear(LED_NEOPIXEL);
        delay_ns(TOTAL_PERIOD-ZERO_ON_NS);
    }
}

void neopixel_show(uint8_t r, uint8_t g, uint8_t b) {

    gpio_mode(LED_NEOPIXEL, OUTPUT);

    __disable_irq();

    for (int i = 7; i >= 0; i--) send_bit((g >> i) & 1);
    for (int i = 7; i >= 0; i--) send_bit((r >> i) & 1);
    for (int i = 7; i >= 0; i--) send_bit((b >> i) & 1);

    __enable_irq();

    time_delay_us(70);
}


