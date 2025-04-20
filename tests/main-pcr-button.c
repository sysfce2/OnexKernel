
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onn.h>

static bool button_pressed=false;

static void every_tick(void*){
  button_pressed=!button_pressed;
  onex_run_evaluators("uid-b", (void*)button_pressed);
}

bool evaluate_button(object* button, void* pressed) {
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
  log_write("evaluate_button: "); object_log(button);
  return true;
}

int main(int argc, char *argv[]) {

  if(argc<=1){
    log_write("Usage: %s <mcast group: ff12::1234 / ff12::4321>\n", argv[0]);
    return -1;
  }
  properties* config = properties_new(32);
  properties_set(config, "dbpath", value_new("button.ondb"));
  properties_set(config, "channels", list_new_from("ipv6", 1));
  properties_set(config, "ipv6_groups", list_new_from(argv[1], 1));
  properties_set(config, "test-uid-prefix", value_new("button"));

  time_init();
  log_init(config);
  random_init();

  onex_init(config);

  log_write("\n------Starting Button Test Server-----\n");

  onex_set_evaluators("evaluate_button", evaluate_button, 0);
  object* button=object_new("uid-b", "evaluate_button", "button", 4);
  object_property_set(button, "state", "up");
  onex_run_evaluators("uid-b", 0);

  time_ticker(every_tick, 0, 2000);

  while(1){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
}

