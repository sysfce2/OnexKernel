
#if defined(TARGET_MCU_NRF51822)
#include <variant.h>
#endif
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onf.h>

object* light;

bool evaluate_light(object* light);

int main()
{
  time_init();
  onex_init();

  time_delay_s(2);
  log_init(9600);
  log_write("\n------Starting Light Test-----\n");

  light=object_new("uid-3-2-1", "light", evaluate_light, 4);
  object_property_set(light, "button", "uid-1-2-3");

  int todo=0;
  while(1){

    onex_loop();

    if(todo<2 && time_ms() >2000+2000*todo){  todo++;
      onex_run_evaluators(light);
    }
  }
}

bool evaluate_light(object* light)
{
  bool buttonpressed=object_property_is(light, "button:state", "down");
  char* s=(char*)(buttonpressed? "on": "off");
  object_property_set(light, "light", s);
  log_write("evaluate_light: "); object_log(light);
  log_write("random number test: %d\n", random_ish_byte());
  return true;
}

