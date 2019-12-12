
#if defined(NRF5)
#include <variant.h>
#include <onex-kernel/gpio.h>
#endif
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onf.h>

object* button;

void button_1_change_cb(int);
bool evaluate_button(object* button, void* pressed);

int main()
{
  time_init();
  onex_init("");
#if defined(NRF5)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, button_1_change_cb);
#endif

  time_delay_s(1);
  log_init(9600);
  log_write("\n------Starting Button Test-----\n");

  onex_set_evaluators("evaluate_button", evaluate_button, 0);
  button=object_new("uid-1-2-3", "evaluate_button", "button", 4);
  object_property_set(button, "name", "£€§");

#if !defined(NRF5)
  int lasttime=0;
  bool button_pressed=false;
#endif

  while(1){

    onex_loop();

    time_delay_ms(1);
#if !defined(NRF5)
    if(time_ms() > lasttime+1000){
      lasttime=time_ms();
      button_pressed=!button_pressed;
      button_1_change_cb(button_pressed);
    }
#endif
  }
}

void button_1_change_cb(int button_pressed)
{
  onex_run_evaluators("uid-1-2-3", (void*)(bool)button_pressed);
}

bool evaluate_button(object* button, void* pressed)
{
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
#if !defined(NRF5)
  log_write("evaluate_button: "); object_log(button);
#endif
  return true;
}


