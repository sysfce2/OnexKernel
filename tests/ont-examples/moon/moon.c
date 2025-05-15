
// --------------------------------------------------------------------

#include <boards.h>

#include <string.h>

#include <onex-kernel/boot.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/random.h>
#include <onex-kernel/time.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>

#include <onex-kernel/seesaw.h>
#include <onex-kernel/compass.h>
#include <onex-kernel/colours.h>
#include <onex-kernel/led-strip.h>
#include <onex-kernel/led-matrix.h>

static volatile bool display_state_prev= LEDS_ACTIVE_STATE;
static volatile bool display_state     =!LEDS_ACTIVE_STATE;

void button_changed(uint8_t pin, uint8_t type) {
  bool pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  if(pressed) display_state = !display_state;
}

const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;

#define DISPLAY_STATE_LED 0

#define BATT_ADC_CHANNEL 0
#define POT1_ADC_CHANNEL 1
#define POT2_ADC_CHANNEL 2

static void set_up_gpio() {

  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);

  for(uint8_t l=0; l< LEDS_NUMBER; l++){
    gpio_mode(leds_list[l], OUTPUT); gpio_set(leds_list[l], !LEDS_ACTIVE_STATE);
   }
  gpio_set(leds_list[DISPLAY_STATE_LED], display_state);

  gpio_adc_init(BATTERY_V, BATT_ADC_CHANNEL);
  gpio_adc_init(GPIO_A0,   POT1_ADC_CHANNEL);
  gpio_adc_init(GPIO_A1,   POT2_ADC_CHANNEL);
}

static void loop_serial(void*){ serial_loop(); }

int main() {

  properties* config = properties_new(32);
  properties_set(config, "flags", list_new_from("debug-on-serial log-to-led",2));

  time_init();

  log_init(config);

  serial_ready_state();
  time_ticker(loop_serial, 0, 1);

  random_init();
  gpio_init();
  set_up_gpio();

  uint8_t usb_status = serial_ready_state();

  led_matrix_init();

  if(usb_status == SERIAL_POWERED_NOT_READY){
    led_strip_fill_col( "#700");
    led_matrix_fill_col("#200");
    led_strip_show(); led_matrix_show();
    log_flash(1,0,0);
    time_delay_ms(500);
    boot_reset(false);
  }

  compass_init();

  log_write("starting the moooon!\n");

  while(1){

    log_loop();

    if(display_state){

      compass_info_t ci = compass_direction();
      uint8_t       tmp = compass_temperature();

      uint8_t hue = (uint8_t)(((uint32_t)ci.o + 180)*256/360);

      led_strip_fill_hsv( (colours_hsv){ hue,255,127 });
      led_matrix_fill_hsv((colours_hsv){ hue,255, 63 });
      led_strip_show(); led_matrix_show();

    } else {

      static bool do_rotary_encoder=true;
      if(do_rotary_encoder){
        #define ROTARY_ENC_ADDRESS 0x36
        uint16_t version_hi = seesaw_status_version_hi(ROTARY_ENC_ADDRESS);
        if(version_hi != 4991){
          log_write("version: mismatch, should be 4991: %d\n", version_hi);
          do_rotary_encoder = false;
        }
      }

      int32_t rotn = do_rotary_encoder? seesaw_encoder_position(ROTARY_ENC_ADDRESS): 0;
      int16_t pot1 = gpio_read(POT1_ADC_CHANNEL);
      int16_t pot2 = gpio_read(POT2_ADC_CHANNEL);

      if(pot1<0)pot1=0;
      if(pot2<0)pot2=0;

      uint8_t colour     = rotn*4;   // lo byte, 4 lsb per click
      uint8_t contrast   = pot2/4;   // 0..1023
      uint8_t brightness = pot1/4;   // 0..1023

      led_strip_fill_hsv( (colours_hsv){ colour,contrast,brightness   });
      led_matrix_fill_hsv((colours_hsv){ colour,contrast,brightness/4 });
      led_strip_show(); led_matrix_show();
    }

    if (display_state_prev != display_state){
      display_state_prev = display_state;
      gpio_set(leds_list[DISPLAY_STATE_LED], display_state);
    }

    time_delay_ms(20);
  }
}

// --------------------------------------------------------------------
