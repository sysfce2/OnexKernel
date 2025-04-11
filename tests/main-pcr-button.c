
#include <time.h>
#include <stdlib.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onn.h>

static bool button_pressed=false;
static void every_1200(){
  button_pressed=!button_pressed;
  onex_run_evaluators("uid-b", (void*)button_pressed);
}

bool evaluate_button(object* button, void* pressed) {
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
  log_write("evaluate_button: "); object_log(button);
  return true;
}

int main() {

  log_init();
  time_init();

  onex_init_ipv6("button.db", "ff12::1234");

  log_write("\n------Starting Button Test Server-----\n");

  onex_set_evaluators("evaluate_button", evaluate_button, 0);
  object* button=object_new("uid-b", "evaluate_button", "button", 4);
  object_property_set(button, "state", "up");
  onex_run_evaluators("uid-b", 0);

  time_ticker(every_1200, 1200);

  while(1){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
}

