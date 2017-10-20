
#include <variant.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onf.h>

object* light;

bool evaluate_light(object* light);

int main()
{
  time_init();
  onex_init();

  time_delay_s(2);
#if !defined(TARGET_MCU_NRF51822) || !defined(ONP_CHANNEL_SERIAL)
  serial_init(0, 9600);
  serial_printf("\n------Starting Light Test-----\n");
#endif

  light=object_new("uid-3-2-1", "light", evaluate_light, 4);
  object_property_set(light, "button", "uid-1-2-3");

  int todo=0;
  while(1){

    onex_loop();

    if(todo<90 && time_ms() >2000+2000*todo){  todo++;
      onex_run_evaluators(light);
    }
  }
}

bool evaluate_light(object* light)
{
  bool buttonpressed=object_property_is(light, "button:state", "down");
  char* s=(char*)(buttonpressed? "on": "off");
  object_property_set(light, "light", s);
#if !defined(TARGET_MCU_NRF51822) || !defined(ONP_CHANNEL_SERIAL)
  char b[128]; serial_printf("%s\n", object_to_text(light,b,128));
#endif
  return true;
}

