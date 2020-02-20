
#if defined(NRF5)
#include <variant.h>
#include <onex-kernel/gpio.h>
// #include <onex-kernel/serial.h>
#else
#include <onex-kernel/log.h>
#endif
#include <onex-kernel/time.h>
#include <onf.h>

object* light;

#if defined(NRF5)
const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;
#endif

bool evaluate_light(object* light, void* d);

// Copied from ONR Behaviours
bool evaluate_device_logic(object* o, void* d)
{
  if(object_property_contains(o, (char*)"Alerted:is", (char*)"device")){
    char* devuid=object_property(o, (char*)"Alerted");
    if(!object_property_contains(o, (char*)"connected-devices", devuid)){
      object_property_add(o, (char*)"connected-devices", devuid);
    }
  }
  return true;
}

int main()
{
  time_init();
  onex_init("");

#if defined(NRF5)
  for(uint8_t l=0; l< LEDS_NUMBER; l++) gpio_mode(leds_list[l], OUTPUT);
#else
  time_delay_s(2);
  log_init(9600);
  log_write("\n------Starting Light Test-----\n");
#endif

  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);
  onex_set_evaluators("evaluate_light", evaluate_light, 0);

  object_set_evaluator(onex_device_object, (char*)"evaluate_device");

  light=object_new(0, "evaluate_light", "editable light", 4);

  char* lightuid=object_property(light, "UID");
  object_property_add(onex_device_object, (char*)"io", lightuid);

  char* deviceuid=object_property(onex_device_object, "UID");
  object_property_set(light, "device", deviceuid);
  object_property_set(light, "light", "off");

  uint16_t todo=0;
  while(1){

    onex_loop();

    if(todo<2 && time_ms() >1000u+2000u*todo){  todo++;
      onex_run_evaluators(lightuid, 0);
    }
  }
}

bool evaluate_light(object* light, void* d)
{
  char* buttonuid=object_property(light, "device:connected-devices:io");
  if(buttonuid) object_property_set(light, "button", buttonuid);

  bool buttonpressed=object_property_is(light, "button:state", "down");
  char* s=(char*)(buttonpressed? "on": "off");
  object_property_set(light, "light", s);
#if defined(NRF5)
  if(buttonpressed){
    gpio_set(leds_list[0], 1);
    gpio_set(leds_list[1], 1);
    gpio_set(leds_list[2], 0);
    gpio_set(leds_list[3], 1);
    gpio_set(leds_list[4], 1);
    gpio_set(leds_list[5], 1);
    gpio_set(leds_list[6], 1);
    gpio_set(leds_list[7], 1);
    gpio_set(leds_list[8], 1);
    gpio_set(leds_list[10], 1);
  } else {
    gpio_set(leds_list[10], 0);
  }
#else
  log_write("evaluate_light: "); object_log(light);
#endif
  return true;
}

