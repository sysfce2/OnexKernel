
#if defined(TARGET_MCU_NRF51822)
#include <variant.h>
#endif
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onf.h>

object* button;

bool evaluate_button(object* button);

bool button_pressed=false;

int main()
{
  time_init();
  onex_init();

  time_delay_s(1);
  log_init(9600);
  log_write("\n------Starting Button Test-----\n");

  button=object_new("uid-1-2-3", "button", evaluate_button, 4);

  int lasttime=0;

  while(1){

    onex_loop();

    if(time_ms() > lasttime+1000){
       lasttime=time_ms();
       button_pressed=!button_pressed;
       onex_run_evaluators(button);
    }
  }
}

bool evaluate_button(object* button)
{
  char* s=(char*)(button_pressed? "down": "up");
  object_property_set(button, "state", s);
  log_write("evaluate_button: "); object_log(button);
  return true;
}

