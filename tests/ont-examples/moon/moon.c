
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

#define DISPLAY_STATE_LED 0

#define ROTARY_ENC_ADDRESS 0x36
#define ROTARY_ENC_BUTTON  24

#define POT1_ADC_CHANNEL 1
#define POT2_ADC_CHANNEL 2

static bool do_rotary_encoder=true;

static volatile bool display_state_prev= LEDS_ACTIVE_STATE;
static volatile bool display_state     =!LEDS_ACTIVE_STATE;

static volatile bool rotary_button_pressed_prev = false;
static volatile bool rotary_button_pressed      = false;

void button_changed(uint8_t pin, uint8_t type) {
  bool pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  if(pressed) display_state = !display_state;
}

const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;

static void set_up_gpio() {

  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);

  for(uint8_t l=0; l< LEDS_NUMBER; l++){
    gpio_mode(leds_list[l], OUTPUT); gpio_set(leds_list[l], !LEDS_ACTIVE_STATE);
  }
  gpio_set(leds_list[DISPLAY_STATE_LED], display_state);

  gpio_adc_init(GPIO_A0,   POT1_ADC_CHANNEL);
  gpio_adc_init(GPIO_A1,   POT2_ADC_CHANNEL);

  uint16_t version_hi = seesaw_status_version_hi(ROTARY_ENC_ADDRESS);
  if(version_hi != 4991){
    log_write("rotary encoder id 4991 not found: %d\n", version_hi);
    do_rotary_encoder = false;
  }
  else{
    seesaw_gpio_input_pullup(ROTARY_ENC_ADDRESS, ROTARY_ENC_BUTTON);
  }
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

  led_matrix_init();

  if(serial_ready_state() == SERIAL_POWERED_NOT_READY){
    led_strip_fill_col( "#700");
    led_matrix_fill_col("#200");
    led_strip_show(); led_matrix_show();
    log_flash(1,0,0);
    time_delay_ms(500);
    boot_reset(false);
  }

  gpio_init();
  set_up_gpio();
  compass_init();

  log_write("starting the moooon!\n");

  while(1){

    if(do_rotary_encoder){
      rotary_button_pressed = !seesaw_gpio_read(ROTARY_ENC_ADDRESS, ROTARY_ENC_BUTTON);
      if(rotary_button_pressed != rotary_button_pressed_prev){
        rotary_button_pressed_prev = rotary_button_pressed;
        if(rotary_button_pressed){
          display_state = !display_state;
        }
      }
    }
    if(display_state_prev != display_state){
      display_state_prev = display_state;
      log_write("in %s mode with%s rotary controls\n",
                display_state? "compass": "manual",
                do_rotary_encoder? "": "out"
      );
      gpio_set(leds_list[DISPLAY_STATE_LED], display_state);
    }

    uint8_t colour;

    if(display_state){

      compass_info_t ci = compass_direction();
      colour = (uint8_t)(((uint32_t)ci.o + 180)*256/360);

    } else {

      int32_t rotn = do_rotary_encoder? seesaw_encoder_position(ROTARY_ENC_ADDRESS): 0;
      colour = (uint8_t)(rotn*4);   // lo byte, 4 lsb per click
    }

    int16_t pot1 = do_rotary_encoder? gpio_read(POT1_ADC_CHANNEL): 1023;
    int16_t pot2 = do_rotary_encoder? gpio_read(POT2_ADC_CHANNEL): 1023;

    if(pot1<0) pot1=0;
    if(pot2<0) pot2=0;

    uint8_t contrast   = pot2/4;   // 0..1023
    uint8_t brightness = pot1/4;   // 0..1023

    led_strip_fill_hsv( (colours_hsv){ colour,contrast,brightness   });
    led_matrix_fill_hsv((colours_hsv){ colour,contrast,brightness/4 });
    led_strip_show(); led_matrix_show();

    time_delay_ms(20);

    log_loop();
  }
}

// --------------------------------------------------------------------
