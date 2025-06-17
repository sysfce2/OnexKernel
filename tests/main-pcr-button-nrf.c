
#include <boards.h>
#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onex-kernel/gpio.h>
#include <onn.h>

static volatile bool button_pressed=false;

static void button_changed(uint8_t pin, uint8_t type){
  button_pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  onex_run_evaluators("uid-button", (void*)button_pressed);
}

bool evaluate_button(object* button, void* pressed) {
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
  return true;
}

int main() {

  properties* config = properties_new(32);
  properties_set(config, "channels", list_vals_new_from_fixed("radio"));
  properties_set(config, "flags", list_vals_new_from_fixed("debug-on-serial log-onp log-to-led"));
  properties_set(config, "test-uid-prefix", value_new("button"));

  time_init();
  log_init(config);
  gpio_init();
  random_init();

  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);

  onex_init(config);

  log_write("\n------Starting Button Test-----\n");

  onex_set_evaluators("eval_button", evaluate_button, 0);
  object* button=object_new("uid-button", "eval_button", "button", 4);
  object_property_set(button, "state", "up");

  while(1){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
}

