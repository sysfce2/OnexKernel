
#include <boards.h>
#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onex-kernel/gpio.h>
#include <onn.h>

object* light;

bool evaluate_light(object* light, void* d) {

  if(object_property_is(light, "button:state", "up"  )){
    object_property_set(light,  "light", "off");
  }
  if(object_property_is(light, "button:state", "down")){
    object_property_set(light,  "light", "on");
  }

  if(object_property_is(light, "light", "on")){
#if defined(BOARD_PCA10059)
    gpio_set(LED2_B, LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
    gpio_set(LED_1, LEDS_ACTIVE_STATE);
#endif
  } else {
#if defined(BOARD_PCA10059)
    gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
    gpio_set(LED_1, !LEDS_ACTIVE_STATE);
#endif
  }
  return true;
}

int main(){

  properties* config = properties_new(32);
  properties_set(config, "channels", list_new_from_fixed("radio"));
  properties_set(config, "flags", list_new_from_fixed("debug-on-serial log-to-led"));
  properties_set(config, "test-uid-prefix", value_new("light"));

  time_init();
  log_init(config);
  gpio_init();
  random_init();

  onex_init(config);

  log_write("\n------Starting Light Test -----\n");

#if defined(BOARD_PCA10059)
  gpio_mode(LED1_G, OUTPUT);
  gpio_mode(LED2_B, OUTPUT);
#elif defined(BOARD_FEATHER_SENSE)
  gpio_mode(LED_1, OUTPUT);
#endif

  onex_set_evaluators("evaluate_light",  evaluate_light, 0);

  light=object_new("uid-light", "evaluate_light",  "light", 4);

  object_property_set(light,  "light", "off");
  object_property_set(light,  "button", "uid-button");

  onex_run_evaluators("uid-light", 0);

#if defined(BOARD_PCA10059)
  gpio_set(LED1_G, LEDS_ACTIVE_STATE);
  gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
  gpio_set(LED_1, LEDS_ACTIVE_STATE);
#endif

  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  time_end();
}

// --------------------------------------------------------------------

