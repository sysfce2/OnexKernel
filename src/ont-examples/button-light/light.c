
#if defined(TARGET_MCU_NRF51822)
#include <variant.h>
#include <onex-kernel/gpio.h>
#else
#include <onex-kernel/log.h>
#endif
#include <onex-kernel/time.h>
#include <onf.h>

object* light;

#if defined(TARGET_MCU_NRF51822)
const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;
#endif

bool evaluate_light(object* light, void* d);

int main()
{
  time_init();
  onex_init("");

#if defined(TARGET_MCU_NRF51822)
  for(uint8_t l=0; l< LEDS_NUMBER; l++) gpio_mode(leds_list[l], OUTPUT);
  for(uint8_t l=0; l< LEDS_NUMBER; l++) gpio_set(leds_list[l], 0);
  time_delay_ms(300);
  for(uint8_t l=0; l< LEDS_NUMBER; l++) gpio_set(leds_list[l], 1);
#else
  time_delay_s(2);
  log_init(9600);
  log_write("\n------Starting Light Test-----\n");
#endif

  onex_set_evaluator("evaluate_light", evaluate_light);
  light=object_new(0, "evaluate_light", "light", 4);
  object_property_set(light, "button", "uid-1-2-3");
  char* uid=object_property(light, (char*)"UID");

  int todo=0;
  while(1){

    onex_loop();

    if(todo<2 && time_ms() >2000+2000*todo){  todo++;
      onex_run_evaluator(uid, 0, 0, 0);
    }
  }
}

bool evaluate_light(object* light, void* d)
{
  bool buttonpressed=object_property_is(light, "button:state", "down");
  char* s=(char*)(buttonpressed? "on": "off");
  object_property_set(light, "light", s);
  log_write("evaluate_light: "); object_log(light);
  return true;
}

